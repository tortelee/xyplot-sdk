// ============================================================
// coordinate_transform.h — Data→Device coordinate pipeline
// ============================================================
// Owner: Agent C (核心算法)
// Consumers: Agent D (图类型 — LinePlot, ScatterPlot)
// Depends on: types.h (ScaleType) — frozen
//
// Pure computation — no IRenderDevice needed.
//
// Pipeline:  DataSpace → NormalizedSpace [0,1] → DeviceSpace (pixels)
//
// Supports:
//   - Linear, Log10, Ln scales
//   - Reversed axes (dataMin > dataMax)
//   - Batch transform for efficiency
// ============================================================
#pragma once
#include "xyplot/types.h"
#include <cstddef>

namespace xyplot {

// ──── Single-value transforms ────

/// Map data value to [0,1] using the given scale type.
/// Returns NaN for invalid log inputs (≤ 0).
double normalize(double dataValue, double dataMin, double dataMax,
                 ScaleType scaleType = ScaleType::Linear);

/// Map normalized [0,1] value to device space.
double denormalize(double normValue, double deviceMin, double deviceMax);

/// Full transform: DataSpace → DeviceSpace (normalize + denormalize).
double transform(double dataValue,
                 double dataMin, double dataMax,
                 double deviceMin, double deviceMax,
                 ScaleType scaleType = ScaleType::Linear);

/// Inverse transform: DeviceSpace → DataSpace.
double inverseTransform(double deviceValue,
                        double dataMin, double dataMax,
                        double deviceMin, double deviceMax,
                        ScaleType scaleType = ScaleType::Linear);

// ──── Batch transforms ────

/// Transform x and y arrays from data space to device space.
/// outX and outY must have room for `count` doubles.
void transformPoints(const double* dataX, const double* dataY, int count,
                     double dataXMin, double dataXMax,
                     double dataYMin, double dataYMax,
                     double deviceXMin, double deviceXMax,
                     double deviceYMin, double deviceYMax,
                     double* outX, double* outY,
                     ScaleType xScale = ScaleType::Linear,
                     ScaleType yScale = ScaleType::Linear);

/// Transform one array (e.g., only X or only Y values).
void transformArray(const double* data, int count,
                    double dataMin, double dataMax,
                    double deviceMin, double deviceMax,
                    double* out,
                    ScaleType scaleType = ScaleType::Linear);

} // namespace xyplot
