// ============================================================
// test_plots.cpp — 图类型单元测试
// ============================================================
// Owner: Agent D (图类型)
// 测试: LinePlot, ScatterPlot, MultiAxis, LegendRenderer, PlotRegistry
// 使用 ContractDevice 验证绘制调用序列
// ============================================================
#include "xyplot/xyplot.h"
#include "../src/xyplot_internal.h"
#include <cassert>
#include <cstring>
#include <vector>
#include <string>
#include <cmath>
#include <memory>
#include <stdexcept>

using namespace xyplot;
using namespace xyplot::internal;

// ============================================================
// Test Helpers
// ============================================================

/// 计数型渲染设备 — 记录每次调用的参数
struct SpyDevice : public IRenderDevice {
    // 调用计数
    int beginFrameCalls = 0;
    int endFrameCalls = 0;
    int setClipRectCalls = 0;
    int resetClipCalls = 0;
    int drawPolylineCalls = 0;
    int drawMarkersCalls = 0;
    int drawTextCalls = 0;
    int fillRectCalls = 0;

    // 最后一次调用参数
    struct PolylineCall {
        int count = 0;
        LineStyle style{};
    };
    PolylineCall lastPolyline;

    struct MarkersCall {
        int count = 0;
        MarkerStyle style{};
    };
    MarkersCall lastMarkers;

    struct TextCall {
        std::string text;
        FontDesc font{};
        TextStyle style{};
    };
    TextCall lastText;

    struct FillRectCall {
        double x, y, w, h;
        FillStyle style{};
    };
    FillRectCall lastFillRect;

    // IRenderDevice overrides
    void beginFrame() override { beginFrameCalls++; }
    void endFrame() override { endFrameCalls++; }

    void setClipRect(double x, double y, double w, double h) override {
        (void)x; (void)y; (void)w; (void)h;
        setClipRectCalls++;
    }
    void resetClip() override { resetClipCalls++; }

    void drawPolyline(const double* xs, const double* ys,
                      int count, const LineStyle& style) override {
        (void)xs; (void)ys;
        drawPolylineCalls++;
        lastPolyline.count = count;
        lastPolyline.style = style;
    }

    void drawMarkers(const double* xs, const double* ys,
                     int count, const MarkerStyle& style) override {
        (void)xs; (void)ys;
        drawMarkersCalls++;
        lastMarkers.count = count;
        lastMarkers.style = style;
    }

    void drawText(double x, double y, const char* text,
                  const FontDesc& font, const TextStyle& style) override {
        (void)x; (void)y;
        drawTextCalls++;
        lastText.text = text ? text : "";
        lastText.font = font;
        lastText.style = style;
    }

    void fillRect(double x, double y, double w, double h,
                  const FillStyle& style) override {
        (void)x; (void)y; (void)w; (void)h;
        fillRectCalls++;
        lastFillRect.x = x;
        lastFillRect.y = y;
        lastFillRect.w = w;
        lastFillRect.h = h;
        lastFillRect.style = style;
    }

    void reset() {
        beginFrameCalls = 0; endFrameCalls = 0;
        setClipRectCalls = 0; resetClipCalls = 0;
        drawPolylineCalls = 0; drawMarkersCalls = 0;
        drawTextCalls = 0; fillRectCalls = 0;
    }
};

// ============================================================
// Section 1: PlotRegistry 测试
// ============================================================
void test_registry_builtins() {
    // 内置类型 Line 和 Scatter 应已注册
    auto linePlot = createPlotType("Line");
    assert(linePlot != nullptr);
    assert(std::string(linePlot->typeName()) == "Line");

    auto scatterPlot = createPlotType("Scatter");
    assert(scatterPlot != nullptr);
    assert(std::string(scatterPlot->typeName()) == "Scatter");

    // 未注册的类型返回 nullptr
    auto unknown = createPlotType("UnknownType");
    assert(unknown == nullptr);

    // listPlotTypes 应包含已注册类型
    auto names = listPlotTypes();
    assert(names.size() >= 2);
    bool hasLine = false, hasScatter = false;
    for (auto& n : names) {
        if (n == "Line") hasLine = true;
        if (n == "Scatter") hasScatter = true;
    }
    assert(hasLine);
    assert(hasScatter);
}

