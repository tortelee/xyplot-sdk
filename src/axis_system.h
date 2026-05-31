// ============================================================
// axis_system.h — Axis tick computation (Nice Number algorithm)
// ============================================================
// Owner: Agent C (核心算法)
// Consumers: Agent D (图类型), Agent E (Plot 门面)
// Depends on: types.h (ScaleType) — frozen
//
// Key algorithm:
//   NiceNumber(x, round): maps any interval to a "nice" interval
//     {1, 2, 2.5, 5, 10} × 10^n
//   computeTicks(config): full auto-tick pipeline
//     range → niceInterval → niceMin/niceMax → tick values → labels
// ============================================================
#pragma once
#include "xyplot/types.h"
#include <vector>
#include <string>

namespace xyplot {

// ──── Tick info ────
struct TickInfo {
    double value = 0.0;
    bool   isMajor = true;  // true = major tick, false = minor
};

// ──── Axis configuration ────
struct AxisConfig {
    double    dataMin          = 0.0;
    double    dataMax          = 1.0;
    ScaleType scaleType        = ScaleType::Linear;
    int       targetMajorTicks = 5;          // desired number of major ticks
    int       targetMinorTicks = 1;          // minor divisions between majors (0 = no minors)
};

// ──── Computed axis ticks ────
struct AxisTicks {
    std::vector<double>      majorTicks;   // tick values in data space
    std::vector<double>      minorTicks;   // minor tick values (may be empty)
    std::vector<std::string> labels;       // formatted labels, 1:1 with majorTicks
    double                   niceMin = 0;  // nice range lower bound
    double                   niceMax = 1;  // nice range upper bound
};

// ──── API ────

/// Compute ticks and labels for a data range.
/// Handles Linear, Log10, Ln scale types.
/// For degenerate ranges (min == max), expands by ±1.
AxisTicks computeTicks(const AxisConfig& config);

/// Nice Number algorithm: rounds `x` to the nearest "nice" number.
/// `round=true`: used after computing the interval → picks a clean interval.
/// `round=false`: used for min/max clamping → picks a conservative bound.
double niceNumber(double x, bool round);

/// Format a tick value for human-readable display.
/// Handles: SI prefixes (k, M, G, m, μ, n), scientific notation for extremes.
/// `precision`: decimal digits. -1 = auto-detect based on tick spacing.
std::string formatTickLabel(double value, int precision = -1);

/// Compute suitable decimal precision for a given tick interval.
/// e.g. interval=0.2 → 1, interval=1.0 → 0, interval=50 → -1 (round to 10s)
int autoPrecision(double tickInterval);

} // namespace xyplot
