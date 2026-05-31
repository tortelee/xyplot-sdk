// ============================================================
// plot_impl.h — Shared internal implementation header
// Owner: Agent E — Backend & Integration
// ============================================================
// Contains Plot::Impl definition shared between:
//   - src/plot.cpp              (render facade)
//   - src/plot_interaction.cpp  (interaction / handleEvent)
// NOT part of the public API.
//
// Dependencies on Agent C modules:
//   - layout_engine.h → LayoutResult type (used by Plot::Impl)
// ============================================================
#pragma once
#include "xyplot/plot.h"
#include "layout_engine.h"   // Agent C: LayoutResult, LayoutConfig
#include "hit_test.h"
#include <vector>
#include <string>

namespace xyplot {

// ── Interaction state machine ──
enum class InteractionState {
    Idle,
    Hovering,
    Selected,
    Dragging
};

// ── Internal series info ──
struct SeriesInfo {
    std::string name;
    std::vector<double> xs;
    std::vector<double> ys;
    int yAxisIndex = 0;
    LineStyle lineStyle;
    MarkerStyle markerStyle;
    enum SeriesType { Line, Scatter, Bar, Step, Area, Histogram,
                      ErrorBar, Polar, Heatmap, Contour };
    SeriesType type = Line;
    bool visible = true;
};

// ── Plot::Impl — shared between plot.cpp and plot_interaction.cpp ──
// LayoutResult comes from Agent C's layout_engine.h
struct Plot::Impl {
    // Data
    std::vector<SeriesInfo> series;
    std::string title;
    std::string xLabel, yLabel, yRightLabel;

    // Axis config
    ScaleType xScale = ScaleType::Linear;
    ScaleType yScale = ScaleType::Linear;
    ScaleType yRightScale = ScaleType::Linear;
    double xMin = 0, xMax = 1;
    double yMin = 0, yMax = 1;
    double yRightMin = 0, yRightMax = 1;
    bool autoRange = true;

    // Layout cache (LayoutResult from Agent C's layout_engine.h)
    LayoutResult layout;
    double canvasWidth = 800;
    double canvasHeight = 600;

    // Interaction state
    InteractionState interactionState = InteractionState::Idle;
    int selectedSeriesIndex = -1;
    int hoveredSeriesIndex = -1;
    int hoveredDataPointIndex = -1;
    double lastMouseX = 0, lastMouseY = 0;

    // Compute auto-range from visible series data
    static void computeAutoRange(Impl* impl);

    // Add a series with specified type and axis binding
    static int addSeries(Impl* impl, const char* name,
                         const double* xs, const double* ys, int count,
                         SeriesInfo::SeriesType type, int yAxisIndex);
};

} // namespace xyplot