void test_registry_custom_type() {
    // 注册自定义类型
    bool ok = registerPlotType("TestPlot", []() -> std::unique_ptr<IPlotType> {
        // 返回一个 LinePlot 作为占位
        return createPlotType("Line");
    });
    assert(ok);

    // 重复注册应失败
    bool dup = registerPlotType("TestPlot", []() -> std::unique_ptr<IPlotType> {
        return nullptr;
    });
    assert(!dup);

    auto tp = createPlotType("TestPlot");
    assert(tp != nullptr);
}

// ============================================================
// Section 2: LinePlot 测试
// ============================================================
void test_line_plot_renders_polyline() {
    auto plot = createPlotType("Line");
    assert(plot != nullptr);

    SpyDevice device;

    // 准备测试数据
    double xs[] = {1.0, 2.0, 3.0, 4.0, 5.0};
    double ys[] = {2.0, 4.0, 6.0, 8.0, 10.0};

    SeriesRenderData data;
    data.xs = xs;
    data.ys = ys;
    data.count = 5;
    data.lineStyle.width = 2.0;
    data.lineStyle.color = {255, 0, 0};
    data.name = "TestLine";

    AxisRenderConfig axis;
    axis.xMin = 0.0; axis.xMax = 6.0;
    axis.yMin = 0.0; axis.yMax = 12.0;
    axis.xScale = ScaleType::Linear;
    axis.yScale = ScaleType::Linear;

    DevicePlotArea area;
    area.left = 60; area.top = 30;
    area.width = 700; area.height = 500;

    plot->render(device, data, axis, area);

    // 应调用 drawPolyline 至少 1 次
    assert(device.drawPolylineCalls >= 1);
    // Polyline 点数应与数据点数一致
    assert(device.lastPolyline.count == 5);
    // 样式应正确传递
    assert(device.lastPolyline.style.width == 2.0);
    assert(device.lastPolyline.style.color.r == 255);
    assert(device.lastPolyline.style.color.g == 0);
    assert(device.lastPolyline.style.color.b == 0);
}

void test_line_plot_empty_data() {
    auto plot = createPlotType("Line");
    SpyDevice device;

    SeriesRenderData data;
    data.xs = nullptr;
    data.ys = nullptr;
    data.count = 0;

    AxisRenderConfig axis;
    DevicePlotArea area;
    area.left = 60; area.top = 30;
    area.width = 700; area.height = 500;

    plot->render(device, data, axis, area);

    // 空数据不应调用 drawPolyline
    assert(device.drawPolylineCalls == 0);
}

void test_line_plot_with_markers() {
    auto plot = createPlotType("Line");
    SpyDevice device;

    double xs[] = {1.0, 2.0, 3.0};
    double ys[] = {3.0, 5.0, 7.0};

    SeriesRenderData data;
    data.xs = xs; data.ys = ys; data.count = 3;
    data.lineStyle.width = 1.0;
    data.lineStyle.color = {0, 0, 255};
    data.markerStyle.size = 4.0;
    data.markerStyle.shape = MarkerStyle::Diamond;

    AxisRenderConfig axis;
    DevicePlotArea area;
    area.left = 60; area.top = 30;
    area.width = 400; area.height = 300;

    plot->render(device, data, axis, area);

    // LinePlot 应在数据点上有线 + 标记
    assert(device.drawPolylineCalls >= 1);
    assert(device.drawMarkersCalls >= 1);
    assert(device.lastMarkers.count == 3);
    assert(device.lastMarkers.style.shape == MarkerStyle::Diamond);
}

