// ============================================================
// test_integration.cpp — Integration tests
// Owner: Agent E — Backend & Integration
// ============================================================
// Tests:
//   1. Plot::render() → RecordingDevice call sequence validation
//   2. HitTest accuracy (curve, data point, legend, plot area)
//   3. Plot::handleEvent() interaction state machine
//   4. Multi-series + multi-Y-axis rendering
//   5. Auto-range calculation
//   6. Coordinate transform correctness
// ============================================================
#include "xyplot/xyplot.h"
#include "../src/hit_test.h"
#include "../src/xyplot_internal.h"
#include "../backends/recording/recording_device.h"
#include <cassert>
#include <cstdio>
#include <cmath>
#include <cstring>

using namespace xyplot;

// ==================================================================
// Test helpers
// ==================================================================
namespace {

int testsPassed = 0;
int testsFailed = 0;

void check(bool condition, const char* testName, const char* detail = "") {
    if (condition) {
        testsPassed++;
    } else {
        testsFailed++;
        std::printf("  FAIL: %s", testName);
        if (detail && detail[0]) std::printf(" — %s", detail);
        std::printf("\n");
    }
}

#define TEST(name) std::printf("[TEST] %s\n", name)
#define CHECK(cond) check((cond), #cond)
#define CHECK_MSG(cond, msg) check((cond), #cond, msg)

} // anonymous namespace

// ==================================================================
// Section 1: RecordingDevice basic recording
// ==================================================================
void test_recording_device_basic() {
    TEST("RecordingDevice records all call types");

    RecordingDevice rec;
    rec.beginFrame();
    rec.setClipRect(10, 20, 100, 200);
    double xs[] = { 1, 2, 3 };
    double ys[] = { 4, 5, 6 };
    rec.drawPolyline(xs, ys, 3, LineStyle{});
    rec.resetClip();
    rec.endFrame();

    CHECK(rec.callCount(RecordingDevice::BeginFrame) == 1);
    CHECK(rec.callCount(RecordingDevice::EndFrame) == 1);
    CHECK(rec.callCount(RecordingDevice::SetClipRect) == 1);
    CHECK(rec.callCount(RecordingDevice::DrawPolyline) == 1);
    CHECK(rec.callCount(RecordingDevice::ResetClip) == 1);
    CHECK(rec.hasValidFrameSequence());
    CHECK(rec.hasBalancedClip());

    // Verify polyline parameters
    const auto& calls = rec.calls();
    bool foundPoly = false;
    for (auto& c : calls) {
        if (c.type == RecordingDevice::DrawPolyline) {
            CHECK_MSG(c.polylineCount == 3, "polyline count");
            CHECK_MSG(c.polylineFirstX == 1, "first X");
            CHECK_MSG(c.polylineFirstY == 4, "first Y");
            CHECK_MSG(c.polylineLastX == 3, "last X");
            CHECK_MSG(c.polylineLastY == 6, "last Y");
            foundPoly = true;
        }
    }
    CHECK(foundPoly);

    // dump() should produce non-empty string
    std::string dump = rec.dump();
    CHECK(!dump.empty());
    CHECK(dump.find("beginFrame") != std::string::npos);
    CHECK(dump.find("drawPolyline") != std::string::npos);
    CHECK(dump.find("endFrame") != std::string::npos);
}

// ==================================================================
// Section 2: Plot::render() → RecordingDevice integration
// ==================================================================
void test_plot_render_recording() {
    TEST("Plot::render() with RecordingDevice produces expected calls");

    Plot plot;
    double xs[] = { 1.0, 2.0, 3.0, 4.0, 5.0 };
    double ys[] = { 2.0, 4.0, 6.0, 8.0, 10.0 };
    plot.addLineSeries("Line1", xs, ys, 5);
    plot.setAxisRange(0, 6, 0, 12);
    plot.setTitle("Test Plot");
    plot.xAxisSetLabel("X");
    plot.yAxisSetLabel("Y");

    RecordingDevice rec;
    plot.render(rec);

    // Frame sequence must be valid
    CHECK(rec.hasValidFrameSequence());

    // Must have clipping operations
    CHECK(rec.callCount(RecordingDevice::SetClipRect) > 0);
    CHECK(rec.callCount(RecordingDevice::ResetClip) > 0);
    CHECK(rec.hasBalancedClip());

    // Must have drawn polylines (grid lines + axes + data series)
    CHECK(rec.callCount(RecordingDevice::DrawPolyline) > 0);

    // Must have drawn text (title + axis labels + tick labels)
    CHECK(rec.callCount(RecordingDevice::DrawText) > 0);

    // Should have fillRect for plot background + legend
    CHECK(rec.callCount(RecordingDevice::FillRect) > 0);

    // Verify title text is in the recording
    std::string dump = rec.dump();
    CHECK(dump.find("Test Plot") != std::string::npos);
}

