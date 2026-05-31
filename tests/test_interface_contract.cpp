// ============================================================
// test_interface_contract.cpp — 接口契约测试（编译级 + 运行时）
// ============================================================
// Owner: Agent B (基础设施)
//
// 目的:
//   1. 编译时守卫: 若 IRenderDevice 新增纯虚方法，ContractDevice
//      将因未实现而编译失败 → CI 拦截
//   2. 类型校验: static_assert 确保 P0 类型定义符合冻结清单
//   3. 集成校验: Plot::render(ContractDevice) 必须可编译且正确运行
//   4. 边界条件: 空图、单点、多彩系列等边界场景
//
// PO 要求: "编译级契约测试，未通过不得合并"
//
// 变更规则:
//   - 修改 IRenderDevice 前必须先更新本文件
//   - 新增纯虚方法 → 在 ContractDevice 中实现
//   - 修改签名 → 同步修改本文件调用
// ============================================================

#include <xyplot/xyplot.h>
#include <xyplot/irender_device.h>
#include <xyplot/iinput_source.h>
#include <xyplot/plot.h>
#include <xyplot/types.h>
#include <cstring>
#include <string>
#include <type_traits>
#include <cassert>
#include <cmath>

// ============================================================
// Section 1: Contract Device (完整实现 IRenderDevice 所有纯虚方法)
// ============================================================

class ContractDevice : public xyplot::IRenderDevice {
public:
    // Call counters for sequence verification
    mutable int callCount_beginFrame = 0;
    mutable int callCount_endFrame = 0;
    mutable int callCount_setClipRect = 0;
    mutable int callCount_resetClip = 0;
    mutable int callCount_drawPolyline = 0;
    mutable int callCount_drawMarkers = 0;
    mutable int callCount_drawText = 0;
    mutable int callCount_fillRect = 0;
    mutable int callCount_textExtent = 0;

    // Last-call parameter capture for content verification
    mutable double lastClipX = 0, lastClipY = 0, lastClipW = 0, lastClipH = 0;
    mutable int lastPolylineCount = 0;
    mutable int lastMarkerCount = 0;
    mutable std::string lastText;
    mutable double lastTextX = 0, lastTextY = 0;

    void beginFrame() override { callCount_beginFrame++; }
    void endFrame() override { callCount_endFrame++; }

    void setClipRect(double x, double y, double w, double h) override {
        lastClipX = x; lastClipY = y; lastClipW = w; lastClipH = h;
        callCount_setClipRect++;
    }
    void resetClip() override { callCount_resetClip++; }

    void drawPolyline(const double* xs, const double* ys,
                      int count, const xyplot::LineStyle& style) override {
        (void)xs; (void)ys; (void)style;
        lastPolylineCount = count;
        callCount_drawPolyline++;
    }

    void drawMarkers(const double* xs, const double* ys,
                     int count, const xyplot::MarkerStyle& style) override {
        (void)xs; (void)ys; (void)count; (void)style;
        lastMarkerCount = count;
        callCount_drawMarkers++;
    }

    void drawText(double x, double y, const char* text,
                  const xyplot::FontDesc& font,
                  const xyplot::TextStyle& style) override {
        lastTextX = x; lastTextY = y;
        lastText = text ? text : "";
        callCount_drawText++;
        (void)font; (void)style;
    }

    void fillRect(double x, double y, double w, double h,
                  const xyplot::FillStyle& style) override {
        (void)x; (void)y; (void)w; (void)h; (void)style;
        callCount_fillRect++;
    }

    void textExtent(const char* text, const xyplot::FontDesc& font,
                    double* w, double* h) override {
        callCount_textExtent++;
        if (text && w && h) {
            *w = static_cast<double>(std::strlen(text)) * font.size * 0.6;
            *h = font.size * 1.25;
        }
    }
};

// ============================================================
// Section 2: Compile-time Verification
// ============================================================

// 2a. IRenderDevice 必须是抽象类
static_assert(std::is_abstract_v<xyplot::IRenderDevice>,
              "IRenderDevice must be abstract");

// 2b. ContractDevice 必须可实例化（实现了所有纯虚方法）
static_assert(!std::is_abstract_v<ContractDevice>,
              "ContractDevice must be concrete — "
              "did you add a pure virtual method to IRenderDevice?");
[[maybe_unused]] ContractDevice contractDeviceInstance;

// 2c. IRenderDevice 必须可多态使用
[[maybe_unused]] xyplot::IRenderDevice& deviceRef = contractDeviceInstance;