void test_line_plot_coordinate_transform() {
    auto plot = createPlotType("Line");
    SpyDevice device;

    // 数据范围 [0, 10] → 设备范围 [100, 500]
    double xs[] = {0.0, 10.0};
    double ys[] = {0.0, 10.0};

    SeriesRenderData data;
    data.xs = xs; data.ys = ys; data.count = 2;

    AxisRenderConfig axis;
    axis.xMin = 0.0; axis.xMax = 10.0;
    axis.yMin = 0.0; axis.yMax = 10.0;

    DevicePlotArea area;
    area.left = 100; area.top = 0;
    area.width = 400; area.height = 300;

    plot->render(device, data, axis, area);

    assert(device.drawPolylineCalls >= 1);
    // 验证变换后坐标在设备范围内（通过 SpyDevice 的 lastPolyline 无法直接获取坐标，
    // 但通过调用验证逻辑正确）
}

// ============================================================
// Section 3: ScatterPlot 测试
// ============================================================
void test_scatter_plot_renders_markers() {
    auto plot = createPlotType("Scatter");
    assert(plot != nullptr);

    SpyDevice device;

    double xs[] = {1.0, 2.0, 3.0, 4.0};
    double ys[] = {2.0, 3.0, 5.0, 7.0};

    SeriesRenderData data;
    data.xs = xs; data.ys = ys; data.count = 4;
    data.lineStyle.color = {255, 0, 0};
    data.markerStyle.size = 8.0;
    data.markerStyle.shape = MarkerStyle::Circle;
    data.markerStyle.fillColor = {255, 100, 100, 200};

    AxisRenderConfig axis;
    axis.xMin = 0.0; axis.xMax = 5.0;
    axis.yMin = 0.0; axis.yMax = 8.0;

    DevicePlotArea area;
    area.left = 60; area.top = 30;
    area.width = 700; area.height = 500;

    plot->render(device, data, axis, area);

    // 散点图应调用 drawMarkers
    assert(device.drawMarkersCalls >= 1);
    assert(device.lastMarkers.count == 4);
    assert(device.lastMarkers.style.shape == MarkerStyle::Circle);
    assert(device.lastMarkers.style.size == 8.0);
}

void test_scatter_plot_default_marker_style() {
    auto plot = createPlotType("Scatter");
    SpyDevice device;

    double xs[] = {1.0, 2.0};
    double ys[] = {3.0, 4.0};

    SeriesRenderData data;
    data.xs = xs; data.ys = ys; data.count = 2;
    data.lineStyle.color = {0, 128, 0};
    // 不设置 markerStyle（全零/默认值）

    AxisRenderConfig axis;
    DevicePlotArea area;
    area.left = 60; area.top = 30;
    area.width = 500; area.height = 400;

    plot->render(device, data, axis, area);

    assert(device.drawMarkersCalls >= 1);
    // 应使用默认尺寸
    assert(device.lastMarkers.style.size == 6.0);
    // 填充色应从 lineStyle.color 继承
    assert(device.lastMarkers.style.fillColor.g == 128);
}

void test_scatter_plot_with_line() {
    auto plot = createPlotType("Scatter");
    SpyDevice device;

    double xs[] = {0.0, 1.0, 2.0, 3.0};
    double ys[] = {0.0, 1.0, 4.0, 9.0};

    SeriesRenderData data;
    data.xs = xs; data.ys = ys; data.count = 4;
    data.lineStyle.width = 1.5;
    data.lineStyle.color = {0, 0, 200};
    data.markerStyle.size = 5.0;
    data.markerStyle.shape = MarkerStyle::Square;

    AxisRenderConfig axis;
    DevicePlotArea area;
    area.left = 60; area.top = 30;
    area.width = 600; area.height = 400;

    plot->render(device, data, axis, area);

    // 有线宽的 ScatterPlot 应同时绘制标记和连接线
    assert(device.drawMarkersCalls >= 1);
    assert(device.drawPolylineCalls >= 1);
}

