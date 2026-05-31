// ============================================================
// axis_system.cpp — Axis tick computation implementation
// ============================================================
// Nice Number algorithm based on standard practices (cf. matplotlib.ticker,
// D3-scale, GNOME libnice).
//
// Core idea:
//   Given range [min, max] and desired tick count n:
//   1. roughInterval = (max - min) / n
//   2. niceInterval  = niceNumber(roughInterval, round=true)
//   3. niceMin       = floor(min / niceInterval) * niceInterval
//   4. niceMax       = ceil(max / niceInterval) * niceInterval
//   5. Generate ticks at niceInterval steps from niceMin to niceMax
// ============================================================
#include "axis_system.h"
#include <cmath>
#include <cfloat>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace xyplot {

// ──── Nice Number ────

double niceNumber(double x, bool round) {
    // Handle zero and degenerate input
    if (x <= 0.0) return x <= 0.0 ? 1.0 : x;
    if (!std::isfinite(x)) return 1.0;

    // 10^exp ≤ x < 10^(exp+1)
    double exp   = std::floor(std::log10(x));
    double frac  = x / std::pow(10.0, exp);

    // Map fraction to nice numbers: 1, 2, 2.5, 5, 10
    double niceFrac;
    if (round) {
        // Round up version — used for the interval itself
        if      (frac <= 1.0)  niceFrac = 1.0;
        else if (frac <= 2.0)  niceFrac = 2.0;
        else if (frac <= 2.5)  niceFrac = 2.5;
        else if (frac <= 5.0)  niceFrac = 5.0;
        else                   niceFrac = 10.0;
    } else {
        // Ceiling version — used for clamping min/max
        if      (frac < 1.5)   niceFrac = 1.0;
        else if (frac < 3.0)   niceFrac = 2.0;
        else if (frac < 5.5)   niceFrac = 5.0;
        else                   niceFrac = 10.0;
    }

    return niceFrac * std::pow(10.0, exp);
}

// ──── Auto precision ────

int autoPrecision(double tickInterval) {
    if (tickInterval <= 0.0 || !std::isfinite(tickInterval)) return 0;

    double absInterval = std::abs(tickInterval);
    double log10 = std::log10(absInterval);

    // If interval ≥ 1, we usually don't need decimals
    // If interval < 1, we need enough decimals to represent it
    if (log10 >= 0.0) {
        // interval ≥ 1 — check if it's a "clean" integer
        double intPart;
        double fracPart = std::modf(absInterval, &intPart);
        if (fracPart < 1e-10) return 0;          // integer → no decimals
        return static_cast<int>(std::ceil(-log10)); // fractional
    } else {
        // interval < 1 — need decimal places
        return static_cast<int>(std::ceil(-log10));
    }
}

// ──── Label formatting ────

std::string formatTickLabel(double value, int precision) {
    if (!std::isfinite(value)) return "NaN";

    double absVal = std::abs(value);

    // Scientific notation for very large or very small values
    if (absVal >= 1e9) {
        // e.g., 1.5e9 → "1.5e9"
        std::ostringstream oss;
        oss << std::scientific << std::setprecision(2) << value;
        return oss.str();
    }
    if (absVal > 0.0 && absVal < 1e-6) {
        std::ostringstream oss;
        oss << std::scientific << std::setprecision(2) << value;
        return oss.str();
    }

    // Auto-precision: choose based on value magnitude
    if (precision < 0) {
        if (absVal >= 1.0) {
            double intPart;
            double fracPart = std::modf(absVal, &intPart);
            precision = (fracPart < 1e-10) ? 0 : 6;
        } else {
            precision = 6;
        }
    }

    // Trim trailing zeros for fractional values
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(precision) << value;
    std::string s = oss.str();

    // Remove unnecessary trailing zeros
    if (s.find('.') != std::string::npos) {
        while (s.size() > 1 && s.back() == '0') s.pop_back();
        if (s.back() == '.') s.pop_back();
    }

    return s;
}

// ──── Tick computation ────