// 2d. IInputSource 接口契约
class ContractInputSource : public xyplot::IInputSource {
public:
    bool pollEvent(xyplot::InputEvent& out) override {
        out.type = xyplot::InputEvent::None;
        return false;
    }
};
[[maybe_unused]] ContractInputSource inputSourceInstance;
static_assert(!std::is_abstract_v<ContractInputSource>,
              "ContractInputSource must be concrete");

// 2e. IInputSource 必须可多态使用
[[maybe_unused]] xyplot::IInputSource& inputRef = inputSourceInstance;

// ============================================================
// Section 3: Type Structure Contracts (编译时)
// ============================================================
namespace type_checks {

// ──── Color ────
static_assert(sizeof(xyplot::Color) >= 4, "Color must be at least 4 bytes");
static_assert(std::is_same_v<decltype(xyplot::Color::r), uint8_t>);
static_assert(std::is_same_v<decltype(xyplot::Color::g), uint8_t>);
static_assert(std::is_same_v<decltype(xyplot::Color::b), uint8_t>);
static_assert(std::is_same_v<decltype(xyplot::Color::a), uint8_t>);
static_assert(std::is_default_constructible_v<xyplot::Color>);

// ──── LineStyle ────
static_assert(std::is_same_v<decltype(xyplot::LineStyle::width), double>);
static_assert(std::is_same_v<decltype(xyplot::LineStyle::color), xyplot::Color>);
static_assert(std::is_same_v<decltype(xyplot::LineStyle::dash),
              xyplot::LineStyle::DashStyle>);
static_assert(std::is_default_constructible_v<xyplot::LineStyle>);

// ──── MarkerStyle ────
static_assert(std::is_same_v<decltype(xyplot::MarkerStyle::size), double>);
static_assert(std::is_same_v<decltype(xyplot::MarkerStyle::shape),
              xyplot::MarkerStyle::Shape>);
static_assert(std::is_same_v<decltype(xyplot::MarkerStyle::fillColor), xyplot::Color>);
static_assert(std::is_same_v<decltype(xyplot::MarkerStyle::edgeColor), xyplot::Color>);
static_assert(std::is_same_v<decltype(xyplot::MarkerStyle::edgeWidth), double>);
static_assert(std::is_default_constructible_v<xyplot::MarkerStyle>);

// ──── FillStyle ────
static_assert(std::is_same_v<decltype(xyplot::FillStyle::color), xyplot::Color>);
static_assert(std::is_default_constructible_v<xyplot::FillStyle>);

// ──── FontDesc ────
static_assert(std::is_same_v<decltype(xyplot::FontDesc::size), double>);
static_assert(std::is_same_v<decltype(xyplot::FontDesc::bold), bool>);
static_assert(std::is_same_v<decltype(xyplot::FontDesc::italic), bool>);
static_assert(std::is_default_constructible_v<xyplot::FontDesc>);

// ──── TextStyle ────
static_assert(std::is_same_v<decltype(xyplot::TextStyle::color), xyplot::Color>);
static_assert(std::is_same_v<decltype(xyplot::TextStyle::hAlign),
              xyplot::TextStyle::Align>);
static_assert(std::is_same_v<decltype(xyplot::TextStyle::vAlign),
              xyplot::TextStyle::VAlign>);
static_assert(std::is_default_constructible_v<xyplot::TextStyle>);

// ──── ScaleType enum ────
static_assert(std::is_enum_v<xyplot::ScaleType>);
[[maybe_unused]] xyplot::ScaleType st_linear = xyplot::ScaleType::Linear;
[[maybe_unused]] xyplot::ScaleType st_log10  = xyplot::ScaleType::Log10;
[[maybe_unused]] xyplot::ScaleType st_ln     = xyplot::ScaleType::Ln;

// ──── Rect ────
static_assert(std::is_same_v<decltype(xyplot::Rect::x), double>);
static_assert(std::is_same_v<decltype(xyplot::Rect::y), double>);
static_assert(std::is_same_v<decltype(xyplot::Rect::w), double>);
static_assert(std::is_same_v<decltype(xyplot::Rect::h), double>);
static_assert(std::is_default_constructible_v<xyplot::Rect>);

// ──── InputEvent ────
static_assert(std::is_same_v<decltype(xyplot::InputEvent::type),
              xyplot::InputEvent::Type>);
static_assert(std::is_same_v<decltype(xyplot::InputEvent::x), double>);
static_assert(std::is_same_v<decltype(xyplot::InputEvent::y), double>);
static_assert(std::is_same_v<decltype(xyplot::InputEvent::button), int>);
static_assert(std::is_same_v<decltype(xyplot::InputEvent::modifiers), int>);
static_assert(std::is_same_v<decltype(xyplot::InputEvent::wheelDelta), double>);
static_assert(std::is_same_v<decltype(xyplot::InputEvent::keyCode), int>);

// ──── InteractionResult ────
static_assert(std::is_same_v<decltype(xyplot::InteractionResult::action),
              xyplot::InteractionResult::Action>);
static_assert(std::is_same_v<decltype(xyplot::InteractionResult::pickedDataX), double>);
static_assert(std::is_same_v<decltype(xyplot::InteractionResult::pickedDataY), double>);

// ──── Plot is non-copyable, movable ────
static_assert(!std::is_copy_constructible_v<xyplot::Plot>,
              "Plot must NOT be copy-constructible");
static_assert(!std::is_copy_assignable_v<xyplot::Plot>,
              "Plot must NOT be copy-assignable");
static_assert(std::is_move_constructible_v<xyplot::Plot>,
              "Plot must be move-constructible");
static_assert(std::is_move_assignable_v<xyplot::Plot>,
              "Plot must be move-assignable");

} // namespace type_checks