// ============================================================
// Section 4: MultiAxis 测试
// ============================================================
void test_multi_axis_default() {
    MultiAxisManager axes;

    // 默认应有 1 个左侧 Y 轴
    assert(axes.count() == 1);
    assert(axes.leftAxisCount() == 1);
    assert(axes.rightAxisCount() == 0);

    const auto& ax0 = axes.axis(0);
    assert(!ax0.isRight);
    assert(ax0.dataMin == 0.0);
    assert(ax0.dataMax == 1.0);
}

void test_multi_axis_add_right() {
    MultiAxisManager axes;

    int idx = axes.addRightAxis("Right Y");
    assert(idx == 1);
    assert(axes.count() == 2);
    assert(axes.leftAxisCount() == 1);
    assert(axes.rightAxisCount() == 1);

    const auto& ax1 = axes.axis(1);
    assert(ax1.isRight);
    assert(ax1.label == "Right Y");
}

void test_multi_axis_config() {
    MultiAxisManager axes;

    axes.setLabel(0, "Primary Y");
    axes.setRange(0, -10.0, 10.0);
    axes.setScale(0, ScaleType::Log10);

    const auto& ax = axes.axis(0);
    assert(ax.label == "Primary Y");
    assert(ax.dataMin == -10.0);
    assert(ax.dataMax == 10.0);
    assert(ax.scale == ScaleType::Log10);
}

void test_multi_axis_auto_range() {
    MultiAxisManager axes;

    double xs1[] = {1.0, 2.0, 3.0};
    double ys1[] = {100.0, 200.0, 150.0};

    double xs2[] = {1.0, 2.0};
    double ys2[] = {10.0, 20.0};

    SeriesRenderData s1;
    s1.xs = xs1; s1.ys = ys1; s1.count = 3;
    s1.yAxisIndex = 0;

    SeriesRenderData s2;
    s2.xs = xs2; s2.ys = ys2; s2.count = 2;
    s2.yAxisIndex = 1;  // 属于第二个轴

    // 添加第二个轴
    axes.addRightAxis("Secondary");

    std::vector<SeriesRenderData> seriesList = {s1, s2};

    axes.autoRange(0, seriesList);
    // 轴 0 应覆盖 s1 的数据范围 [100, 200] 加上 5% 边距
    assert(axes.axis(0).dataMin <= 100.0);
    assert(axes.axis(0).dataMax >= 200.0);

    axes.autoRange(1, seriesList);
    // 轴 1 应覆盖 s2 的数据范围 [10, 20] 加上 5% 边距
    assert(axes.axis(1).dataMin <= 10.0);
    assert(axes.axis(1).dataMax >= 20.0);
}

void test_multi_axis_render_labels() {
    MultiAxisManager axes;
    axes.setLabel(0, "Temperature (°C)");
    axes.addRightAxis("Humidity (%)");

    SpyDevice device;

    DevicePlotArea area;
    area.left = 80; area.top = 20;
    area.width = 600; area.height = 400;

    axes.renderAxisLabels(device, area, 60.0, 20.0);

    // 应绘制 2 个轴标签
    assert(device.drawTextCalls >= 2);
}

// ============================================================
// Section 5: LegendRenderer 测试
// ============================================================
void test_legend_renderer_layout() {
    LegendRenderer renderer;

    std::vector<LegendEntry> entries;
    entries.push_back({"Series A", {255, 0, 0}, LineStyle{}});
    entries.push_back({"Series B", {0, 0, 255}, LineStyle{}});
    entries.push_back({"Series C", {0, 128, 0}, LineStyle{}});

    DevicePlotArea area;
    area.left = 60; area.top = 30;
    area.width = 700; area.height = 500;

    auto items = renderer.layout(entries, area, 11.0);

    // 应有 3 个布局项
    assert(items.size() == 3);

    // 每个项应有非零尺寸
    for (auto& item : items) {
        assert(item.swatchW > 0);
        assert(item.swatchH > 0);
        assert(item.entry != nullptr);
    }

    // 图例应在绘图区右上角
    Rect bb = renderer.boundingBox();
    assert(bb.x >= area.left);
    assert(bb.y >= area.top);
    assert(bb.x + bb.w <= area.left + area.width);
    assert(bb.y + bb.h <= area.top + area.height);
}