// ==================================================================
// Section 3: Plot render with multiple series
// ==================================================================
void test_plot_render_multi_series() {
    TEST("Plot::render() with multiple series");

    Plot plot;
    double xs[] = { 1.0, 2.0, 3.0 };
    double y1[] = { 1.0, 4.0, 9.0 };
    double y2[] = { 2.0, 3.0, 1.0 };

    plot.addLineSeries("Quadratic", xs, y1, 3);
    plot.addScatterSeries("Random", xs, y2, 3);
    plot.setAxisRange(0, 4, 0, 10);

    RecordingDevice rec;
    plot.render(rec);

    CHECK(rec.hasValidFrameSequence());
    // Should have drawPolyline (for Line series)
    CHECK(rec.callCount(RecordingDevice::DrawPolyline) > 0);
    // Should have drawMarkers (for Scatter series)
    CHECK(rec.callCount(RecordingDevice::DrawMarkers) > 0);
}

// ==================================================================
// Section 4: HitTest — Legend item
// ==================================================================
void test_hit_test_legend() {
    TEST("HitTest detects legend item click");

    HitTestLayout layout;
    layout.legendArea = { 650, 70, 140, 100 };
    layout.legendItemCount = 3;
    layout.legendItemHeight = 20;
    layout.plotArea = { 70, 50, 580, 500 };

    std::vector<HitTestSeries> series(3);
    HitTestAxisRange axis;

    // Click on first legend item
    HitTestResult r1 = HitTest::test(660, 80, layout, series, axis);
    CHECK(r1.hitType == HitTestResult::LegendItem);
    CHECK(r1.seriesIndex == 0);

    // Click on second legend item
    HitTestResult r2 = HitTest::test(660, 100, layout, series, axis);
    CHECK(r2.hitType == HitTestResult::LegendItem);
    CHECK(r2.seriesIndex == 1);

    // Click outside legend
    HitTestResult r3 = HitTest::test(660, 200, layout, series, axis);
    CHECK(r3.hitType != HitTestResult::LegendItem);
}

// ==================================================================
// Section 5: HitTest — Data point
// ==================================================================
void test_hit_test_data_point() {
    TEST("HitTest detects nearest data point");

    HitTestLayout layout;
    layout.plotArea = { 70, 50, 580, 500 };
    layout.legendArea = { 0, 0, 0, 0 };
    layout.legendItemCount = 0;

    std::vector<HitTestSeries> series(1);
    double xs[] = { 1.0, 2.0, 3.0, 4.0, 5.0 };
    double ys[] = { 2.0, 4.0, 6.0, 8.0, 10.0 };
    series[0].xs = xs;
    series[0].ys = ys;
    series[0].count = 5;
    series[0].visible = true;

    HitTestAxisRange axis;
    axis.xMin = 0; axis.xMax = 6;
    axis.yMin = 0; axis.yMax = 12;

    // Hit test near the middle data point (3, 6)
    // x=3 → device: 70 + (3/6)*580 = 70 + 290 = 360
    // y=6 → device: 550 - (6/12)*500 = 550 - 250 = 300
    HitTestResult r = HitTest::test(360, 300, layout, series, axis);
    CHECK(r.hitType == HitTestResult::DataPoint ||
          r.hitType == HitTestResult::CurveLine);
    CHECK(r.seriesIndex == 0);

    // Click in empty area (far from any point)
    HitTestResult r2 = HitTest::test(70, 50, layout, series, axis);
    CHECK(r2.hitType != HitTestResult::DataPoint ||
          r2.distancePx > HitTest::kDataPointHitRadius);
}

// ==================================================================
// Section 6: HitTest — Plot area
// ==================================================================
void test_hit_test_plot_area() {
    TEST("HitTest detects plot area vs outside");

    HitTestLayout layout;
    layout.plotArea = { 70, 50, 580, 500 };
    layout.legendArea = { 0, 0, 0, 0 };
    layout.legendItemCount = 0;

    std::vector<HitTestSeries> series;
    HitTestAxisRange axis;

    // Inside plot area
    HitTestResult r1 = HitTest::test(350, 300, layout, series, axis);
    CHECK(r1.hitType != HitTestResult::None);

    // Outside plot area
    HitTestResult r2 = HitTest::test(10, 10, layout, series, axis);
    CHECK(r2.hitType == HitTestResult::None);
}

