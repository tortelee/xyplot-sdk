// ============================================================
// hit_test.cpp — HitTest system implementation
// Owner: Agent E — Backend & Integration
// ============================================================
#include "hit_test.h"
#include <cmath>
#include <algorithm>
#include <cfloat>

namespace xyplot {

// ==================================================================
// Coordinate transforms (device ↔ data)
// Duplicated from plot.cpp's transform_util for module independence.
// ==================================================================

double HitTest::dataToDeviceX(double dataX, const HitTestAxisRange& axis,
                               const Rect& plotArea) {
    double srcLo = axis.xMin, srcHi = axis.xMax;
    double dstLo = plotArea.x, dstHi = plotArea.x + plotArea.w;

    if (axis.xScale == ScaleType::Log10 || axis.xScale == ScaleType::Ln) {
        double logLo = std::log10(srcLo > 0 ? srcLo : 1e-10);
        double logHi = std::log10(srcHi > 0 ? srcHi : 1.0);
        double logRange = logHi - logLo;
        if (logRange == 0.0) logRange = 1.0;
        double t = (std::log10(dataX > 0 ? dataX : 1e-10) - logLo) / logRange;
        return dstLo + t * (dstHi - dstLo);
    }
    double srcRange = srcHi - srcLo;
    if (srcRange == 0.0) srcRange = 1.0;
    double t = (dataX - srcLo) / srcRange;
    return dstLo + t * (dstHi - dstLo);
}

double HitTest::dataToDeviceY(double dataY, int yAxisIndex,
                               const HitTestAxisRange& axis,
                               const Rect& plotArea) {
    double srcLo = (yAxisIndex == 0) ? axis.yMin : axis.yRightMin;
    double srcHi = (yAxisIndex == 0) ? axis.yMax : axis.yRightMax;
    ScaleType scale = (yAxisIndex == 0) ? axis.yScale : axis.yRightScale;
    double dstLo = plotArea.y + plotArea.h; // bottom
    double dstHi = plotArea.y;              // top (Y is inverted in device space)

    if (scale == ScaleType::Log10 || scale == ScaleType::Ln) {
        double logLo = std::log10(srcLo > 0 ? srcLo : 1e-10);
        double logHi = std::log10(srcHi > 0 ? srcHi : 1.0);
        double logRange = logHi - logLo;
        if (logRange == 0.0) logRange = 1.0;
        double t = (std::log10(dataY > 0 ? dataY : 1e-10) - logLo) / logRange;
        return dstLo + t * (dstHi - dstLo);
    }
    double srcRange = srcHi - srcLo;
    if (srcRange == 0.0) srcRange = 1.0;
    double t = (dataY - srcLo) / srcRange;
    return dstLo + t * (dstHi - dstLo);
}

double HitTest::deviceToDataX(double px, const HitTestAxisRange& axis,
                               const Rect& plotArea) {
    double srcLo = axis.xMin, srcHi = axis.xMax;
    double dstLo = plotArea.x, dstHi = plotArea.x + plotArea.w;
    double dstRange = dstHi - dstLo;
    if (dstRange == 0.0) dstRange = 1.0;

    if (axis.xScale == ScaleType::Log10 || axis.xScale == ScaleType::Ln) {
        double logLo = std::log10(srcLo > 0 ? srcLo : 1e-10);
        double logHi = std::log10(srcHi > 0 ? srcHi : 1.0);
        double logRange = logHi - logLo;
        if (logRange == 0.0) logRange = 1.0;
        double t = (px - dstLo) / dstRange;
        return std::pow(10.0, logLo + t * logRange);
    }
    double srcRange = srcHi - srcLo;
    if (srcRange == 0.0) srcRange = 1.0;
    double t = (px - dstLo) / dstRange;
    return srcLo + t * srcRange;
}

double HitTest::deviceToDataY(double py, int yAxisIndex,
                               const HitTestAxisRange& axis,
                               const Rect& plotArea) {
    double srcLo = (yAxisIndex == 0) ? axis.yMin : axis.yRightMin;
    double srcHi = (yAxisIndex == 0) ? axis.yMax : axis.yRightMax;
    ScaleType scale = (yAxisIndex == 0) ? axis.yScale : axis.yRightScale;
    double dstLo = plotArea.y + plotArea.h;
    double dstHi = plotArea.y;
    double dstRange = dstHi - dstLo;
    if (dstRange == 0.0) dstRange = 1.0;

    if (scale == ScaleType::Log10 || scale == ScaleType::Ln) {
        double logLo = std::log10(srcLo > 0 ? srcLo : 1e-10);
        double logHi = std::log10(srcHi > 0 ? srcHi : 1.0);
        double logRange = logHi - logLo;
        if (logRange == 0.0) logRange = 1.0;
        double t = (py - dstLo) / dstRange;
        return std::pow(10.0, logLo + t * logRange);
    }
    double srcRange = srcHi - srcLo;
    if (srcRange == 0.0) srcRange = 1.0;
    double t = (py - dstLo) / dstRange;
    return srcLo + t * srcRange;
}

// ==================================================================
// Point-in-rect test
// ==================================================================
bool HitTest::pointInRect(double px, double py, const Rect& r) {
    return px >= r.x && px <= r.x + r.w &&
           py >= r.y && py <= r.y + r.h;
}

// ==================================================================
// Point-to-segment distance
// ==================================================================
double HitTest::pointToSegmentDist(double px, double py,
                                    double x1, double y1,
                                    double x2, double y2) {
    double dx = x2 - x1;
    double dy = y2 - y1;
    double lenSq = dx * dx + dy * dy;
    if (lenSq < 1e-12) {
        // Degenerate segment: treat as point
        double pdx = px - x1, pdy = py - y1;
        return std::sqrt(pdx * pdx + pdy * pdy);
    }
    // Project point onto line, clamp to segment
    double t = ((px - x1) * dx + (py - y1) * dy) / lenSq;
    t = std::max(0.0, std::min(1.0, t));
    double projX = x1 + t * dx;
    double projY = y1 + t * dy;
    double pdx = px - projX, pdy = py - projY;
    return std::sqrt(pdx * pdx + pdy * pdy);
}

// ==================================================================
// Main hit test
// ==================================================================
HitTestResult HitTest::test(double px, double py,
                             const HitTestLayout& layout,
                             const std::vector<HitTestSeries>& series,
                             const HitTestAxisRange& axis) {
    HitTestResult result;

    // ── Priority 1: Legend items (highest z-order for interaction) ──
    if (layout.legendItemCount > 0 && layout.legendArea.w > 0) {
        for (int i = 0; i < layout.legendItemCount; i++) {
            double itemY = layout.legendArea.y + 8 + i * layout.legendItemHeight;
            Rect itemRect = { layout.legendArea.x, itemY,
                              layout.legendArea.w, layout.legendItemHeight };
            if (pointInRect(px, py, itemRect)) {
                result.hitType = HitTestResult::LegendItem;
                result.seriesIndex = i;
                result.distancePx = 0.0;
                return result;
            }
        }
    }

    // ── Priority 2: Plot area ──
    bool inPlot = pointInRect(px, py, layout.plotArea);
    if (!inPlot) {
        // Check if it's near the axis region
        // Extended margin around plot area for axis ticks/labels
        const double axisMargin = 30.0;
        Rect extendedPlot = {
            layout.plotArea.x - axisMargin,
            layout.plotArea.y - axisMargin,
            layout.plotArea.w + 2 * axisMargin,
            layout.plotArea.h + axisMargin + 30.0
        };
        if (pointInRect(px, py, extendedPlot)) {
            result.hitType = HitTestResult::Axis;
            return result;
        }
        result.hitType = HitTestResult::None;
        return result;
    }

    // ── Priority 3: Find nearest data point / curve ──
    double bestDist = DBL_MAX;
    int bestSeries = -1;
    int bestPoint = -1;
    double bestDataX = 0, bestDataY = 0;

    for (size_t si = 0; si < series.size(); si++) {
        const auto& s = series[si];
        if (!s.visible || s.count == 0) continue;

        // Check distance to each line segment and each data point
        for (int i = 0; i < s.count; i++) {
            double dx = dataToDeviceX(s.xs[i], axis, layout.plotArea);
            double dy = dataToDeviceY(s.ys[i], s.yAxisIndex, axis, layout.plotArea);

            // Distance to this data point
            double pdx = px - dx, pdy = py - dy;
            double dist = std::sqrt(pdx * pdx + pdy * pdy);

            if (dist < bestDist) {
                bestDist = dist;
                bestSeries = static_cast<int>(si);
                bestPoint = i;
                bestDataX = s.xs[i];
                bestDataY = s.ys[i];
            }

            // Distance to segment (from this point to next)
            if (i + 1 < s.count) {
                double nx = dataToDeviceX(s.xs[i + 1], axis, layout.plotArea);
                double ny = dataToDeviceY(s.ys[i + 1], s.yAxisIndex, axis, layout.plotArea);
                double segDist = pointToSegmentDist(px, py, dx, dy, nx, ny);
                if (segDist < bestDist) {
                    bestDist = segDist;
                    bestSeries = static_cast<int>(si);
                    // For segment hits, store the closer endpoint
                    double d1 = std::sqrt((px - dx) * (px - dx) + (py - dy) * (py - dy));
                    double d2 = std::sqrt((px - nx) * (px - nx) + (py - ny) * (py - ny));
                    if (d1 <= d2) {
                        bestPoint = i;
                        bestDataX = s.xs[i];
                        bestDataY = s.ys[i];
                    } else {
                        bestPoint = i + 1;
                        bestDataX = s.xs[i + 1];
                        bestDataY = s.ys[i + 1];
                    }
                }
            }
        }
    }

    // Classify the hit
    if (bestSeries >= 0) {
        if (bestDist <= kDataPointHitRadius) {
            result.hitType = HitTestResult::DataPoint;
            result.seriesIndex = bestSeries;
            result.dataPointIndex = bestPoint;
            result.dataX = bestDataX;
            result.dataY = bestDataY;
            result.distancePx = bestDist;
        } else if (bestDist <= kCurveHitRadius) {
            result.hitType = HitTestResult::CurveLine;
            result.seriesIndex = bestSeries;
            result.dataX = bestDataX;
            result.dataY = bestDataY;
            result.distancePx = bestDist;
        } else {
            result.hitType = HitTestResult::PlotArea;
            // Still report data-space coords of the click
            result.dataX = deviceToDataX(px, axis, layout.plotArea);
            result.dataY = deviceToDataY(py, 0, axis, layout.plotArea);
        }
    } else {
        result.hitType = HitTestResult::PlotArea;
        result.dataX = deviceToDataX(px, axis, layout.plotArea);
        result.dataY = deviceToDataY(py, 0, axis, layout.plotArea);
    }

    return result;
}

} // namespace xyplot