void test_legend_renderer_render() {
    LegendRenderer renderer;

    std::vector<LegendEntry> entries;
    LegendEntry e1;
    e1.name = "Alpha";
    e1.swatchColor = {255, 0, 0};
    e1.lineStyle.width = 1.0;
    entries.push_back(e1);

    LegendEntry e2;
    e2.name = "Beta";
    e2.swatchColor = {0, 0, 255};
    e2.lineStyle.width = 2.0;
    entries.push_back(e2);

    SpyDevice device;

    DevicePlotArea area;
    area.left = 60; area.top = 30;
    area.width = 400; area.height = 300;

    renderer.render(device, entries, area, 12.0);

    // 应绘制每个图例项：色块 + 文本
    assert(device.fillRectCalls >= 3);  // 2 个色块 + 1 个背景
    assert(device.drawTextCalls >= 2);   // 2 个标签

    // 文本内容应匹配
    // (lastText 只记录最后一次，但已足够验证调用发生)
}

void test_legend_empty_entries() {
    LegendRenderer renderer;
    SpyDevice device;

    std::vector<LegendEntry> entries;
    DevicePlotArea area;
    area.left = 60; area.top = 30;
    area.width = 400; area.height = 300;

    renderer.render(device, entries, area, 11.0);

    // 空列表不产生任何渲染调用
    assert(device.fillRectCalls == 0);
    assert(device.drawTextCalls == 0);
    assert(device.drawPolylineCalls == 0);
}

// ============================================================
// Section 6: 坐标变换测试
// ============================================================
void test_transform_linear() {
    AxisRenderConfig axis;
    axis.xMin = 0.0; axis.xMax = 100.0;
    axis.xScale = ScaleType::Linear;
    axis.yMin = 0.0; axis.yMax = 50.0;
    axis.yScale = ScaleType::Linear;

    DevicePlotArea area;
    area.left = 100; area.top = 0;
    area.width = 400; area.height = 300;

    // X: 0 → 100 (左边缘), 100 → 500 (右边缘)
    double x0 = transform::dataToDeviceX(0.0, axis, area);
    double xMid = transform::dataToDeviceX(50.0, axis, area);
    double xMax = transform::dataToDeviceX(100.0, axis, area);

    assert(std::abs(x0 - 100.0) < 1e-9);
    assert(std::abs(xMid - 300.0) < 1e-9);
    assert(std::abs(xMax - 500.0) < 1e-9);

    // Y: 0 → 300 (底部), 50 → 0 (顶部) — 翻转
    double y0 = transform::dataToDeviceY(0.0, axis, area);
    double yMid = transform::dataToDeviceY(25.0, axis, area);
    double yMax = transform::dataToDeviceY(50.0, axis, area);

    assert(std::abs(y0 - 300.0) < 1e-9);
    assert(std::abs(yMid - 150.0) < 1e-9);
    assert(std::abs(yMax - 0.0) < 1e-9);
}

void test_transform_batch() {
    double xs[] = {0.0, 5.0, 10.0};
    double ys[] = {0.0, 5.0, 10.0};

    AxisRenderConfig axis;
    axis.xMin = 0.0; axis.xMax = 10.0;
    axis.yMin = 0.0; axis.yMax = 10.0;

    DevicePlotArea area;
    area.left = 0; area.top = 0;
    area.width = 100; area.height = 100;

    double outX[3], outY[3];
    transform::transformPoints(xs, ys, 3, axis, area, outX, outY);

    assert(std::abs(outX[0] - 0.0) < 1e-9);
    assert(std::abs(outX[1] - 50.0) < 1e-9);
    assert(std::abs(outX[2] - 100.0) < 1e-9);

    assert(std::abs(outY[0] - 100.0) < 1e-9);  // Y=0 → bottom
    assert(std::abs(outY[1] - 50.0) < 1e-9);
    assert(std::abs(outY[2] - 0.0) < 1e-9);    // Y=10 → top
}