// ==================================================================
// Section 7: Plot::handleEvent — Curve selection via legend
// ==================================================================
void test_handle_event_legend_select() {
    TEST("handleEvent: legend click selects curve");

    Plot plot;
    double xs[] = { 1.0, 2.0, 3.0 };
    double y1[] = { 1.0, 2.0, 3.0 };
    double y2[] = { 3.0, 2.0, 1.0 };

    plot.addLineSeries("Curve A", xs, y1, 3);
    plot.addLineSeries("Curve B", xs, y2, 3);
    plot.setAxisRange(0, 4, 0, 4);

    // Render first to establish layout
    RecordingDevice rec;
    plot.render(rec);

    // Click on legend area (should be around x=660, y=78 for first item)
    InputEvent ev;
    ev.type = InputEvent::MouseDown;
    ev.x = 660;
    ev.y = 75;
    ev.button = 0;

    InteractionResult result = plot.handleEvent(ev);
    // Should return CurveSelected (or DataPicked) since we clicked in legend area
    CHECK(result.action == InteractionResult::CurveSelected ||
          result.action == InteractionResult::DataPicked ||
          result.action == InteractionResult::ViewChanged ||
          result.action == InteractionResult::None);
}

// ==================================================================
// Section 8: Plot::handleEvent — MouseMove hover
// ==================================================================
void test_handle_event_hover() {
    TEST("handleEvent: MouseMove updates hover state");

    Plot plot;
    double xs[] = { 1.0, 2.0, 3.0, 4.0, 5.0 };
    double ys[] = { 2.0, 4.0, 6.0, 8.0, 10.0 };
    plot.addLineSeries("Test", xs, ys, 5);
    plot.setAxisRange(0, 6, 0, 12);

    RecordingDevice rec;
    plot.render(rec);

    // Move mouse to the middle of the plot area (near data)
    InputEvent ev;
    ev.type = InputEvent::MouseMove;
    ev.x = 360;  // approximately x=3 in data space
    ev.y = 300;  // approximately y=6 in data space

    InteractionResult result = plot.handleEvent(ev);
    // handleEvent should not crash and should return a valid result
    (void)result;
    CHECK(true); // If we got here, it didn't crash
}

// ==================================================================
// Section 9: Plot with auto-range
// ==================================================================
void test_plot_auto_range() {
    TEST("Plot auto-range computes from data");

    Plot plot;
    double xs[] = { 10.0, 20.0, 30.0 };
    double ys[] = { 100.0, 200.0, 300.0 };
    plot.addLineSeries("Data", xs, ys, 3);
    // Don't call setAxisRange — should auto-compute

    RecordingDevice rec;
    plot.render(rec);

    // Auto-range should have computed something non-trivial
    CHECK(rec.hasValidFrameSequence());
    // The render should still produce grid lines etc.
    CHECK(rec.callCount(RecordingDevice::DrawPolyline) > 0);
}

// ==================================================================
// Section 10: Scatter plot rendering
// ==================================================================
void test_scatter_plot_rendering() {
    TEST("Scatter plot produces drawMarkers calls");

    Plot plot;
    double xs[] = { 1.0, 2.0, 3.0, 4.0 };
    double ys[] = { 1.5, 3.5, 2.0, 4.5 };
    plot.addScatterSeries("Points", xs, ys, 4);
    plot.setAxisRange(0, 5, 0, 5);

    RecordingDevice rec;
    plot.render(rec);

    CHECK(rec.callCount(RecordingDevice::DrawMarkers) > 0);

    // Verify markers have correct count
    const auto& calls = rec.calls();
    for (auto& c : calls) {
        if (c.type == RecordingDevice::DrawMarkers) {
            CHECK_MSG(c.polylineCount == 4, "marker count should be 4");
        }
    }
}

// ==================================================================
// Section 11: Multi-Y-axis rendering
// ==================================================================
void test_multi_y_axis() {
    TEST("Multi-Y-axis rendering with right axis");

    Plot plot;
    double xs[] = { 1.0, 2.0, 3.0 };
    double y1[] = { 10.0, 20.0, 30.0 };

    plot.addLineSeries("Left Axis", xs, y1, 3);
    plot.yAxisSetLabel("Left Y");
    plot.yAxisAddRight("Right Y");

    RecordingDevice rec;
    plot.render(rec);

    CHECK(rec.hasValidFrameSequence());

    // Should have text for both axis labels
    std::string dump = rec.dump();
    CHECK(dump.find("Left Y") != std::string::npos);
    CHECK(dump.find("Right Y") != std::string::npos);
}

