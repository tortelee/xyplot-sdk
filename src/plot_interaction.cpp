// ============================================================
// plot_interaction.cpp — Interaction handler & handleEvent
// Owner: Agent E — Backend & Integration
// ============================================================
// Implements Plot::handleEvent with a state-machine-based
// interaction system. Uses HitTest to translate device-space
// input events into semantic chart interactions.
//
// State machine: Idle → Hovering ⇄ Selected → Dragging
//
// Events supported:
//   MouseDown  — select curve / pick data point / toggle legend
//   MouseMove  — hover tracking
//   MouseUp    — complete drag / confirm selection
//   MouseWheel — (reserved for future zoom)
//   KeyDown    — (reserved for keyboard shortcuts)
// ============================================================
#include "xyplot/plot.h"
#include "plot_impl.h"
#include "hit_test.h"
#include <cmath>
#include <algorithm>

namespace xyplot {

// ==================================================================
// Plot::handleEvent — main entry point for interaction
// ==================================================================
InteractionResult Plot::handleEvent(const InputEvent& event, IRenderDevice* /*device*/) {
    InteractionResult result;
    result.action = InteractionResult::None;

    auto* impl = m_impl;

    // ── Build HitTest context (inlined to access private Impl) ──
    auto buildLayout = [&]() {
        HitTestLayout hl;
        hl.plotArea = impl->layout.plotRect;
        hl.legendArea = impl->layout.legendRect;
        hl.legendItemHeight = 20.0;
        hl.legendItemCount = static_cast<int>(impl->series.size());
        return hl;
    };

    auto buildSeries = [&]() {
        std::vector<HitTestSeries> out;
        out.reserve(impl->series.size());
        for (const auto& s : impl->series) {
            HitTestSeries hs;
            hs.xs = s.xs.data();
            hs.ys = s.ys.data();
            hs.count = static_cast<int>(s.xs.size());
            hs.yAxisIndex = s.yAxisIndex;
            hs.visible = s.visible;
            out.push_back(hs);
        }
        return out;
    };

    auto buildAxis = [&]() {
        HitTestAxisRange ar;
        ar.xMin = impl->xMin;
        ar.xMax = impl->xMax;
        ar.yMin = impl->yMin;
        ar.yMax = impl->yMax;
        ar.yRightMin = impl->yRightMin;
        ar.yRightMax = impl->yRightMax;
        ar.xScale = impl->xScale;
        ar.yScale = impl->yScale;
        ar.yRightScale = impl->yRightScale;
        return ar;
    };

    switch (event.type) {

    // ── Mouse Down ──
    case InputEvent::MouseDown: {
        HitTestResult hit = HitTest::test(event.x, event.y,
                                          buildLayout(), buildSeries(), buildAxis());

        switch (hit.hitType) {
        case HitTestResult::LegendItem:
            if (hit.seriesIndex >= 0 && hit.seriesIndex < static_cast<int>(impl->series.size())) {
                if (impl->selectedSeriesIndex == hit.seriesIndex) {
                    impl->selectedSeriesIndex = -1;
                    impl->interactionState = InteractionState::Idle;
                    result.action = InteractionResult::CurveSelected;
                    result.selectedCurveIndex = -1;
                } else {
                    impl->selectedSeriesIndex = hit.seriesIndex;
                    impl->interactionState = InteractionState::Selected;
                    result.action = InteractionResult::CurveSelected;
                    result.selectedCurveIndex = hit.seriesIndex;
                }
            }
            break;

        case HitTestResult::DataPoint:
            impl->selectedSeriesIndex = hit.seriesIndex;
            impl->hoveredDataPointIndex = hit.dataPointIndex;
            impl->interactionState = InteractionState::Selected;
            result.action = InteractionResult::DataPicked;
            result.pickedDataX = hit.dataX;
            result.pickedDataY = hit.dataY;
            result.selectedCurveIndex = hit.seriesIndex;
            break;

        case HitTestResult::CurveLine:
            impl->selectedSeriesIndex = hit.seriesIndex;
            impl->interactionState = InteractionState::Selected;
            result.action = InteractionResult::CurveSelected;
            result.selectedCurveIndex = hit.seriesIndex;
            break;

        case HitTestResult::PlotArea:
            impl->selectedSeriesIndex = -1;
            impl->interactionState = InteractionState::Idle;
            result.action = InteractionResult::ViewChanged;
            break;

        default:
            break;
        }

        impl->lastMouseX = event.x;
        impl->lastMouseY = event.y;
        break;
    }

    // ── Mouse Move ──
    case InputEvent::MouseMove: {
        HitTestResult hit = HitTest::test(event.x, event.y,
                                          buildLayout(), buildSeries(), buildAxis());

        if (hit.hitType == HitTestResult::DataPoint ||
            hit.hitType == HitTestResult::CurveLine) {
            impl->hoveredSeriesIndex = hit.seriesIndex;
            impl->hoveredDataPointIndex = hit.dataPointIndex;
            if (impl->interactionState == InteractionState::Idle)
                impl->interactionState = InteractionState::Hovering;
        } else {
            impl->hoveredSeriesIndex = -1;
            impl->hoveredDataPointIndex = -1;
            if (impl->interactionState == InteractionState::Hovering)
                impl->interactionState = InteractionState::Idle;
        }

        // Dragging detection
        if (impl->interactionState == InteractionState::Selected) {
            double dx = event.x - impl->lastMouseX;
            double dy = event.y - impl->lastMouseY;
            if (std::abs(dx) > 2.0 || std::abs(dy) > 2.0)
                impl->interactionState = InteractionState::Dragging;
        }

        if (hit.hitType == HitTestResult::DataPoint) {
            result.action = InteractionResult::DataPicked;
            result.pickedDataX = hit.dataX;
            result.pickedDataY = hit.dataY;
            result.selectedCurveIndex = hit.seriesIndex;
        }

        impl->lastMouseX = event.x;
        impl->lastMouseY = event.y;
        break;
    }

    // ── Mouse Up ──
    case InputEvent::MouseUp: {
        if (impl->interactionState == InteractionState::Dragging) {
            impl->interactionState = InteractionState::Selected;
            result.action = InteractionResult::ViewChanged;
        }
        break;
    }

    // ── Mouse Wheel (reserved for zoom) ──
    case InputEvent::MouseWheel: {
        result.action = InteractionResult::ViewChanged;
        break;
    }

    // ── Keyboard / Touch (reserved) ──
    case InputEvent::KeyDown:
    case InputEvent::KeyUp:
    case InputEvent::TouchBegin:
    case InputEvent::TouchMove:
    case InputEvent::TouchEnd:
        break;

    default:
        break;
    }

    return result;
}

} // namespace xyplot