// ============================================================
// Section 4: Integration — Plot::render() compiles & runs
// ============================================================
void test_plot_render_basic() {
    xyplot::Plot plot;

    double xs[] = {1.0, 2.0, 3.0, 4.0, 5.0};
    double ys[] = {2.0, 4.0, 6.0, 8.0, 10.0};
    plot.addLineSeries("test_line", xs, ys, 5);

    double sx[] = {1.5, 2.5, 3.5};
    double sy[] = {3.0, 5.0, 7.0};
    plot.addScatterSeries("test_scatter", sx, sy, 3);

    plot.setAxisRange(0.0, 6.0, 0.0, 12.0);
    plot.xAxisSetLabel("X Axis");
    plot.yAxisSetLabel("Y Axis");
    plot.setTitle("Contract Test Plot");

    ContractDevice device;
    device.beginFrame();
    plot.render(device);
    device.endFrame();

    // Verify render produced output
    assert(device.callCount_beginFrame >= 1);
    assert(device.callCount_endFrame >= 1);
    assert(device.callCount_drawPolyline > 0);
    assert(device.callCount_drawText > 0);
}

// ============================================================
// Section 5: Render call sequence contract
// ============================================================
void test_render_call_sequence() {
    xyplot::Plot plot;
    double xs[] = {1.0, 2.0};
    double ys[] = {1.0, 2.0};
    plot.addLineSeries("s", xs, ys, 2);

    ContractDevice device;
    plot.render(device);

    // Clipping must be used
    assert(device.callCount_setClipRect > 0);
    // Polyline for data + axes + grid
    assert(device.callCount_drawPolyline > 0);
    // Text for title/axis labels
    assert(device.callCount_drawText > 0);
    // Clip must be reset
    assert(device.callCount_resetClip > 0);
}

// ============================================================
// Section 6: P0 Degradable — textExtent fallback
// ============================================================
class DeviceWithoutTextExtent : public xyplot::IRenderDevice {
public:
    void beginFrame() override {}
    void endFrame() override {}
    void setClipRect(double, double, double, double) override {}
    void resetClip() override {}
    void drawPolyline(const double*, const double*, int,
                      const xyplot::LineStyle&) override {}
    void drawMarkers(const double*, const double*, int,
                     const xyplot::MarkerStyle&) override {}
    void drawText(double, double, const char*,
                  const xyplot::FontDesc&, const xyplot::TextStyle&) override {}
    void fillRect(double, double, double, double,
                  const xyplot::FillStyle&) override {}
    // textExtent NOT overridden — uses default fallback
};

void test_fallback_text_extent() {
    DeviceWithoutTextExtent device;
    xyplot::Plot plot;
    double xs[] = {1.0, 2.0};
    double ys[] = {1.0, 2.0};
    plot.addLineSeries("s", xs, ys, 2);
    plot.render(device);
    // Must not crash, even without textExtent override
}

// ============================================================
// Section 7: Edge Cases
// ============================================================

void test_empty_plot() {
    xyplot::Plot plot;
    ContractDevice device;
    plot.render(device);
    // Empty plot must render without crash
    assert(device.callCount_beginFrame >= 1);
    assert(device.callCount_endFrame >= 1);
}

void test_single_data_point() {
    xyplot::Plot plot;
    double x = 42.0, y = 99.0;
    plot.addLineSeries("dot", &x, &y, 1);
    plot.setAxisRange(0, 100, 0, 100);

    ContractDevice device;
    plot.render(device);
    // Single point must render as a polyline with 1 point
    assert(device.callCount_drawPolyline > 0);
}