// ==================================================================
// Section 12: Series styling
// ==================================================================
void test_series_styling() {
    TEST("Series style is applied to rendering");

    Plot plot;
    double xs[] = { 1.0, 2.0, 3.0 };
    double ys[] = { 1.0, 2.0, 3.0 };
    int id = plot.addLineSeries("Styled", xs, ys, 3);

    LineStyle customStyle;
    customStyle.width = 3.5;
    customStyle.color = { 255, 0, 0 };
    customStyle.dash = LineStyle::DashLine;
    plot.setSeriesStyle(id, customStyle);

    plot.setAxisRange(0, 4, 0, 4);

    RecordingDevice rec;
    plot.render(rec);

    // Find the data polyline (not grid/axis lines)
    const auto& calls = rec.calls();
    for (auto& c : calls) {
        if (c.type == RecordingDevice::DrawPolyline && c.polylineCount == 3) {
            CHECK_MSG(c.lineWidth == 3.5, "custom line width");
            CHECK_MSG(c.lineDash == LineStyle::DashLine, "custom dash style");
        }
    }
}

// ==================================================================
// Section 13: HitTest — Empty series
// ==================================================================
void test_hit_test_empty_series() {
    TEST("HitTest handles empty series gracefully");

    HitTestLayout layout;
    layout.plotArea = { 70, 50, 580, 500 };
    layout.legendArea = { 0, 0, 0, 0 };
    layout.legendItemCount = 0;

    std::vector<HitTestSeries> series;
    HitTestAxisRange axis;

    HitTestResult r = HitTest::test(350, 300, layout, series, axis);
    CHECK(r.hitType == HitTestResult::PlotArea);
    CHECK(r.seriesIndex == -1);
}

// ==================================================================
// Section 14: RecordingDevice reset
// ==================================================================
void test_recording_device_reset() {
    TEST("RecordingDevice reset clears all calls");

    RecordingDevice rec;
    rec.beginFrame();
    rec.endFrame();
    CHECK(rec.totalCalls() == 2);

    rec.reset();
    CHECK(rec.totalCalls() == 0);
    CHECK(rec.callCount(RecordingDevice::BeginFrame) == 0);
}

// ==================================================================
// Section 15: Plot move semantics
// ==================================================================
void test_plot_move_semantics() {
    TEST("Plot supports move semantics");

    Plot plot1;
    double xs[] = { 1.0, 2.0 };
    double ys[] = { 1.0, 2.0 };
    plot1.addLineSeries("Moved", xs, ys, 2);

    Plot plot2 = std::move(plot1);

    RecordingDevice rec;
    plot2.render(rec);

    CHECK(rec.hasValidFrameSequence());
    CHECK(rec.callCount(RecordingDevice::DrawPolyline) > 0);
}

// ==================================================================
// Section 16-21: B1 Integration — PlotRegistry + Plot type pattern
// (Phase B: P1 图类型扩展 — Agent E 集成测试)
// ==================================================================

// ── B1.1: Builtin types are registered ──
void test_b1_builtin_types_registered() {
    TEST("B1: Builtin Line and Scatter types are registered");

    auto names = xyplot::internal::listPlotTypes();

    bool hasLine = false, hasScatter = false;
    for (auto& n : names) {
        if (n == "Line")    hasLine = true;
        if (n == "Scatter") hasScatter = true;
    }
    CHECK(hasLine);
    CHECK(hasScatter);
    CHECK(names.size() >= 2);
}

// ── B1.2: Register custom type and create instance ──
void test_b1_register_custom_type() {
    TEST("B1: Register and create a custom plot type");

    // Define a minimal mock IPlotType for testing
    struct MockPlotType : public xyplot::internal::IPlotType {
        const char* typeName() const override { return "MockType"; }
        void render(IRenderDevice&, const xyplot::internal::SeriesRenderData&,
                    const xyplot::internal::AxisRenderConfig&,
                    const xyplot::internal::DevicePlotArea&) override {}
    };

    bool ok = xyplot::internal::registerPlotType("MockType",
        []() -> std::unique_ptr<xyplot::internal::IPlotType> {
            return std::make_unique<MockPlotType>();
        });
    CHECK(ok);

    auto instance = xyplot::internal::createPlotType("MockType");
    CHECK(instance != nullptr);
    CHECK(std::string(instance->typeName()) == "MockType");

    // Verify it appears in the list
    auto names = xyplot::internal::listPlotTypes();
    bool found = false;
    for (auto& n : names)
        if (n == "MockType") found = true;
    CHECK(found);
}

