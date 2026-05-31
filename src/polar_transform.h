// ============================================================
// polar_transform.h — Polar coordinate transform
// ============================================================
// Owner: Agent C (核心算法)
// Phase: B1 — 零接口变更 P1 图类型扩展
// Consumer: Agent D (PolarPlot)
//
// Converts polar coordinates (r, θ) to Cartesian (x, y).
// θ is in radians. Standard convention: θ=0 → +x, θ=π/2 → +y.
//
// Pure computation — no IRenderDevice dependency.
// ============================================================
#pragma once
#include <cstddef>

namespace xyplot {

/// Convert a single polar coordinate to Cartesian.
/// @param  r       radius (negative values flipped to positive with θ += π)
/// @param  theta   angle in radians
/// @param  x       output: Cartesian x
/// @param  y       output: Cartesian y
void polarToCartesian(double r, double theta, double& x, double& y);

/// Batch convert polar coordinates to Cartesian.
/// outX and outY must have room for `count` doubles.
void polarToCartesianBatch(const double* r, const double* theta,
                           int count,
                           double* outX, double* outY);

} // namespace xyplot