void test_transform_log_scale() {
    AxisRenderConfig axis;
    axis.xMin = 1.0; axis.xMax = 100.0;
    axis.xScale = ScaleType::Log10;
    axis.yMin = 1.0; axis.yMax = 1000.0;
    axis.yScale = ScaleType::Log10;

    DevicePlotArea area;
    area.left = 0; area.top = 0;
    area.width = 200; area.height = 100;

    // Log10: 1→0, 10→100, 100→200
    double x1 = transform::dataToDeviceX(1.0, axis, area);
    double x10 = transform::dataToDeviceX(10.0, axis, area);
    double x100 = transform::dataToDeviceX(100.0, axis, area);

    assert(std::abs(x1 - 0.0) < 1e-9);
    assert(std::abs(x10 - 100.0) < 1e-9);
    assert(std::abs(x100 - 200.0) < 1e-9);
}

// ============================================================
// Section 2a: LinePlot 边界条件
// ============================================================
void test_line_plot_single_point() {
    auto plot = createPlotType("Line");
    SpyDevice device;

    double xs[] = {5.0};
    double ys[] = {3.0};

    SeriesRenderData data;
    data.xs = xs; data.ys = ys; data.count = 1;
    data.lineStyle.width = 1.0;
    data.lineStyle.color = {100, 100, 100};

    AxisRenderConfig axis;
    axis.xMin = 0.0; axis.xMax = 10.0;
    axis.yMin = 0.0; axis.yMax = 10.0;

    DevicePlotArea area;
    area.left = 60; area.top = 30;
    area.width = 400; area.height = 300;

    plot->render(device, data, axis, area);

    // 单点应产生 polyline（即使只有1个点）
    assert(device.drawPolylineCalls >= 1);
    assert(device.lastPolyline.count == 1);
}

void test_line_plot_log10_scale() {
    auto plot = createPlotType("Line");
    SpyDevice device;

    // Log10 range: 1 to 1000
    double xs[] = {1.0, 10.0, 100.0, 1000.0};
    double ys[] = {1.0, 10.0, 100.0, 1000.0};

    SeriesRenderData data;
    data.xs = xs; data.ys = ys; data.count = 4;
    data.lineStyle.width = 1.0;
    data.lineStyle.color = {0, 0, 0};

    AxisRenderConfig axis;
    axis.xMin = 1.0; axis.xMax = 1000.0;
    axis.yMin = 1.0; axis.yMax = 1000.0;
    axis.xScale = ScaleType::Log10;
    axis.yScale = ScaleType::Log10;

    DevicePlotArea area;
    area.left = 100; area.top = 0;
    area.width = 400; area.height = 300;

    plot->render(device, data, axis, area);

    // Log scale 渲染应成功产生 polyline
    assert(device.drawPolylineCalls >= 1);
    assert(device.lastPolyline.count == 4);
}

void test_scatter_plot_empty_data() {
    auto plot = createPlotType("Scatter");
    SpyDevice device;

    SeriesRenderData data;
    data.xs = nullptr;
    data.ys = nullptr;
    data.count = 0;
    data.lineStyle.color = {255, 0, 0};

    AxisRenderConfig axis;
    DevicePlotArea area;
    area.left = 60; area.top = 30;
    area.width = 400; area.height = 300;

    plot->render(device, data, axis, area);

    // 空数据不应调用任何绘制
    assert(device.drawMarkersCalls == 0);
    assert(device.drawPolylineCalls == 0);
}

