// ============================================================
// layout_engine.h — Automatic area layout computation
// ============================================================
// Owner: Agent C (核心算法)
// Consumers: Agent E (Plot 门面 — 串联 Layout + Axis + PlotTypes)
// Depends on: types.h, irender_device.h (for textExtent) — frozen
//
// Takes available space + content configuration, computes rectangular
// regions for: title, plot, axes, labels, legend.
//
// Layout is pure computation — no rendering calls, only textExtent.
// ============================================================
#pragma once
#include "xyplot/types.h"
#include "xyplot/irender_device.h"
#include <string>

namespace xyplot {

// ──── Layout input ────
struct LayoutConfig {
    // Available device space
    double totalWidth  = 800.0;
    double totalHeight = 600.0;

    // Content presence flags
    bool hasTitle           = false;
    bool hasXAxisLabel      = false;
    bool hasYAxisLabel      = false;
    bool hasYAxisRightLabel = false;
    bool hasLegend          = false;

    // Override sizes (0 = auto from font metrics)
    double titleHeight      = 0.0;
    double xLabelHeight     = 0.0;
    double yLabelWidth      = 0.0;
    double legendWidth      = 0.0;
    double legendHeight     = 0.0;

    // Font descriptions for text extent calls
    FontDesc titleFont     {14.0, true,  false};
    FontDesc axisLabelFont {12.0, false, false};
    FontDesc tickLabelFont {10.0, false, false};
    FontDesc legendFont    {11.0, false, false};

    // Text content (needed for extent measurement)
    std::string title;
    std::string xLabel;
    std::string yLabel;
    std::string yRightLabel;

    // Tick label extents (the longest label on each axis)
    double maxTickLabelWidth  = 0.0;   // 0 = auto from typical tick width
    double maxTickLabelHeight = 0.0;   // 0 = auto from tick font height

    // Legend item count (affects legend height)
    int legendItemCount = 0;
};

// ──── Layout output ────
struct LayoutResult {
    Rect titleRect;          // Title bounding box
    Rect plotRect;           // Main plotting area
    Rect xAxisRect;          // X axis line + ticks (below plot)
    Rect yAxisRect;          // Y axis line + ticks (left of plot)
    Rect yAxisRightRect;     // Right Y axis line + ticks (right of plot)
    Rect legendRect;         // Legend bounding box
    Rect xLabelRect;         // X axis label (below x axis)
    Rect yLabelRect;         // Y axis label (left of y axis)
    Rect yLabelRightRect;    // Right Y axis label (right of right y axis)

    // Convenience
    bool isValid() const { return plotRect.w > 0 && plotRect.h > 0; }
};

// ──── API ────

/// Compute the layout. Uses `device.textExtent()` for text measurements.
LayoutResult computeLayout(const LayoutConfig& config, IRenderDevice& device);

/// Compute a minimal layout without a device (uses conservative font estimates).
LayoutResult computeLayoutMinimal(const LayoutConfig& config);

} // namespace xyplot