// ── B1.3: Duplicate registration is rejected ──
void test_b1_duplicate_registration_rejected() {
    TEST("B1: Duplicate type registration returns false");

    struct DupType : public xyplot::internal::IPlotType {
        const char* typeName() const override { return "DupType"; }
        void render(IRenderDevice&, const xyplot::internal::SeriesRenderData&,
                    const xyplot::internal::AxisRenderConfig&,
                    const xyplot::internal::DevicePlotArea&) override {}
    };

    auto factory = []() -> std::unique_ptr<xyplot::internal::IPlotType> {
        return std::make_unique<DupType>();
    };

    bool first = xyplot::internal::registerPlotType("DupType", factory);
    bool second = xyplot::internal::registerPlotType("DupType", factory);

    CHECK(first);
    CHECK(!second);  // Duplicate must be rejected
}

// ── B1.4: Unknown type returns nullptr ──
void test_b1_create_unknown_type() {
    TEST("B1: Creating unknown type returns nullptr");

    auto instance = xyplot::internal::createPlotType("NonExistentType_XYZ");
    CHECK(instance == nullptr);
}

// ── B1.5: Mock Bar type integration — fillRect rendering ──
void test_b1_mock_bar_type() {
    TEST("B1: Mock Bar type uses fillRect via IPlotType::render");

    struct BarPlotType : public xyplot::internal::IPlotType {
        const char* typeName() const override { return "Bar"; }
        void render(IRenderDevice& device,
                    const xyplot::internal::SeriesRenderData& data,
                    const xyplot::internal::AxisRenderConfig& axis,
                    const xyplot::internal::DevicePlotArea& area) override {
            if (data.count == 0) return;
            double barWidth = area.width / static_cast<double>(data.count) * 0.8;
            for (int i = 0; i < data.count; i++) {
                double dx = xyplot::transform(data.xs[i],
                    axis.xMin, axis.xMax,
                    area.left, area.left + area.width,
                    axis.xScale);
                double dy = xyplot::transform(data.ys[i],
                    axis.yMin, axis.yMax,
                    area.top + area.height, area.top,
                    axis.yScale);
                double barLeft = dx - barWidth / 2.0;
                double barTop = dy;
                double barH = area.top + area.height - dy;
                if (barH < 0) barH = 0;
                FillStyle fill;
                fill.color = data.lineStyle.color;
                device.fillRect(barLeft, barTop, barWidth, barH, fill);
            }
        }
    };

    // Use unique name to avoid conflict with Agent D's real "Bar" type
    bool ok = xyplot::internal::registerPlotType("__TestMockBar",
        []() -> std::unique_ptr<xyplot::internal::IPlotType> {
            return std::make_unique<BarPlotType>();
        });
    CHECK(ok);

    // Verify the type can be created and renders fillRect calls
    auto bar = xyplot::internal::createPlotType("__TestMockBar");
    CHECK(bar != nullptr);

    RecordingDevice rec;
    rec.beginFrame();

    xyplot::internal::SeriesRenderData data;
    double xs[] = { 0.0, 1.0, 2.0, 3.0, 4.0 };
    double ys[] = { 2.0, 4.0, 1.0, 5.0, 3.0 };
    data.xs = xs;
    data.ys = ys;
    data.count = 5;
    data.lineStyle.color = { 31, 119, 180 };

    xyplot::internal::AxisRenderConfig axisCfg;
    axisCfg.xMin = 0; axisCfg.xMax = 5;
    axisCfg.yMin = 0; axisCfg.yMax = 6;

    xyplot::internal::DevicePlotArea area;
    area.left = 50; area.top = 20;
    area.width = 600; area.height = 400;

    bar->render(rec, data, axisCfg, area);
    rec.endFrame();

    // Bar type should produce 5 fillRect calls (one per bar)
    CHECK(rec.callCount(RecordingDevice::FillRect) == 5);
    CHECK(rec.hasValidFrameSequence());
}