AxisTicks computeTicks(const AxisConfig& config) {
    AxisTicks result;
    double dMin = config.dataMin;
    double dMax = config.dataMax;

    // Handle degenerate range
    if (dMin == dMax) {
        dMin -= 1.0;
        dMax += 1.0;
    }

    // Handle reversed axes
    double rangeMin = std::min(dMin, dMax);
    double rangeMax = std::max(dMin, dMax);

    // ──── Log scale: compute in log space ────
    if (config.scaleType == ScaleType::Log10 || config.scaleType == ScaleType::Ln) {
        double logBase = (config.scaleType == ScaleType::Log10) ? 10.0 : std::exp(1.0);

        double logMin = std::log10(rangeMin) / std::log10(logBase);
        double logMax = std::log10(rangeMax) / std::log10(logBase);

        // Handle invalid log inputs
        if (!std::isfinite(logMin)) logMin = -3.0;
        if (!std::isfinite(logMax)) logMax = 3.0;

        double logRange = logMax - logMin;
        if (logRange <= 0) logRange = 1.0;

        double roughInterval = logRange / static_cast<double>(config.targetMajorTicks);
        double niceInterval = niceNumber(roughInterval, true);

        double niceLogMin = std::floor(logMin / niceInterval) * niceInterval;
        double niceLogMax = std::ceil(logMax / niceInterval) * niceInterval;

        // Generate log-space ticks
        for (double lv = niceLogMin; lv <= niceLogMax + 0.5 * niceInterval; lv += niceInterval) {
            double dataVal = std::pow(logBase, lv);
            // Filter out values too close together in data space
            if (!result.majorTicks.empty()) {
                double last = result.majorTicks.back();
                if (std::abs(dataVal - last) < std::abs(last) * 1e-10) continue;
            }
            result.majorTicks.push_back(dataVal);
            result.labels.push_back(formatTickLabel(dataVal,
                autoPrecision(niceInterval)));
        }

        // Remove duplicates from floating-point drift
        auto last = std::unique(result.majorTicks.begin(), result.majorTicks.end(),
            [](double a, double b) { return std::abs(a - b) < 1e-12 * std::max(1.0, std::abs(a)); });
        result.majorTicks.erase(last, result.majorTicks.end());

        // Rebuild labels after dedup
        result.labels.clear();
        for (auto v : result.majorTicks) {
            result.labels.push_back(formatTickLabel(v));
        }

        result.niceMin = result.majorTicks.empty() ? rangeMin : result.majorTicks.front();
        result.niceMax = result.majorTicks.empty() ? rangeMax : result.majorTicks.back();
        return result;
    }

    // ──── Linear scale ────
    double range = rangeMax - rangeMin;
    if (range <= 0) range = 1.0;

    double roughInterval = range / static_cast<double>(config.targetMajorTicks);
    double niceInterval  = niceNumber(roughInterval, true);

    // Avoid zero or near-zero intervals
    if (niceInterval < DBL_EPSILON) niceInterval = 1.0;

    double niceMin = std::floor(rangeMin / niceInterval) * niceInterval;
    double niceMax = std::ceil(rangeMax / niceInterval) * niceInterval;

    // Generate major ticks
    int    maxTicks  = static_cast<int>((niceMax - niceMin) / niceInterval) + 2;
    double lastTick  = niceMin - niceInterval;
    for (int i = 0; i <= maxTicks; ++i) {
        double v = niceMin + static_cast<double>(i) * niceInterval;
        if (v > niceMax + 0.5 * niceInterval) break;

        // Avoid duplicate floating-point values
        if (std::abs(v - lastTick) < niceInterval * 1e-10) continue;
        lastTick = v;

        // Slightly extend beyond nice range if data requires it
        result.majorTicks.push_back(v);
    }

    // Generate minor ticks (between each major pair)
    if (config.targetMinorTicks > 0 && result.majorTicks.size() >= 2) {
        int subdivs = config.targetMinorTicks + 1;
        for (size_t i = 0; i + 1 < result.majorTicks.size(); ++i) {
            double start = result.majorTicks[i];
            double end   = result.majorTicks[i + 1];
            for (int s = 1; s < subdivs; ++s) {
                double mv = start + (end - start) * static_cast<double>(s) / static_cast<double>(subdivs);
                result.minorTicks.push_back(mv);
            }
        }
    }

    // Generate labels
    int prec = autoPrecision(niceInterval);
    result.labels.reserve(result.majorTicks.size());
    for (double v : result.majorTicks) {
        result.labels.push_back(formatTickLabel(v, prec));
    }

    result.niceMin = niceMin;
    result.niceMax = niceMax;

    return result;
}

} // namespace xyplot
