// ============================================================
// polar_transform.cpp — Polar coordinate transform implementation
// ============================================================
// Converts polar coordinates (r, θ) to Cartesian (x, y):
//   x = r × cos(θ)
//   y = r × sin(θ)
//
// Edge cases:
//   - r = 0       → (0, 0)
//   - r < 0       → normalized: |r|, θ + π
//   - NaN/Inf     → propagated to output
//   - Large θ     → cos/sin are periodic, handled natively
// ============================================================
#include "polar_transform.h"
#include <cmath>
#include <limits>

namespace xyplot {

void polarToCartesian(double r, double theta, double& x, double& y) {
    // Handle NaN/Inf: propagate
    if (!std::isfinite(r) || !std::isfinite(theta)) {
        x = std::numeric_limits<double>::quiet_NaN();
        y = std::numeric_limits<double>::quiet_NaN();
        return;
    }

    // Handle zero radius
    if (r == 0.0) {
        x = 0.0;
        y = 0.0;
        return;
    }

    // Normalize negative radius: negate r, shift angle by π
    double effectiveR   = r;
    double effectiveTheta = theta;
    if (r < 0.0) {
        effectiveR = -r;
        effectiveTheta = theta + 3.14159265358979323846; // +π
    }

    x = effectiveR * std::cos(effectiveTheta);
    y = effectiveR * std::sin(effectiveTheta);
}

void polarToCartesianBatch(const double* r, const double* theta,
                           int count,
                           double* outX, double* outY) {
    if (!r || !theta || !outX || !outY || count <= 0) return;

    for (int i = 0; i < count; ++i) {
        polarToCartesian(r[i], theta[i], outX[i], outY[i]);
    }
}

} // namespace xyplot
