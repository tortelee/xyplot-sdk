// ============================================================
// layout_engine.cpp — Auto-layout computation
// ============================================================
// Layout strategy (progressive subtraction):
//
//   1. Start with totalW × totalH
//   2. Reserve margins (outer padding)
//   3. Subtract title area (top)
//   4. Subtract axis labels (left/right/bottom)
//   5. Subtract legend (right side, top-aligned with plot)
//   6. Remaining space → plot area
//   7. Distribute tick label areas within axis rects
//
// This is a "subtractive" layout — the plot gets what's left after
// all decorations are placed. It works well because text elements
// have predictable sizes (via textExtent), while the plot scales
// to any remaining space.
// ============================================================
#include "layout_engine.h"
#include <cmath>
#include <algorithm>

namespace xyplot {

// ──── Constants ────
namespace {
    constexpr double kOuterMargin  = 10.0;
    constexpr double kInnerPadding = 5.0;
    constexpr double kAxisTickLength = 5.0;
    constexpr double kLegendItemH  = 18.0;
    constexpr double kLegendSwatchW = 12.0;
    constexpr double kLegendSwatchH = 12.0;
    constexpr double kLegendPadX    = 8.0;
    constexpr double kMinPlotW      = 50.0;
    constexpr double kMinPlotH      = 50.0;
}

// ──── Text measurement helper ────
static void measureText(IRenderDevice& device, const std::string& text,
                        const FontDesc& font, double& w, double& h) {
    if (text.empty()) {
        w = 0; h = 0;
        return;
    }
    device.textExtent(text.c_str(), font, &w, &h);
}

// ──── Conservative estimate (no device) ────
static void estimateText(const std::string& text, const FontDesc& font,
                         double& w, double& h) {
    if (text.empty()) {
        w = 0; h = 0;
        return;
    }
    // Conservative: ~0.6 × fontSize per char, height = fontSize × 1.25
    w = static_cast<double>(text.size()) * font.size * 0.65;
    h = font.size * 1.3;
}

// ──── Layout computation ────

LayoutResult computeLayout(const LayoutConfig& cfg, IRenderDevice& device) {
    LayoutResult r;

    double m = kOuterMargin;                  // outer margin
    double availW = cfg.totalWidth  - 2.0 * m;
    double availH = cfg.totalHeight - 2.0 * m;
    double left   = m;
    double top    = m;

    if (availW < kMinPlotW) availW = kMinPlotW;
    if (availH < kMinPlotH) availH = kMinPlotH;

    // ──── 1. Title area (top) ────
    double titleH = 0;
    if (cfg.hasTitle) {
        if (cfg.titleHeight > 0) {
            titleH = cfg.titleHeight;
        } else {
            double tw, th;
            measureText(device, cfg.title, cfg.titleFont, tw, th);
            titleH = th + kInnerPadding * 2.0;
        }

        r.titleRect = Rect{left, top, availW, titleH};
        top  += titleH;
        availH -= titleH;
    }

    // ──── 2. X axis label (bottom) ────
    double xLabelH = 0;
    if (cfg.hasXAxisLabel) {
        if (cfg.xLabelHeight > 0) {
            xLabelH = cfg.xLabelHeight;
        } else {
            double lw, lh;
            measureText(device, cfg.xLabel, cfg.axisLabelFont, lw, lh);
            xLabelH = lh + kInnerPadding;
        }
    }

    // ──── 3. X axis tick area (below plot) ────
    double xTickH = 0;
    {
        double th = cfg.maxTickLabelHeight > 0
                        ? cfg.maxTickLabelHeight
                        : cfg.tickLabelFont.size * 1.5;
        xTickH = th + kAxisTickLength + kInnerPadding;
    }

    // ──── 4. Y axis label (left) ────
    double yLabelW = 0;
    if (cfg.hasYAxisLabel) {
        if (cfg.yLabelWidth > 0) {
            yLabelW = cfg.yLabelWidth;
        } else {
            double lw, lh;
            measureText(device, cfg.yLabel, cfg.axisLabelFont, lw, lh);
            yLabelW = lh + kInnerPadding; // rotated text: text "height" becomes "width"
        }
    }

    // ──── 5. Y axis tick area (left of plot) ────
    double yTickW = 0;
    {
        double tw = cfg.maxTickLabelWidth > 0
                        ? cfg.maxTickLabelWidth
                        : cfg.tickLabelFont.size * 4.0; // conservative: ~4 chars
        yTickW = tw + kAxisTickLength + kInnerPadding;
    }

    // ──── 6. Right Y axis (optional) ────
    double yRightTickW = 0;
    double yRightLabelW = 0;
    if (cfg.hasYAxisRightLabel) {
        if (cfg.yLabelWidth > 0) {
            yRightLabelW = cfg.yLabelWidth;
        } else {
            double lw, lh;
            measureText(device, cfg.yRightLabel, cfg.axisLabelFont, lw, lh);
            yRightLabelW = lh + kInnerPadding;
        }
        yRightTickW = yTickW; // same tick width as left
    }

    // ──── 7. Legend area (right side, next to plot) ────
    double legendW = 0;
    double legendH = 0;
    if (cfg.hasLegend) {
        legendH = cfg.legendHeight > 0
                      ? cfg.legendHeight
                      : static_cast<double>(std::max(1, cfg.legendItemCount)) * kLegendItemH + kInnerPadding;

        // Measure legend text width
        legendW = cfg.legendWidth > 0 ? cfg.legendWidth : 80.0;
        // The actual width is measured by the legend renderer (Agent D),
        // so we just reserve a reasonable width here.
    }

    // ──── 8. Compute plot area ────
    double plotLeft = left + yLabelW + yTickW;
    double plotTop  = top;
    double plotW    = availW - yLabelW - yTickW;
    double plotH    = availH - xTickH - xLabelH;

    // Subtract right axis
    plotW -= (yRightTickW + yRightLabelW);

    // Subtract legend
    if (cfg.hasLegend) {
        plotW -= legendW;
    }

    // Ensure minimum plot size
    if (plotW < kMinPlotW) plotW = kMinPlotW;
    if (plotH < kMinPlotH) plotH = kMinPlotH;

    r.plotRect = Rect{plotLeft, plotTop, plotW, plotH};

    // ──── 9. Position axis rects ────
    r.yLabelRect = Rect{left, plotTop, yLabelW, plotH};
    r.yAxisRect  = Rect{left + yLabelW, plotTop, yTickW, plotH};

    double rightTickLeft = plotLeft + plotW;
    r.yAxisRightRect = Rect{rightTickLeft, plotTop, yRightTickW, plotH};
    r.yLabelRightRect = Rect{rightTickLeft + yRightTickW, plotTop, yRightLabelW, plotH};

    double xAxisTop = plotTop + plotH;
    r.xAxisRect  = Rect{plotLeft, xAxisTop, plotW, xTickH};
    r.xLabelRect = Rect{plotLeft, xAxisTop + xTickH, plotW, xLabelH};

    // ──── 10. Legend rect ────
    if (cfg.hasLegend) {
        double legendLeft = plotLeft + plotW + yRightTickW + yRightLabelW + kInnerPadding;
        r.legendRect = Rect{legendLeft, plotTop, legendW, legendH};
    }

    return r;
}

LayoutResult computeLayoutMinimal(const LayoutConfig& cfg) {
    // Same logic but uses font estimates instead of device.textExtent
    LayoutResult r;

    double m = kOuterMargin;
    double availW = cfg.totalWidth  - 2.0 * m;
    double availH = cfg.totalHeight - 2.0 * m;
    double left   = m;
    double top    = m;

    if (availW < kMinPlotW) availW = kMinPlotW;
    if (availH < kMinPlotH) availH = kMinPlotH;

    double titleH = 0;
    if (cfg.hasTitle) {
        if (cfg.titleHeight > 0) {
            titleH = cfg.titleHeight;
        } else {
            double tw, th;
            estimateText(cfg.title, cfg.titleFont, tw, th);
            titleH = th + kInnerPadding * 2.0;
        }
        r.titleRect = Rect{left, top, availW, titleH};
        top  += titleH;
        availH -= titleH;
    }

    double xLabelH = 0;
    if (cfg.hasXAxisLabel) {
        if (cfg.xLabelHeight > 0) {
            xLabelH = cfg.xLabelHeight;
        } else {
            double lw, lh;
            estimateText(cfg.xLabel, cfg.axisLabelFont, lw, lh);
            xLabelH = lh + kInnerPadding;
        }
    }

    double xTickH = cfg.maxTickLabelHeight > 0
                        ? cfg.maxTickLabelHeight
                        : cfg.tickLabelFont.size * 1.5;
    xTickH += kAxisTickLength + kInnerPadding;

    double yLabelW = 0;
    if (cfg.hasYAxisLabel) {
        if (cfg.yLabelWidth > 0) {
            yLabelW = cfg.yLabelWidth;
        } else {
            double lw, lh;
            estimateText(cfg.yLabel, cfg.axisLabelFont, lw, lh);
            yLabelW = lh + kInnerPadding;
        }
    }

    double yTickW = cfg.maxTickLabelWidth > 0
                        ? cfg.maxTickLabelWidth
                        : cfg.tickLabelFont.size * 4.0;
    yTickW += kAxisTickLength + kInnerPadding;

    double yRightTickW = cfg.hasYAxisRightLabel ? yTickW : 0;
    double yRightLabelW = 0;
    if (cfg.hasYAxisRightLabel) {
        if (cfg.yLabelWidth > 0) {
            yRightLabelW = cfg.yLabelWidth;
        } else {
            double lw, lh;
            estimateText(cfg.yRightLabel, cfg.axisLabelFont, lw, lh);
            yRightLabelW = lh + kInnerPadding;
        }
    }

    double legendW = 0;
    double legendH = 0;
    if (cfg.hasLegend) {
        legendH = cfg.legendHeight > 0
                      ? cfg.legendHeight
                      : static_cast<double>(std::max(1, cfg.legendItemCount)) * kLegendItemH + kInnerPadding;
        legendW = cfg.legendWidth > 0 ? cfg.legendWidth : 80.0;
    }

    double plotLeft = left + yLabelW + yTickW;
    double plotTop  = top;
    double plotW    = availW - yLabelW - yTickW - yRightTickW - yRightLabelW;
    double plotH    = availH - xTickH - xLabelH;

    if (cfg.hasLegend) plotW -= legendW;
    if (plotW < kMinPlotW) plotW = kMinPlotW;
    if (plotH < kMinPlotH) plotH = kMinPlotH;

    r.plotRect = Rect{plotLeft, plotTop, plotW, plotH};

    r.yLabelRect = Rect{left, plotTop, yLabelW, plotH};
    r.yAxisRect  = Rect{left + yLabelW, plotTop, yTickW, plotH};

    double rightTickLeft = plotLeft + plotW;
    r.yAxisRightRect = Rect{rightTickLeft, plotTop, yRightTickW, plotH};
    r.yLabelRightRect = Rect{rightTickLeft + yRightTickW, plotTop, yRightLabelW, plotH};

    double xAxisTop = plotTop + plotH;
    r.xAxisRect  = Rect{plotLeft, xAxisTop, plotW, xTickH};
    r.xLabelRect = Rect{plotLeft, xAxisTop + xTickH, plotW, xLabelH};

    if (cfg.hasLegend) {
        double legendLeft = plotLeft + plotW + yRightTickW + yRightLabelW + kInnerPadding;
        r.legendRect = Rect{legendLeft, plotTop, legendW, legendH};
    }

    return r;
}

} // namespace xyplot