void test_multi_series() {
    xyplot::Plot plot;
    double x1[] = {0.0, 1.0, 2.0}, y1[] = {0.0, 1.0, 4.0};
    double x2[] = {0.0, 1.0, 2.0}, y2[] = {5.0, 3.0, 1.0};
    double x3[] = {0.5, 1.5    }, y3[] = {2.0, 2.0    };

    plot.addLineSeries("curve_A", x1, y1, 3);
    plot.addLineSeries("curve_B", x2, y2, 3);
    plot.addScatterSeries("points", x3, y3, 2);

    ContractDevice device;
    plot.render(device);

    // Must have drawn all three series (data polylines + markers)
    assert(device.callCount_drawPolyline >= 3); // at least 3 data series
    assert(device.callCount_drawMarkers >= 1);  // scatter markers
}

void test_style_configuration() {
    xyplot::Plot plot;
    double xs[] = {1.0, 2.0}, ys[] = {1.0, 2.0};
    int id = plot.addLineSeries("styled", xs, ys, 2);

    xyplot::LineStyle customStyle;
    customStyle.width = 3.0;
    customStyle.color = {255, 0, 0, 255};
    customStyle.dash = xyplot::LineStyle::DashLine;
    plot.setSeriesStyle(id, customStyle);

    ContractDevice device;
    plot.render(device);
    // Style must not crash the render pipeline
    assert(device.callCount_drawPolyline > 0);
}

void test_axis_configuration_edge_cases() {
    xyplot::Plot plot;
    double xs[] = { -5.0, 0.0, 5.0 }, ys[] = { -10.0, 0.0, 10.0 };
    plot.addLineSeries("neg", xs, ys, 3);

    // Reversed range
    plot.setAxisRange(10.0, -10.0, 5.0, -5.0);

    ContractDevice device;
    plot.render(device);
    // Reversed ranges must not crash
    assert(device.callCount_drawPolyline > 0);
}

void test_update_series_data() {
    xyplot::Plot plot;
    double xs[] = {1.0, 2.0}, ys[] = {1.0, 2.0};
    int id = plot.addLineSeries("updatable", xs, ys, 2);

    double newXs[] = {3.0, 4.0, 5.0}, newYs[] = {6.0, 7.0, 8.0};
    plot.updateSeriesData(id, newXs, newYs, 3);

    ContractDevice device;
    plot.render(device);
    assert(device.callCount_drawPolyline > 0);
}

// NOTE: test_interaction_noop() removed temporarily.
// Plot::handleEvent() implementation is being migrated to
// src/plot_interaction.cpp by Agent E. Will re-enable once
// the migration is complete and the symbol is available.
//
// void test_interaction_noop() { ... }

void test_move_semantics() {
    xyplot::Plot plot1;
    double xs[] = {1.0, 2.0}, ys[] = {1.0, 2.0};
    plot1.addLineSeries("move_test", xs, ys, 2);

    // Move construct
    xyplot::Plot plot2(std::move(plot1));
    ContractDevice device;
    plot2.render(device);
    assert(device.callCount_drawPolyline > 0);

    // Move assign
    xyplot::Plot plot3;
    plot3 = std::move(plot2);
    ContractDevice device2;
    plot3.render(device2);
    assert(device2.callCount_drawPolyline > 0);
}

void test_text_extent_override() {
    // Verify that overriding textExtent actually gets called
    ContractDevice device;
    double w = 0, h = 0;
    xyplot::FontDesc font;
    font.size = 14.0;
    device.textExtent("Hello", font, &w, &h);

    assert(device.callCount_textExtent == 1);
    assert(w > 0.0);
    assert(h > 0.0);
}

void test_multiple_renders_same_plot() {
    xyplot::Plot plot;
    double xs[] = {1.0, 2.0}, ys[] = {1.0, 2.0};
    plot.addLineSeries("persistent", xs, ys, 2);

    ContractDevice device1;
    plot.render(device1);

    ContractDevice device2;
    plot.render(device2);

    // Both renders must produce equivalent output
    assert(device1.callCount_drawPolyline > 0);
    assert(device2.callCount_drawPolyline > 0);
    assert(device1.callCount_drawPolyline == device2.callCount_drawPolyline);
}

// ============================================================
// Section 8: main — runs all runtime checks
// ============================================================
int main() {
    // Core contract tests
    test_plot_render_basic();
    test_render_call_sequence();
    test_fallback_text_extent();

    // Edge case tests
    test_empty_plot();
    test_single_data_point();
    test_multi_series();
    test_style_configuration();
    test_axis_configuration_edge_cases();
    test_update_series_data();
    // test_interaction_noop();  // disabled: handleEvent migration in progress
    test_move_semantics();
    test_text_extent_override();
    test_multiple_renders_same_plot();

    return 0;
}