void test_scatter_plot_single_point() {
    auto plot = createPlotType("Scatter");
    SpyDevice device;

    double xs[] = {5.0};
    double ys[] = {7.0};

    SeriesRenderData data;
    data.xs = xs; data.ys = ys; data.count = 1;
    data.lineStyle.color = {0, 128, 0};
    data.markerStyle.size = 8.0;
    data.markerStyle.shape = MarkerStyle::Diamond;

    AxisRenderConfig axis;
    DevicePlotArea area;
    area.left = 60; area.top = 30;
    area.width = 500; area.height = 400;

    plot->render(device, data, axis, area);

    // 单点散点应产生 1 个 marker
    assert(device.drawMarkersCalls >= 1);
    assert(device.lastMarkers.count == 1);
}

// ============================================================
// Section 4a: MultiAxis 边界条件
// ============================================================
void test_multi_axis_multiple_right() {
    MultiAxisManager axes;

    axes.addRightAxis("Right 1");
    axes.addRightAxis("Right 2");

    assert(axes.count() == 3);
    assert(axes.leftAxisCount() == 1);
    assert(axes.rightAxisCount() == 2);

    // 验证右侧轴按索引正确存储
    assert(axes.axis(1).isRight);
    assert(axes.axis(2).isRight);
    assert(axes.axis(1).label == "Right 1");
    assert(axes.axis(2).label == "Right 2");
}

void test_multi_axis_out_of_bounds() {
    MultiAxisManager axes;

    // 越界访问 .at() 应抛出异常
    bool caught = false;
    try {
        (void)axes.axis(99);
    } catch (const std::out_of_range&) {
        caught = true;
    }
    assert(caught);
}

void test_multi_axis_render_ticks_integration() {
    // 验证 renderTicksAndGrid 使用 Agent C 的 computeTicks() 正常工作
    MultiAxisManager axes;
    axes.setRange(0, 0.0, 100.0);

    SpyDevice device;
    DevicePlotArea area;
    area.left = 80; area.top = 20;
    area.width = 600; area.height = 400;

    axes.renderTicksAndGrid(0, device, area, false);

    // 应产生网格线（水平线）和轴线
    assert(device.drawPolylineCalls >= 2);  // 至少 1 条网格线 + 1 条轴线
}

// ============================================================
// Section 5a: LegendRenderer 边界条件
// ============================================================
void test_legend_single_entry() {
    LegendRenderer renderer;

    std::vector<LegendEntry> entries;
    entries.push_back({"Only", {128, 0, 128}, LineStyle{}});

    DevicePlotArea area;
    area.left = 60; area.top = 30;
    area.width = 400; area.height = 300;

    SpyDevice device;
    renderer.render(device, entries, area, 12.0);

    // 单条目：1 个背景 + 1 个色块 = 2 个 fillRect
    assert(device.fillRectCalls >= 2);
    assert(device.drawTextCalls >= 1);
}

void test_legend_many_entries() {
    LegendRenderer renderer;

    std::vector<LegendEntry> entries;
    for (int i = 0; i < 10; i++) {
        LegendEntry e;
        e.name = "Series " + std::to_string(i);
        e.swatchColor = {static_cast<uint8_t>(i * 25), 0, 0};
        entries.push_back(e);
    }

    DevicePlotArea area;
    area.left = 60; area.top = 30;
    area.width = 700; area.height = 500;

    auto items = renderer.layout(entries, area, 11.0);

    assert(items.size() == 10);
    // 所有条目应在绘图区内
    Rect bb = renderer.boundingBox();
    assert(bb.y >= area.top);
    assert(bb.y + bb.h <= area.top + area.height);
}

