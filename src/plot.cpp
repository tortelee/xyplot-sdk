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
// Data binding
// ==================================================================
int Plot::addLineSeries(const char* name, const double* xs, const double* ys, int count) {
    SeriesInfo s;
    s.name = name ? name : "";
    s.xs.assign(xs, xs + count);
    s.ys.assign(ys, ys + count);
    s.type = SeriesInfo::Line;
    // Default style: cycle through some colors
    static const Color kColorCycle[] = {
        { 31, 119, 180 }, { 255, 127, 14 }, { 44, 160, 44 },
        { 148, 103, 189 }, { 140, 86, 75 }, { 227, 119, 194 },
        { 127, 127, 127 }, { 188, 189, 34 }, { 23, 190, 207 }
    };
    int idx = static_cast<int>(m_impl->series.size());
    s.lineStyle.color = kColorCycle[idx % 9];
    s.lineStyle.width = 2.0;
    s.markerStyle.fillColor = s.lineStyle.color;
    s.markerStyle.edgeColor = s.lineStyle.color;
    s.markerStyle.shape = MarkerStyle::Circle;
    s.markerStyle.size = 5.0;
    m_impl->series.push_back(std::move(s));
    return static_cast<int>(m_impl->series.size()) - 1;
}

int Plot::addScatterSeries(const char* name, const double* xs, const double* ys, int count) {
    int id = addLineSeries(name, xs, ys, count);
    m_impl->series[id].type = SeriesInfo::Scatter;
    return id;
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
    // Add 5% padding
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

    m_impl->layout = computeLayout(layoutCfg, device);  // Agent C

    const LayoutResult& L = m_impl->layout;
    const Rect& pr = L.plotRect;  // plot area

    // ── 1. Plot area background ──
    {
        FillStyle bgFill; bgFill.color = { 252, 252, 252 };
        device.fillRect(pr.x, pr.y, pr.w, pr.h, bgFill);
    }

    // ── 2. Grid lines (Agent C: axis_system) ──
    AxisConfig yAxisCfg;
    yAxisCfg.dataMin = m_impl->yMin;
    yAxisCfg.dataMax = m_impl->yMax;
    yAxisCfg.scaleType = m_impl->yScale;
    yAxisCfg.targetMajorTicks = 5;
    AxisTicks yTicks = computeTicks(yAxisCfg);          // Agent C

    AxisConfig xAxisCfg;
    xAxisCfg.dataMin = m_impl->xMin;
    xAxisCfg.dataMax = m_impl->xMax;
    xAxisCfg.scaleType = m_impl->xScale;
    xAxisCfg.targetMajorTicks = 5;
    AxisTicks xTicks = computeTicks(xAxisCfg);          // Agent C

    LineStyle gridStyle;
    gridStyle.width = 0.5;
    gridStyle.color = { 220, 220, 220 };
    gridStyle.dash = LineStyle::DashLine;

    device.setClipRect(pr.x, pr.y, pr.w, pr.h);

    for (double yv : yTicks.majorTicks) {
        double dy = transform(yv, m_impl->yMin, m_impl->yMax,         // Agent C
                              pr.y + pr.h, pr.y, m_impl->yScale);
        double gx[] = { pr.x, pr.x + pr.w };
        double gyArr[] = { dy, dy };
        device.drawPolyline(gx, gyArr, 2, gridStyle);
    }

    // ── 3. Grid lines (vertical) ──
    for (double xv : xTicks.majorTicks) {
        double dx = transform(xv, m_impl->xMin, m_impl->xMax,         // Agent C
                              pr.x, pr.x + pr.w, m_impl->xScale);
        double gyArr2[] = { pr.y, pr.y + pr.h };
        double gxArr[] = { dx, dx };
        device.drawPolyline(gxArr, gyArr2, 2, gridStyle);
    }

    // ── 4. Data series (Agent C: coordinate_transform) ──
    for (auto& s : m_impl->series) {
        if (!s.visible || s.xs.empty()) continue;

        double yLo = (s.yAxisIndex == 0) ? m_impl->yMin : m_impl->yRightMin;
        double yHi = (s.yAxisIndex == 0) ? m_impl->yMax : m_impl->yRightMax;
        ScaleType ySc = (s.yAxisIndex == 0) ? m_impl->yScale : m_impl->yRightScale;

        std::vector<double> dx(s.xs.size()), dy(s.xs.size());
        // Batch transform via Agent C
        transformPoints(s.xs.data(), s.ys.data(), static_cast<int>(s.xs.size()),
                        m_impl->xMin, m_impl->xMax, yLo, yHi,
                        pr.x, pr.x + pr.w, pr.y + pr.h, pr.y,
                        dx.data(), dy.data(),
                        m_impl->xScale, ySc);                       // Agent C

        if (s.type == SeriesInfo::Line) {
            device.drawPolyline(dx.data(), dy.data(),
                                static_cast<int>(dx.size()), s.lineStyle);
        } else {
            device.drawMarkers(dx.data(), dy.data(),
                               static_cast<int>(dx.size()), s.markerStyle);
        }
    }
    device.resetClip();

    // ── 5. Axes (border lines) ──
    LineStyle axisStyle;
    axisStyle.width = 1.5;
    axisStyle.color = { 0, 0, 0 };

    // Bottom axis (X)
    {
        double ax[] = { pr.x, pr.x + pr.w };
        double ay[] = { pr.y + pr.h, pr.y + pr.h };
        device.drawPolyline(ax, ay, 2, axisStyle);
    }
    // Left axis (Y)
    {
        double ay2[] = { pr.y, pr.y + pr.h };
        double ax2[] = { pr.x, pr.x };
        device.drawPolyline(ax2, ay2, 2, axisStyle);
    }
    // Right axis (Y, if right axis configured)
    if (!m_impl->yRightLabel.empty() || hasYRightLabel) {
        double ay3[] = { pr.y, pr.y + pr.h };
        double ax3[] = { pr.x + pr.w, pr.x + pr.w };
        device.drawPolyline(ax3, ay3, 2, axisStyle);
    }

    // ── 6. X-axis tick labels (Agent C: formatTickLabel) ──
    FontDesc tickFont; tickFont.size = 10;
    TextStyle tickStyle;
    tickStyle.hAlign = TextStyle::Center;
    tickStyle.vAlign = TextStyle::Top;
    for (size_t i = 0; i < xTicks.majorTicks.size(); i++) {
        double xv = xTicks.majorTicks[i];
        double dx = transform(xv, m_impl->xMin, m_impl->xMax,           // Agent C
                              pr.x, pr.x + pr.w, m_impl->xScale);
        const std::string& label = xTicks.labels[i];                    // Agent C
        device.drawText(dx, pr.y + pr.h + 4, label.c_str(), tickFont, tickStyle);
    }

    // ── 7. Y-axis tick labels (left) ──
    TextStyle yTickStyle;
    yTickStyle.hAlign = TextStyle::Right;
    yTickStyle.vAlign = TextStyle::Middle;
    for (size_t i = 0; i < yTicks.majorTicks.size(); i++) {
        double yv = yTicks.majorTicks[i];
        double dy = transform(yv, m_impl->yMin, m_impl->yMax,           // Agent C
                              pr.y + pr.h, pr.y, m_impl->yScale);
        const std::string& label = yTicks.labels[i];                    // Agent C
        device.drawText(pr.x - 5, dy, label.c_str(), tickFont, yTickStyle);
    }

    // ── 8. Y-axis tick labels (right) ──
    if (!m_impl->yRightLabel.empty()) {
        AxisConfig yRAxisCfg;
        yRAxisCfg.dataMin = m_impl->yRightMin;
        yRAxisCfg.dataMax = m_impl->yRightMax;
        yRAxisCfg.scaleType = m_impl->yRightScale;
        yRAxisCfg.targetMajorTicks = 5;
        AxisTicks yRTicks = computeTicks(yRAxisCfg);                   // Agent C

        TextStyle yRTickStyle;
        yRTickStyle.hAlign = TextStyle::Left;
        yRTickStyle.vAlign = TextStyle::Middle;
        for (size_t i = 0; i < yRTicks.majorTicks.size(); i++) {
            double yv = yRTicks.majorTicks[i];
            double dy = transform(yv, m_impl->yRightMin, m_impl->yRightMax,  // Agent C
                                  pr.y + pr.h, pr.y, m_impl->yRightScale);
            const std::string& label = yRTicks.labels[i];                    // Agent C
            device.drawText(pr.x + pr.w + 5, dy, label.c_str(), tickFont, yRTickStyle);
        }
    }

    // ── 9. Title (Agent C LayoutResult: titleRect) ──
    if (hasTitle) {
        FontDesc titleFont; titleFont.size = 16; titleFont.bold = true;
        TextStyle titleStyle;
        titleStyle.hAlign = TextStyle::Center;
        titleStyle.color = { 0, 0, 0 };
        double tx = pr.x + pr.w / 2.0;
        double ty = L.titleRect.y + 4;
        device.drawText(tx, ty, m_impl->title.c_str(), titleFont, titleStyle);
    }

    // ── 10. Axis labels (Agent C LayoutResult) ──
    FontDesc axisLabelFont; axisLabelFont.size = 13; axisLabelFont.bold = false;

    if (hasXLabel) {
        TextStyle xlStyle;
        xlStyle.hAlign = TextStyle::Center;
        double lx = pr.x + pr.w / 2.0;
        double ly = L.xLabelRect.y + 4;
        device.drawText(lx, ly, m_impl->xLabel.c_str(), axisLabelFont, xlStyle);
    }

    if (hasYLabel) {
        TextStyle ylStyle;
        ylStyle.hAlign = TextStyle::Center;
        double lx = L.yLabelRect.x + 5;
        double ly = pr.y + pr.h / 2.0;
        device.drawText(lx, ly, m_impl->yLabel.c_str(), axisLabelFont, ylStyle);
    }

    if (hasYRightLabel) {
        TextStyle yrlStyle;
        yrlStyle.hAlign = TextStyle::Center;
        double rx = L.yLabelRightRect.x;
        double ry = pr.y + pr.h / 2.0;
        device.drawText(rx, ry, m_impl->yRightLabel.c_str(), axisLabelFont, yrlStyle);
    }

    // ── 11. Legend (Agent C LayoutResult: legendRect) ──
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
                            m_impl->series[i].name.c_str(),
                            legendFont, legendStyle);
        }
    }

    device.endFrame();
}

} // namespace xyplot