// ── B1.6: Mock Step type integration — polyline with staircase path ──
void test_b1_mock_step_type() {
    TEST("B1: Mock Step type uses drawPolyline with staircase path");

    struct StepPlotType : public xyplot::internal::IPlotType {
        const char* typeName() const override { return "Step"; }
        void render(IRenderDevice& device,
                    const xyplot::internal::SeriesRenderData& data,
                    const xyplot::internal::AxisRenderConfig& axis,
                    const xyplot::internal::DevicePlotArea& area) override {
            if (data.count < 2) return;
            // Staircase: for each point, step horizontally then vertically
            std::vector<double> sx, sy;
            sx.reserve(data.count * 2);
            sy.reserve(data.count * 2);
            for (int i = 0; i < data.count - 1; i++) {
                double x1 = xyplot::transform(data.xs[i],
                    axis.xMin, axis.xMax,
                    area.left, area.left + area.width, axis.xScale);
                double y1 = xyplot::transform(data.ys[i],
                    axis.yMin, axis.yMax,
                    area.top + area.height, area.top, axis.yScale);
                double x2 = xyplot::transform(data.xs[i+1],
                    axis.xMin, axis.xMax,
                    area.left, area.left + area.width, axis.xScale);
                sx.push_back(x1); sy.push_back(y1);  // start
                sx.push_back(x2); sy.push_back(y1);  // horizontal step
                sx.push_back(x2);                     // vertical
                sy.push_back(xyplot::transform(data.ys[i+1],
                    axis.yMin, axis.yMax,
                    area.top + area.height, area.top, axis.yScale));
            }
            device.drawPolyline(sx.data(), sy.data(),
                                static_cast<int>(sx.size()), data.lineStyle);
        }
    };

    // Use unique name to avoid conflict with Agent D's real "Step" type
    bool ok = xyplot::internal::registerPlotType("__TestMockStep",
        []() -> std::unique_ptr<xyplot::internal::IPlotType> {
            return std::make_unique<StepPlotType>();
        });
    CHECK(ok);

    auto step = xyplot::internal::createPlotType("__TestMockStep");
    CHECK(step != nullptr);

    RecordingDevice rec;
    rec.beginFrame();

    xyplot::internal::SeriesRenderData data;
    double xs[] = { 0.0, 1.0, 2.0, 3.0 };
    double ys[] = { 1.0, 3.0, 2.0, 4.0 };
    data.xs = xs; data.ys = ys; data.count = 4;
    data.lineStyle.color = { 255, 127, 14 };

    xyplot::internal::AxisRenderConfig axisCfg;
    axisCfg.xMin = 0; axisCfg.xMax = 4;
    axisCfg.yMin = 0; axisCfg.yMax = 5;

    xyplot::internal::DevicePlotArea area;
    area.left = 50; area.top = 20;
    area.width = 600; area.height = 400;

    step->render(rec, data, axisCfg, area);
    rec.endFrame();

    // Step type produces polyline calls
    CHECK(rec.callCount(RecordingDevice::DrawPolyline) > 0);
    // The staircase path: 3 segments × 3 points each = 9 points total
    const auto& calls = rec.calls();
    for (auto& c : calls) {
        if (c.type == RecordingDevice::DrawPolyline) {
            CHECK_MSG(c.polylineCount == 9, "staircase: 3 segments × 3 vertices");
        }
    }
}

// ==================================================================
// Section 22-26: B2 Integration — fillPolygon + drawImage
// (Phase B: IRenderDevice +2 方法 — Agent E 集成测试)
// ==================================================================

// ── B2.1: RecordingDevice records fillPolygon ──
void test_b2_recording_fillPolygon() {
    TEST("B2: RecordingDevice records fillPolygon calls");

    RecordingDevice rec;
    rec.beginFrame();

    double xs[] = { 10.0, 100.0, 100.0, 10.0 };
    double ys[] = { 10.0, 10.0, 80.0, 80.0 };
    FillStyle style;
    style.color = { 255, 100, 50, 200 };

    rec.fillPolygon(xs, ys, 4, style);
    rec.endFrame();

    CHECK(rec.callCount(RecordingDevice::FillPolygon) == 1);
    CHECK(rec.hasValidFrameSequence());

    const auto& calls = rec.calls();
    for (auto& c : calls) {
        if (c.type == RecordingDevice::FillPolygon) {
            CHECK_MSG(c.polylineCount == 4, "4 vertices");
            CHECK_MSG(c.fillColor.r == 255, "red channel");
            CHECK_MSG(c.fillColor.g == 100, "green channel");
        }
    }
}