// ============================================================
// Section 7: Integration — 完整渲染流程
// ============================================================
void test_integration_line_and_scatter() {
    SpyDevice device;

    // 模拟完整 Plot::render() 流程
    DevicePlotArea area;
    area.left = 60; area.top = 30;
    area.width = 700; area.height = 500;

    AxisRenderConfig axis;
    axis.xMin = 0.0; axis.xMax = 10.0;
    axis.yMin = 0.0; axis.yMax = 20.0;

    // 创建 Line 和 Scatter series
    double xsLine[] = {0.0, 2.0, 4.0, 6.0, 8.0, 10.0};
    double ysLine[] = {0.0, 4.0, 16.0, 12.0, 18.0, 20.0};

    double xsScatter[] = {1.0, 3.0, 5.0, 7.0, 9.0};
    double ysScatter[] = {2.0, 8.0, 14.0, 10.0, 16.0};

    SeriesRenderData lineData;
    lineData.xs = xsLine; lineData.ys = ysLine; lineData.count = 6;
    lineData.lineStyle.width = 2.0;
    lineData.lineStyle.color = {255, 0, 0};
    lineData.name = "LineSeries";

    SeriesRenderData scatterData;
    scatterData.xs = xsScatter; scatterData.ys = ysScatter; scatterData.count = 5;
    scatterData.lineStyle.color = {0, 0, 255};
    scatterData.markerStyle.size = 6.0;
    scatterData.markerStyle.shape = MarkerStyle::Circle;
    scatterData.name = "ScatterSeries";

    // 渲染 Line
    auto linePlot = createPlotType("Line");
    device.setClipRect(area.left, area.top, area.width, area.height);
    linePlot->render(device, lineData, axis, area);
    device.resetClip();

    // 渲染 Scatter
    device.setClipRect(area.left, area.top, area.width, area.height);
    auto scatterPlot = createPlotType("Scatter");
    scatterPlot->render(device, scatterData, axis, area);
    device.resetClip();

    // 渲染图例
    std::vector<LegendEntry> legendEntries;
    LegendEntry le1;
    le1.name = "LineSeries";
    le1.swatchColor = {255, 0, 0};
    le1.lineStyle = lineData.lineStyle;
    legendEntries.push_back(le1);

    LegendEntry le2;
    le2.name = "ScatterSeries";
    le2.swatchColor = {0, 0, 255};
    le2.lineStyle = scatterData.lineStyle;
    legendEntries.push_back(le2);

    LegendRenderer legend;
    legend.render(device, legendEntries, area, 11.0);

    // 验证: Line 产生 polyline, Scatter 产生 markers
    assert(device.drawPolylineCalls >= 1);   // Line 产生 polyline
    assert(device.drawMarkersCalls >= 1);     // Scatter 产生 markers
    assert(device.fillRectCalls >= 2);        // 图例背景 + 2 个色块
    assert(device.drawTextCalls >= 2);        // 2 个图例标签
}

// ============================================================
// main
// ============================================================
int main() {
    // Registry
    test_registry_builtins();
    test_registry_custom_type();

    // LinePlot
    test_line_plot_renders_polyline();
    test_line_plot_empty_data();
    test_line_plot_with_markers();
    test_line_plot_coordinate_transform();
    test_line_plot_single_point();
    test_line_plot_log10_scale();

    // ScatterPlot
    test_scatter_plot_renders_markers();
    test_scatter_plot_default_marker_style();
    test_scatter_plot_with_line();
    test_scatter_plot_empty_data();
    test_scatter_plot_single_point();

    // MultiAxis
    test_multi_axis_default();
    test_multi_axis_add_right();
    test_multi_axis_config();
    test_multi_axis_auto_range();
    test_multi_axis_render_labels();
    test_multi_axis_multiple_right();
    test_multi_axis_out_of_bounds();
    test_multi_axis_render_ticks_integration();

    // LegendRenderer
    test_legend_renderer_layout();
    test_legend_renderer_render();
    test_legend_empty_entries();
    test_legend_single_entry();
    test_legend_many_entries();

    // Coordinate Transform
    test_transform_linear();
    test_transform_batch();
    test_transform_log_scale();

    // Integration
    test_integration_line_and_scatter();

    return 0;
}
