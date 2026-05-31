// ============================================================
// plot.cpp — Plot facade class implementation
// Owner: Agent E — Backend & Integration
// ============================================================
// Orchestrates: DataBinding → Layout → Axis → Transform → Render
// Depends on: IRenderDevice (Agent A, frozen), types.h (Agent A, frozen)
// Uses Agent C modules: axis_system, coordinate_transform, layout_engine
// Coordinates with: Agent D (plot_types, legend_renderer)
// ============================================================
#include "xyplot/plot.h"
#include "plot_impl.h"
#include "axis_system.h"           // Agent C: computeTicks, formatTickLabel
#include "coordinate_transform.h"  // Agent C: transform, transformPoints
#include "xyplot_internal.h"       // Agent D: IPlotType, PlotRegistry, SeriesRenderData
#include <vector>
#include <string>
#include <cstring>
#include <cmath>
#include <algorithm>

namespace xyplot {

// ==================================================================
// Plot lifecycle
// (SeriesInfo, Plot::Impl, LayoutResult are in plot_impl.h)
// ==================================================================
Plot::Plot() : m_impl(new Impl()) {}
Plot::~Plot() { delete m_impl; }
Plot::Plot(Plot&& other) noexcept : m_impl(other.m_impl) { other.m_impl = nullptr; }
Plot& Plot::operator=(Plot&& other) noexcept {
    if (this != &other) { delete m_impl; m_impl = other.m_impl; other.m_impl = nullptr; }
    return *this;
}

// ==================================================================
// Canvas (BUG-001 fix)
// ==================================================================
void Plot::setCanvasSize(double width, double height) {
    if (width > 0)  m_impl->canvasWidth  = width;
    if (height > 0) m_impl->canvasHeight = height;
}

// ==================================================================
// Data binding — internal helper (BUG-002 + BUG-003 fix)
// ==================================================================
namespace {

const Color kColorCycle[] = {
    { 31, 119, 180 }, { 255, 127, 14 }, { 44, 160, 44 },
    { 148, 103, 189 }, { 140, 86, 75 }, { 227, 119, 194 },
    { 127, 127, 127 }, { 188, 189, 34 }, { 23, 190, 207 }
};

} // anonymous namespace

// ==================================================================
// Data binding — public API
// ==================================================================
int Plot::addLineSeries(const char* name, const double* xs, const double* ys,
                         int count, int yAxisIndex) {
    SeriesInfo s;
    s.name = name ? name : "";
    s.xs.assign(xs, xs + count);
    s.ys.assign(ys, ys + count);
    s.type = SeriesInfo::Line;
    s.yAxisIndex = yAxisIndex;
    int idx = static_cast<int>(m_impl->series.size());
    s.lineStyle.color = kColorCycle[idx % 9];
    s.lineStyle.width = 2.0;
    s.markerStyle.fillColor = s.lineStyle.color;
    s.markerStyle.edgeColor = s.lineStyle.color;
    s.markerStyle.shape = MarkerStyle::Circle;
    s.markerStyle.size = 5.0;
    m_impl->series.push_back(std::move(s));
    return idx;
}

int Plot::addScatterSeries(const char* name, const double* xs, const double* ys,
                            int count, int yAxisIndex) {
    SeriesInfo s;
    s.name = name ? name : "";
    s.xs.assign(xs, xs + count);
    s.ys.assign(ys, ys + count);
    s.type = SeriesInfo::Scatter;
    s.yAxisIndex = yAxisIndex;
    int idx = static_cast<int>(m_impl->series.size());
    s.lineStyle.color = kColorCycle[idx % 9];
    s.lineStyle.width = 2.0;
    s.markerStyle.fillColor = s.lineStyle.color;
    s.markerStyle.edgeColor = s.lineStyle.color;
    s.markerStyle.shape = MarkerStyle::Circle;
    s.markerStyle.size = 5.0;
    m_impl->series.push_back(std::move(s));
    return idx;
}

int Plot::addBarSeries(const char* name, const double* xs, const double* ys, int count) {
    SeriesInfo s;
    s.name = name ? name : "";
    s.xs.assign(xs, xs + count);
    s.ys.assign(ys, ys + count);
    s.type = SeriesInfo::Bar;
    int idx = static_cast<int>(m_impl->series.size());
    s.lineStyle.color = kColorCycle[idx % 9];
    s.lineStyle.width = 2.0;
    m_impl->series.push_back(std::move(s));
    return idx;
}

int Plot::addStepSeries(const char* name, const double* xs, const double* ys, int count) {
    SeriesInfo s;
    s.name = name ? name : "";
    s.xs.assign(xs, xs + count);
    s.ys.assign(ys, ys + count);
    s.type = SeriesInfo::Step;
    int idx = static_cast<int>(m_impl->series.size());
    s.lineStyle.color = kColorCycle[idx % 9];
    s.lineStyle.width = 2.0;
    m_impl->series.push_back(std::move(s));
    return idx;
}

int Plot::addAreaSeries(const char* name, const double* xs, const double* ys, int count) {
    SeriesInfo s;
    s.name = name ? name : "";
    s.xs.assign(xs, xs + count);
    s.ys.assign(ys, ys + count);
    s.type = SeriesInfo::Area;
    int idx = static_cast<int>(m_impl->series.size());
    s.lineStyle.color = kColorCycle[idx % 9];
    s.lineStyle.width = 2.0;
    m_impl->series.push_back(std::move(s));
    return idx;
}

int Plot::addHistogramSeries(const char* name, const double* xs, const double* ys, int count) {
    SeriesInfo s;
    s.name = name ? name : "";
    s.xs.assign(xs, xs + count);
    s.ys.assign(ys, ys + count);
    s.type = SeriesInfo::Histogram;
    int idx = static_cast<int>(m_impl->series.size());
    s.lineStyle.color = kColorCycle[idx % 9];
    s.lineStyle.width = 2.0;
    m_impl->series.push_back(std::move(s));
    return idx;
}

int Plot::addErrorBarSeries(const char* name, const double* xs, const double* ys, int count) {
    SeriesInfo s;
    s.name = name ? name : "";
    s.xs.assign(xs, xs + count);
    s.ys.assign(ys, ys + count);
    s.type = SeriesInfo::ErrorBar;
    int idx = static_cast<int>(m_impl->series.size());
    s.lineStyle.color = kColorCycle[idx % 9];
    s.lineStyle.width = 2.0;
    m_impl->series.push_back(std::move(s));
    return idx;
}

int Plot::addPolarSeries(const char* name, const double* xs, const double* ys, int count) {
    SeriesInfo s;
    s.name = name ? name : "";
    s.xs.assign(xs, xs + count);
    s.ys.assign(ys, ys + count);
    s.type = SeriesInfo::Polar;
    int idx = static_cast<int>(m_impl->series.size());
    s.lineStyle.color = kColorCycle[idx % 9];
    s.lineStyle.width = 2.0;
    m_impl->series.push_back(std::move(s));
    return idx;
}

int Plot::addHeatmapSeries(const char* name, const double* xs, const double* ys, int count) {
    SeriesInfo s;
    s.name = name ? name : "";
    s.xs.assign(xs, xs + count);
    s.ys.assign(ys, ys + count);
    s.type = SeriesInfo::Heatmap;
    int idx = static_cast<int>(m_impl->series.size());
    s.lineStyle.color = kColorCycle[idx % 9];
    s.lineStyle.width = 2.0;
    m_impl->series.push_back(std::move(s));
    return idx;
}

int Plot::addContourSeries(const char* name, const double* xs, const double* ys, int count) {
    SeriesInfo s;
    s.name = name ? name : "";
    s.xs.assign(xs, xs + count);
    s.ys.assign(ys, ys + count);
    s.type = SeriesInfo::Contour;
    int idx = static_cast<int>(m_impl->series.size());
    s.lineStyle.color = kColorCycle[idx % 9];
    s.lineStyle.width = 2.0;
    m_impl->series.push_back(std::move(s));
    return idx;
}

void Plot::updateSeriesData(int seriesId, const double* xs, const double* ys, int count) {
    if (seriesId < 0 || seriesId >= static_cast<int>(m_impl->series.size())) return;
    auto& s = m_impl->series[seriesId];
    s.xs.assign(xs, xs + count);
    s.ys.assign(ys, ys + count);
}

// ==================================================================
// Axis configuration
// ==================================================================
void Plot::xAxisSetLabel(const char* label) { m_impl->xLabel = label ? label : ""; }
void Plot::yAxisSetLabel(const char* label) { m_impl->yLabel = label ? label : ""; }
void Plot::yAxisAddRight(const char* label) { m_impl->yRightLabel = label ? label : ""; }
void Plot::xAxisSetScale(ScaleType type) { m_impl->xScale = type; }
void Plot::setAxisRange(double xMin, double xMax, double yMin, double yMax, int yAxisIndex) {
    if (yAxisIndex == 0) {
        m_impl->xMin = xMin; m_impl->xMax = xMax;
        m_impl->yMin = yMin; m_impl->yMax = yMax;
    } else {
        m_impl->yRightMin = yMin; m_impl->yRightMax = yMax;
    }
    m_impl->autoRange = false;
}

// ==================================================================
// Styling
// ==================================================================
void Plot::setTitle(const char* title) { m_impl->title = title ? title : ""; }
void Plot::setSeriesStyle(int seriesId, const LineStyle& style) {
    if (seriesId >= 0 && seriesId < static_cast<int>(m_impl->series.size()))
        m_impl->series[seriesId].lineStyle = style;
}

// ==================================================================
// Auto-range calculation (static method of Plot::Impl)
// ==================================================================
void Plot::Impl::computeAutoRange(Impl* impl) {
    if (!impl->autoRange) return;
    double xMin = 0, xMax = 1, yMin = 0, yMax = 1;
    bool first = true;
    for (auto& s : impl->series) {
        if (!s.visible) continue;
        if (s.xs.empty()) continue;
        if (s.yAxisIndex == 0) {
            for (size_t i = 0; i < s.xs.size(); i++) {
                if (first) {
                    xMin = xMax = s.xs[i];
                    yMin = yMax = s.ys[i];
                    first = false;
                } else {
                    if (s.xs[i] < xMin) xMin = s.xs[i];
                    if (s.xs[i] > xMax) xMax = s.xs[i];
                    if (s.ys[i] < yMin) yMin = s.ys[i];
                    if (s.ys[i] > yMax) yMax = s.ys[i];
                }
            }
        }
    }
    double xPad = (xMax - xMin) * 0.05;
    double yPad = (yMax - yMin) * 0.05;
    if (xPad == 0) xPad = 0.5;
    if (yPad == 0) yPad = 0.5;
    impl->xMin = xMin - xPad;
    impl->xMax = xMax + xPad;
    impl->yMin = yMin - yPad;
    impl->yMax = yMax + yPad;
}

// ==================================================================
// Core rendering
// ==================================================================
void Plot::render(IRenderDevice& device) {
    device.beginFrame();

    Impl::computeAutoRange(m_impl);

    // ── Layout (Agent C: layout_engine) ──
    bool hasTitle = !m_impl->title.empty();
    bool hasXLabel = !m_impl->xLabel.empty();
    bool hasYLabel = !m_impl->yLabel.empty();
    bool hasYRightLabel = !m_impl->yRightLabel.empty();
    bool hasLegend = !m_impl->series.empty();
    int legendCount = static_cast<int>(m_impl->series.size());

    LayoutConfig layoutCfg;
    layoutCfg.totalWidth  = m_impl->canvasWidth;
    layoutCfg.totalHeight = m_impl->canvasHeight;
    layoutCfg.hasTitle           = hasTitle;
    layoutCfg.hasXAxisLabel      = hasXLabel;
    layoutCfg.hasYAxisLabel      = hasYLabel;
    layoutCfg.hasYAxisRightLabel = hasYRightLabel;
    layoutCfg.hasLegend          = hasLegend;
    layoutCfg.title              = m_impl->title;
    layoutCfg.xLabel             = m_impl->xLabel;
    layoutCfg.yLabel             = m_impl->yLabel;
    layoutCfg.yRightLabel        = m_impl->yRightLabel;
    layoutCfg.legendItemCount    = legendCount;

    m_impl->layout = computeLayout(layoutCfg, device);

    const LayoutResult& L = m_impl->layout;
    const Rect& pr = L.plotRect;

    // ── 1. Plot area background ──
    {
        FillStyle bgFill; bgFill.color = { 252, 252, 252 };
        device.fillRect(pr.x, pr.y, pr.w, pr.h, bgFill);
    }

    // ── 2. Grid lines ──
    AxisConfig yAxisCfg;
    yAxisCfg.dataMin = m_impl->yMin;
    yAxisCfg.dataMax = m_impl->yMax;
    yAxisCfg.scaleType = m_impl->yScale;
    yAxisCfg.targetMajorTicks = 5;
    AxisTicks yTicks = computeTicks(yAxisCfg);

    AxisConfig xAxisCfg;
    xAxisCfg.dataMin = m_impl->xMin;
    xAxisCfg.dataMax = m_impl->xMax;
    xAxisCfg.scaleType = m_impl->xScale;
    xAxisCfg.targetMajorTicks = 5;
    AxisTicks xTicks = computeTicks(xAxisCfg);

    LineStyle gridStyle;
    gridStyle.width = 0.5;
    gridStyle.color = { 220, 220, 220 };
    gridStyle.dash = LineStyle::DashLine;

    device.setClipRect(pr.x, pr.y, pr.w, pr.h);

    for (double yv : yTicks.majorTicks) {
        double dy = transform(yv, m_impl->yMin, m_impl->yMax,
                              pr.y + pr.h, pr.y, m_impl->yScale);
        double gx[] = { pr.x, pr.x + pr.w };
        double gyArr[] = { dy, dy };
        device.drawPolyline(gx, gyArr, 2, gridStyle);
    }

    for (double xv : xTicks.majorTicks) {
        double dx = transform(xv, m_impl->xMin, m_impl->xMax,
                              pr.x, pr.x + pr.w, m_impl->xScale);
        double gyArr2[] = { pr.y, pr.y + pr.h };
        double gxArr[] = { dx, dx };
        device.drawPolyline(gxArr, gyArr2, 2, gridStyle);
    }

    // ── 3. Data series — Agent D 类型分发 ──
    static const char* kTypeNames[] = {
        "Line", "Scatter", "Bar", "Step", "Area",
        "Histogram", "ErrorBar", "Polar", "Heatmap", "Contour"
    };

    for (auto& s : m_impl->series) {
        if (!s.visible || s.xs.empty()) continue;

        double yLo = (s.yAxisIndex == 0) ? m_impl->yMin : m_impl->yRightMin;
        double yHi = (s.yAxisIndex == 0) ? m_impl->yMax : m_impl->yRightMax;
        ScaleType ySc = (s.yAxisIndex == 0) ? m_impl->yScale : m_impl->yRightScale;

        internal::SeriesRenderData renderData;
        renderData.xs = s.xs.data();
        renderData.ys = s.ys.data();
        renderData.count = static_cast<int>(s.xs.size());
        renderData.lineStyle = s.lineStyle;
        renderData.markerStyle = s.markerStyle;
        renderData.yAxisIndex = s.yAxisIndex;
        renderData.name = s.name.c_str();

        internal::AxisRenderConfig axisCfg;
        axisCfg.xMin = m_impl->xMin;
        axisCfg.xMax = m_impl->xMax;
        axisCfg.yMin = yLo;
        axisCfg.yMax = yHi;
        axisCfg.xScale = m_impl->xScale;
        axisCfg.yScale = ySc;

        internal::DevicePlotArea plotArea;
        plotArea.left = pr.x;
        plotArea.top = pr.y;
        plotArea.width = pr.w;
        plotArea.height = pr.h;

        const char* typeName = (s.type >= 0 && s.type < 10)
            ? kTypeNames[s.type] : "Line";

        auto plotType = internal::createPlotType(typeName);
        if (plotType) {
            plotType->render(device, renderData, axisCfg, plotArea);
        } else {
            // Fallback: unknown type → drawPolyline
            std::vector<double> dx(s.xs.size()), dy(s.xs.size());
            transformPoints(s.xs.data(), s.ys.data(), static_cast<int>(s.xs.size()),
                            m_impl->xMin, m_impl->xMax, yLo, yHi,
                            pr.x, pr.x + pr.w, pr.y + pr.h, pr.y,
                            dx.data(), dy.data(),
                            m_impl->xScale, ySc);
            device.drawPolyline(dx.data(), dy.data(),
                               static_cast<int>(dx.size()), s.lineStyle);
        }
    }
    device.resetClip();

    // ── 4. Axes ──
    LineStyle axisStyle;
    axisStyle.width = 1.5;
    axisStyle.color = { 0, 0, 0 };

    {
        double ax[] = { pr.x, pr.x + pr.w };
        double ay[] = { pr.y + pr.h, pr.y + pr.h };
        device.drawPolyline(ax, ay, 2, axisStyle);
    }
    {
        double ay2[] = { pr.y, pr.y + pr.h };
        double ax2[] = { pr.x, pr.x };
        device.drawPolyline(ax2, ay2, 2, axisStyle);
    }
    if (!m_impl->yRightLabel.empty() || hasYRightLabel) {
        double ay3[] = { pr.y, pr.y + pr.h };
        double ax3[] = { pr.x + pr.w, pr.x + pr.w };
        device.drawPolyline(ax3, ay3, 2, axisStyle);
    }

    // ── 5. Tick labels ──
    FontDesc tickFont; tickFont.size = 10;
    TextStyle tickStyle;
    tickStyle.hAlign = TextStyle::Center;
    tickStyle.vAlign = TextStyle::Top;
    for (size_t i = 0; i < xTicks.majorTicks.size(); i++) {
        double dx = transform(xTicks.majorTicks[i], m_impl->xMin, m_impl->xMax,
                              pr.x, pr.x + pr.w, m_impl->xScale);
        // BUG-005 fix: gap = tickLength(5) + fontSize(10) + 6 = 21px
        device.drawText(dx, pr.y + pr.h + tickFont.size + 6,
                        xTicks.labels[i].c_str(), tickFont, tickStyle);
    }

    TextStyle yTickStyle;
    yTickStyle.hAlign = TextStyle::Right;
    yTickStyle.vAlign = TextStyle::Middle;
    for (size_t i = 0; i < yTicks.majorTicks.size(); i++) {
        double dy = transform(yTicks.majorTicks[i], m_impl->yMin, m_impl->yMax,
                              pr.y + pr.h, pr.y, m_impl->yScale);
        device.drawText(pr.x - 5, dy, yTicks.labels[i].c_str(), tickFont, yTickStyle);
    }

    if (!m_impl->yRightLabel.empty()) {
        AxisConfig yRAxisCfg;
        yRAxisCfg.dataMin = m_impl->yRightMin;
        yRAxisCfg.dataMax = m_impl->yRightMax;
        yRAxisCfg.scaleType = m_impl->yRightScale;
        yRAxisCfg.targetMajorTicks = 5;
        AxisTicks yRTicks = computeTicks(yRAxisCfg);

        TextStyle yRTickStyle;
        yRTickStyle.hAlign = TextStyle::Left;
        yRTickStyle.vAlign = TextStyle::Middle;
        for (size_t i = 0; i < yRTicks.majorTicks.size(); i++) {
            double dy = transform(yRTicks.majorTicks[i], m_impl->yRightMin, m_impl->yRightMax,
                                  pr.y + pr.h, pr.y, m_impl->yRightScale);
            device.drawText(pr.x + pr.w + 5, dy,
                            yRTicks.labels[i].c_str(), tickFont, yRTickStyle);
        }
    }

    // ── 6. Title ──
    if (hasTitle) {
        FontDesc titleFont; titleFont.size = 16; titleFont.bold = true;
        TextStyle titleStyle;
        titleStyle.hAlign = TextStyle::Center;
        titleStyle.color = { 0, 0, 0 };
        device.drawText(pr.x + pr.w / 2.0, L.titleRect.y + 4,
                        m_impl->title.c_str(), titleFont, titleStyle);
    }

    // ── 7. Axis labels ──
    FontDesc axisLabelFont; axisLabelFont.size = 13;

    if (hasXLabel) {
        TextStyle xlStyle; xlStyle.hAlign = TextStyle::Center;
        device.drawText(pr.x + pr.w / 2.0, L.xLabelRect.y + 4,
                        m_impl->xLabel.c_str(), axisLabelFont, xlStyle);
    }
    if (hasYLabel) {
        TextStyle ylStyle; ylStyle.hAlign = TextStyle::Center;
        device.drawText(L.yLabelRect.x + 5, pr.y + pr.h / 2.0,
                        m_impl->yLabel.c_str(), axisLabelFont, ylStyle);
    }
    if (hasYRightLabel) {
        TextStyle yrlStyle; yrlStyle.hAlign = TextStyle::Center;
        device.drawText(L.yLabelRightRect.x, pr.y + pr.h / 2.0,
                        m_impl->yRightLabel.c_str(), axisLabelFont, yrlStyle);
    }

    // ── 8. Legend ──
    if (hasLegend) {
        const Rect& la = L.legendRect;
        FillStyle legendBg; legendBg.color = { 255, 255, 255, 240 };
        device.fillRect(la.x, la.y, la.w, la.h, legendBg);

        LineStyle legendBorder;
        legendBorder.width = 0.5;
        legendBorder.color = { 180, 180, 180 };
        {
            double bx[] = { la.x, la.x + la.w, la.x + la.w, la.x, la.x };
            double by[] = { la.y, la.y, la.y + la.h, la.y + la.h, la.y };
            device.drawPolyline(bx, by, 5, legendBorder);
        }

        FontDesc legendFont; legendFont.size = 11;
        TextStyle legendStyle;
        legendStyle.vAlign = TextStyle::Middle;
        for (size_t i = 0; i < m_impl->series.size(); i++) {
            double itemY = la.y + 8 + i * 20.0;
            Color c = m_impl->series[i].lineStyle.color;

            FillStyle swatchFill; swatchFill.color = c;
            device.fillRect(la.x + 8, itemY, 14, 14, swatchFill);

            LineStyle swatchBorder; swatchBorder.width = 0.5;
            swatchBorder.color = { 100, 100, 100 };
            {
                double sbx[] = { la.x + 8, la.x + 22, la.x + 22, la.x + 8, la.x + 8 };
                double sby[] = { itemY, itemY, itemY + 14, itemY + 14, itemY };
                device.drawPolyline(sbx, sby, 5, swatchBorder);
            }

            device.drawText(la.x + 28, itemY + 10,
                            m_impl->series[i].name.c_str(), legendFont, legendStyle);
        }
    }

    device.endFrame();
}

} // namespace xyplot
