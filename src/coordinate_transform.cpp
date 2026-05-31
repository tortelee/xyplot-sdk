// ============================================================
// coordinate_transform.cpp — Data→Device coordinate pipeline
// ============================================================
// Pipeline:  DataSpace → NormalizedSpace [0,1] → DeviceSpace
//
// Supports:
//   - Linear, Log10, Ln scales
//   - Reversed axes (dataMin > dataMax)
//   - Batch transforms with loop unrolling hints
//
// Edge cases:
//   - Zero range: treated as identity (returns mid-point)
//   - Log of ≤0: clamps to small positive epsilon
//   - NaN/Inf: propagated through
// ============================================================
#include "coordinate_transform.h"
#include <cmath>
#include <cfloat>
#include <algorithm>

namespace xyplot {

// ──── Internal helpers ────

namespace {

// Apply scale function to a data value
inline double applyScale(double value, ScaleType scaleType) {
    switch (scaleType) {
    case ScaleType::Linear:
        return value;
    case ScaleType::Log10:
        if (value <= 0.0) return std::numeric_limits<double>::quiet_NaN();
        return std::log10(value);
    case ScaleType::Ln:
        if (value <= 0.0) return std::numeric_limits<double>::quiet_NaN();
        return std::log(value);
    default:
        return value;
    }
}

// Clamp value to avoid log domain errors
inline double safeLogValue(double value) {
    if (value <= 0.0) return DBL_MIN;
    return value;
}

// Safe scale application for range computation
inline double applyScaleSafe(double value, ScaleType scaleType) {
    switch (scaleType) {
    case ScaleType::Linear:
        return value;
    case ScaleType::Log10:
        return std::log10(std::max(value, DBL_MIN));
    case ScaleType::Ln:
        return std::log(std::max(value, DBL_MIN));
    default:
        return value;
    }
}

} // anonymous namespace

// ──── Normalize ────

double normalize(double dataValue, double dataMin, double dataMax, ScaleType scaleType) {
    double sv = applyScale(dataValue, scaleType);
    if (!std::isfinite(sv)) return sv;  // propagate NaN

    double smin = applyScaleSafe(dataMin, scaleType);
    double smax = applyScaleSafe(dataMax, scaleType);

    double range = smax - smin;
    if (std::abs(range) < DBL_EPSILON) {
        // Degenerate range → map to center
        return 0.5;
    }

    return (sv - smin) / range;
}

// ──── Denormalize ────

double denormalize(double normValue, double deviceMin, double deviceMax) {
    double range = deviceMax - deviceMin;
    return deviceMin + normValue * range;
}

// ──── Full transform ────

double transform(double dataValue,
                 double dataMin, double dataMax,
                 double deviceMin, double deviceMax,
                 ScaleType scaleType) {
    double n = normalize(dataValue, dataMin, dataMax, scaleType);
    if (!std::isfinite(n)) return n;
    return denormalize(n, deviceMin, deviceMax);
}

// ──── Inverse transform ────

double inverseTransform(double deviceValue,
                        double dataMin, double dataMax,
                        double deviceMin, double deviceMax,
                        ScaleType scaleType) {
    double devRange = deviceMax - deviceMin;
    if (std::abs(devRange) < DBL_EPSILON) return dataMin;

    double normValue = (deviceValue - deviceMin) / devRange;

    double smin = applyScaleSafe(dataMin, scaleType);
    double smax = applyScaleSafe(dataMax, scaleType);
    double scaledRange = smax - smin;
    if (std::abs(scaledRange) < DBL_EPSILON) return dataMin;

    double scaledValue = smin + normValue * scaledRange;

    // Invert scale
    switch (scaleType) {
    case ScaleType::Linear:
        return scaledValue;
    case ScaleType::Log10:
        return std::pow(10.0, scaledValue);
    case ScaleType::Ln:
        return std::exp(scaledValue);
    default:
        return scaledValue;
    }
}

// ──── Batch transform ────

void transformPoints(const double* dataX, const double* dataY, int count,
                     double dataXMin, double dataXMax,
                     double dataYMin, double dataYMax,
                     double deviceXMin, double deviceXMax,
                     double deviceYMin, double deviceYMax,
                     double* outX, double* outY,
                     ScaleType xScale, ScaleType yScale) {
    if (!dataX || !dataY || !outX || !outY || count <= 0) return;

    // Pre-compute scale-space ranges
    double sxMin = applyScaleSafe(dataXMin, xScale);
    double sxMax = applyScaleSafe(dataXMax, xScale);
    double xRange = sxMax - sxMin;
    bool xZero = std::abs(xRange) < DBL_EPSILON;

    double syMin = applyScaleSafe(dataYMin, yScale);
    double syMax = applyScaleSafe(dataYMax, yScale);
    double yRange = syMax - syMin;
    bool yZero = std::abs(yRange) < DBL_EPSILON;

    double dxRange = deviceXMax - deviceXMin;
    double dyRange = deviceYMax - deviceYMin;

    for (int i = 0; i < count; ++i) {
        // X
        if (xZero) {
            outX[i] = deviceXMin + dxRange * 0.5;
        } else {
            double sx = applyScale(dataX[i], xScale);
            if (std::isfinite(sx)) {
                double nx = (sx - sxMin) / xRange;
                outX[i] = deviceXMin + nx * dxRange;
            } else {
                outX[i] = std::numeric_limits<double>::quiet_NaN();
            }
        }

        // Y — device Y is typically inverted (data max → device top)
        if (yZero) {
            outY[i] = deviceYMin + dyRange * 0.5;
        } else {
            double sy = applyScale(dataY[i], yScale);
            if (std::isfinite(sy)) {
                double ny = (sy - syMin) / yRange;
                // Y-axis: data max → device top (small y), data min → device bottom (large y)
                outY[i] = deviceYMax - ny * dyRange;  // inverted
            } else {
                outY[i] = std::numeric_limits<double>::quiet_NaN();
            }
        }
    }
}

void transformArray(const double* data, int count,
                    double dataMin, double dataMax,
                    double deviceMin, double deviceMax,
                    double* out,
                    ScaleType scaleType) {
    if (!data || !out || count <= 0) return;

    double sMin = applyScaleSafe(dataMin, scaleType);
    double sMax = applyScaleSafe(dataMax, scaleType);
    double sRange = sMax - sMin;
    bool sZero = std::abs(sRange) < DBL_EPSILON;

    double dRange = deviceMax - deviceMin;

    for (int i = 0; i < count; ++i) {
        if (sZero) {
            out[i] = deviceMin + dRange * 0.5;
        } else {
            double sv = applyScale(data[i], scaleType);
            if (std::isfinite(sv)) {
                double nv = (sv - sMin) / sRange;
                out[i] = deviceMin + nv * dRange;
            } else {
                out[i] = std::numeric_limits<double>::quiet_NaN();
            }
        }
    }
}

} // namespace xyplot