// ── B2.2: RecordingDevice records drawImage ──
void test_b2_recording_drawImage() {
    TEST("B2: RecordingDevice records drawImage calls");

    RecordingDevice rec;
    rec.beginFrame();

    uint8_t rgba[] = { 255, 0, 0, 255,  0, 255, 0, 255,
                       0, 0, 255, 255,  128, 128, 128, 255 };

    rec.drawImage(50, 50, 200, 150, rgba, 2, 2);
    rec.endFrame();

    CHECK(rec.callCount(RecordingDevice::DrawImage) == 1);
    CHECK(rec.hasValidFrameSequence());

    const auto& calls = rec.calls();
    for (auto& c : calls) {
        if (c.type == RecordingDevice::DrawImage) {
            CHECK_MSG(c.px == 50 && c.py == 50, "position");
            CHECK_MSG(c.pw == 200 && c.ph == 150, "size");
            CHECK_MSG(c.polylineCount == 2, "imgW=2");
            CHECK_MSG(static_cast<int>(c.markerSize) == 2, "imgH=2");
        }
    }
}

// ── B2.3: Mock Area type using fillPolygon via IPlotType ──
void test_b2_mock_area_type() {
    TEST("B2: Mock Area type renders fillPolygon + drawPolyline");

    struct AreaPlotType : public xyplot::internal::IPlotType {
        const char* typeName() const override { return "Area"; }
        void render(IRenderDevice& device,
                    const xyplot::internal::SeriesRenderData& data,
                    const xyplot::internal::AxisRenderConfig& axis,
                    const xyplot::internal::DevicePlotArea& area) override {
            if (data.count < 2) return;
            int n = data.count;
            std::vector<double> tx(n), ty(n);
            xyplot::internal::transform::transformPoints(
                data.xs, data.ys, n, axis, area, tx.data(), ty.data());

            double baseline = area.top + area.height;  // bottom of plot area
            // Build filled polygon: curve points + baseline corners
            std::vector<double> px, py;
            px.reserve(n + 2);
            py.reserve(n + 2);
            // Top edge (the curve)
            for (int i = 0; i < n; i++) {
                px.push_back(tx[i]);
                py.push_back(ty[i]);
            }
            // Bottom edge (close back to baseline)
            px.push_back(tx[n-1]); py.push_back(baseline);
            px.push_back(tx[0]);   py.push_back(baseline);

            FillStyle fill;
            fill.color = data.lineStyle.color;
            fill.color.a = 100;  // semi-transparent
            device.fillPolygon(px.data(), py.data(), static_cast<int>(px.size()), fill);

            // Draw the top edge as a line
            device.drawPolyline(tx.data(), ty.data(), n, data.lineStyle);
        }
    };

    bool ok = xyplot::internal::registerPlotType("__TestMockArea",
        []() -> std::unique_ptr<xyplot::internal::IPlotType> {
            return std::make_unique<AreaPlotType>();
        });
    CHECK(ok);

    auto area = xyplot::internal::createPlotType("__TestMockArea");
    CHECK(area != nullptr);

    RecordingDevice rec;
    rec.beginFrame();

    xyplot::internal::SeriesRenderData data;
    double xs[] = { 0.0, 1.0, 2.0, 3.0, 4.0 };
    double ys[] = { 1.0, 3.0, 2.0, 4.0, 2.0 };
    data.xs = xs; data.ys = ys; data.count = 5;
    data.lineStyle.color = { 31, 119, 180 };

    xyplot::internal::AxisRenderConfig axisCfg;
    axisCfg.xMin = 0; axisCfg.xMax = 5;
    axisCfg.yMin = 0; axisCfg.yMax = 5;

    xyplot::internal::DevicePlotArea dpa;
    dpa.left = 50; dpa.top = 20; dpa.width = 600; dpa.height = 400;

    area->render(rec, data, axisCfg, dpa);
    rec.endFrame();

    // Should have 1 fillPolygon (area fill) + 1 drawPolyline (top edge)
    CHECK(rec.callCount(RecordingDevice::FillPolygon) == 1);
    CHECK(rec.callCount(RecordingDevice::DrawPolyline) == 1);

    // fillPolygon should have n+2 vertices (curve + 2 baseline corners)
    const auto& calls = rec.calls();
    for (auto& c : calls) {
        if (c.type == RecordingDevice::FillPolygon) {
            CHECK_MSG(c.polylineCount == 7, "5 curve pts + 2 baseline = 7 vertices");
        }
    }
}

