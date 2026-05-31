// ============================================================
// hit_test.h — HitTest system declaration
// Owner: Agent E — Backend & Integration
// ============================================================
// Provides device-coordinate → chart-element lookup.
// Used by InteractionHandler to translate mouse/touch events
// into semantic actions (curve selection, data picking).
// ============================================================
#pragma once
#include "xyplot/types.h"
#include <vector>
#include <string>
#include <cstddef>

namespace xyplot {

// ==================================================================
// HitTestResult — what was hit
// ==================================================================
struct HitTestResult {
    enum HitType {
        None = 0,
        PlotArea,       // Clicked inside the plot area (but not on a curve/point)
        Axis,           // Clicked on an axis region
        LegendItem,     // Clicked on a legend entry
        CurveLine,      // Clicked near a curve line
        DataPoint       // Clicked near a specific data point
    };

    HitType hitType = None;

    // Which series (if any)
    int seriesIndex = -1;

    // Which data point within the series (if DataPoint)
    int dataPointIndex = -1;

    // Data-space coordinates of the hit
    double dataX = 0.0;
    double dataY = 0.0;

    // Device-space distance from click to hit (px)
    double distancePx = 0.0;
};

// ==================================================================
// Layout descriptor (passed from Plot to HitTest)
// ==================================================================
struct HitTestLayout {
    Rect plotArea;
    Rect legendArea;
    double legendItemHeight = 20.0;
    int legendItemCount = 0;
};

// ==================================================================
// Series descriptor for hit testing
// ==================================================================
struct HitTestSeries {
    const double* xs = nullptr;
    const double* ys = nullptr;
    int count = 0;
    int yAxisIndex = 0;
    bool visible = true;
};

// ==================================================================
// Axis range descriptor
// ==================================================================
struct HitTestAxisRange {
    double xMin = 0, xMax = 1;
    double yMin = 0, yMax = 1;
    double yRightMin = 0, yRightMax = 1;
    ScaleType xScale = ScaleType::Linear;
    ScaleType yScale = ScaleType::Linear;
    ScaleType yRightScale = ScaleType::Linear;
};

// ==================================================================
// HitTest — core hit testing engine
// ==================================================================
class HitTest {
public:
    /// Maximum distance in pixels to consider "on" a curve (for CurveLine hits)
    static constexpr double kCurveHitRadius = 8.0;

    /// Maximum distance in pixels to consider "on" a data point
    static constexpr double kDataPointHitRadius = 12.0;

    /// Perform a full hit test at device coordinates (px, py).
    /// Checks legend first, then plot area, then curves/data points.
    static HitTestResult test(double px, double py,
                              const HitTestLayout& layout,
                              const std::vector<HitTestSeries>& series,
                              const HitTestAxisRange& axis);

private:
    /// Transform data → device (same logic as plot.cpp; duplicated for independence)
    static double dataToDeviceX(double dataX, const HitTestAxisRange& axis,
                                 const Rect& plotArea);
    static double dataToDeviceY(double dataY, int yAxisIndex,
                                 const HitTestAxisRange& axis,
                                 const Rect& plotArea);
    static double deviceToDataX(double px, const HitTestAxisRange& axis,
                                 const Rect& plotArea);
    static double deviceToDataY(double py, int yAxisIndex,
                                 const HitTestAxisRange& axis,
                                 const Rect& plotArea);

    /// Check if (px, py) is inside rect
    static bool pointInRect(double px, double py, const Rect& r);

    /// Distance from point to line segment
    static double pointToSegmentDist(double px, double py,
                                      double x1, double y1,
                                      double x2, double y2);
};

} // namespace xyplot