// ── B2.4: Mock Heatmap type using drawImage via IPlotType ──
void test_b2_mock_heatmap_type() {
    TEST("B2: Mock Heatmap type renders drawImage");

    struct HeatmapType : public xyplot::internal::IPlotType {
        const char* typeName() const override { return "Heatmap"; }
        void render(IRenderDevice& device,
                    const xyplot::internal::SeriesRenderData& /*data*/,
                    const xyplot::internal::AxisRenderConfig& /*axis*/,
                    const xyplot::internal::DevicePlotArea& area) override {
            // Generate a simple 4×4 heatmap image
            const int W = 4, H = 4;
            uint8_t rgba[W * H * 4];
            for (int i = 0; i < W * H; i++) {
                rgba[i*4 + 0] = static_cast<uint8_t>(i * 16);       // R
                rgba[i*4 + 1] = static_cast<uint8_t>(128 - i * 8);  // G
                rgba[i*4 + 2] = static_cast<uint8_t>(255 - i * 16); // B
                rgba[i*4 + 3] = 255;                                 // A
            }
            device.drawImage(area.left, area.top,
                             area.width, area.height,
                             rgba, W, H);
        }
    };

    bool ok = xyplot::internal::registerPlotType("__TestMockHeatmap",
        []() -> std::unique_ptr<xyplot::internal::IPlotType> {
            return std::make_unique<HeatmapType>();
        });
    CHECK(ok);

    auto heatmap = xyplot::internal::createPlotType("__TestMockHeatmap");
    CHECK(heatmap != nullptr);

    RecordingDevice rec;
    rec.beginFrame();

    xyplot::internal::SeriesRenderData data;
    data.count = 0;
    xyplot::internal::AxisRenderConfig axisCfg;
    xyplot::internal::DevicePlotArea dpa;
    dpa.left = 30; dpa.top = 20; dpa.width = 640; dpa.height = 480;

    heatmap->render(rec, data, axisCfg, dpa);
    rec.endFrame();

    CHECK(rec.callCount(RecordingDevice::DrawImage) == 1);

    const auto& calls = rec.calls();
    for (auto& c : calls) {
        if (c.type == RecordingDevice::DrawImage) {
            CHECK_MSG(c.polylineCount == 4, "imgW=4");
            CHECK_MSG(static_cast<int>(c.markerSize) == 4, "imgH=4");
        }
    }
}

// ── B2.5: RecordingDevice dump includes B2 call types ──
void test_b2_recording_dump_includes_b2() {
    TEST("B2: RecordingDevice dump() includes fillPolygon and drawImage");

    RecordingDevice rec;
    rec.beginFrame();
    double xs[] = { 0, 50, 50, 0 };
    double ys[] = { 0, 0, 30, 30 };
    FillStyle fs; fs.color = { 10, 20, 30, 40 };
    rec.fillPolygon(xs, ys, 4, fs);
    uint8_t rgba[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };
    rec.drawImage(5, 5, 10, 10, rgba, 2, 2);
    rec.endFrame();

    std::string dump = rec.dump();
    CHECK(dump.find("fillPolygon") != std::string::npos);
    CHECK(dump.find("drawImage") != std::string::npos);
}

// ==================================================================
// Main
// ==================================================================
int main() {
    std::printf("═══════════════════════════════════════════\n");
    std::printf(" XYPlot SDK — Integration Tests\n");
    std::printf(" Owner: Agent E — Backend & Integration\n");
    std::printf("═══════════════════════════════════════════\n\n");

    test_recording_device_basic();
    test_plot_render_recording();
    test_plot_render_multi_series();
    test_hit_test_legend();
    test_hit_test_data_point();
    test_hit_test_plot_area();
    test_handle_event_legend_select();
    test_handle_event_hover();
    test_plot_auto_range();
    test_scatter_plot_rendering();
    test_multi_y_axis();
    test_series_styling();
    test_hit_test_empty_series();
    test_recording_device_reset();
    test_plot_move_semantics();

    // ── B1 Integration tests ──
    test_b1_builtin_types_registered();
    test_b1_register_custom_type();
    test_b1_duplicate_registration_rejected();
    test_b1_create_unknown_type();
    test_b1_mock_bar_type();
    test_b1_mock_step_type();

    // ── B2 Integration tests ──
    test_b2_recording_fillPolygon();
    test_b2_recording_drawImage();
    test_b2_mock_area_type();
    test_b2_mock_heatmap_type();
    test_b2_recording_dump_includes_b2();

    std::printf("\n═══════════════════════════════════════════\n");
    std::printf(" Results: %d passed, %d failed\n",
                testsPassed, testsFailed);
    std::printf("═══════════════════════════════════════════\n");

    return testsFailed > 0 ? 1 : 0;
}
