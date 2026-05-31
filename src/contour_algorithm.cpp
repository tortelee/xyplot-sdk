// ============================================================
// contour_algorithm.cpp — Marching Squares contour extraction
// ============================================================
// Classic Marching Squares for isoline extraction from 2D grids.
//
// Algorithm:
//   1. For each 2×2 cell in the grid, compute a 4-bit case index
//      based on which corners are above the isovalue.
//   2. Look up which cell edges the contour crosses.
//   3. Linearly interpolate the crossing point along each edge.
//   4. Optionally link segments into connected polylines.
//
// 16 cases: 2 with no crossing, 12 with one segment, 2 saddles (two segments).
//
// Edge numbering (looking at the cell):
//       2 (top)
//   ┌───────────┐
//   │           │
// 3 │           │ 1
//   │           │
//   └───────────┘
//       0 (bottom)
//
// Corners: TL=bit0(1), TR=bit1(2), BR=bit2(4), BL=bit3(8)
// ============================================================
#include "contour_algorithm.h"
#include <cmath>
#include <cfloat>
#include <algorithm>
#include <utility>

namespace xyplot {

// ──── Marching Squares case table ────
// For each of 16 cases, up to 2 segments as (edge_a, edge_b) pairs.
// -1 = no segment.
namespace {

// clang-format off
static const int CASE_EDGES[16][4] = {
    {-1, -1, -1, -1},  //  0: all below → no contour
    { 2,  3, -1, -1},  //  1: TL only
    { 1,  2, -1, -1},  //  2: TR only
    { 1,  3, -1, -1},  //  3: TL+TR (top half)
    { 0,  1, -1, -1},  //  4: BR only
    { 0,  3,  1,  2},  //  5: TL+BR (saddle) → two segments
    { 0,  2, -1, -1},  //  6: TR+BR (right half)
    { 0,  3, -1, -1},  //  7: TL+TR+BR → BL outsider
    { 3,  0, -1, -1},  //  8: BL only
    { 0,  2, -1, -1},  //  9: TL+BL (left half)
    { 0,  1,  3,  2},  // 10: TR+BL (saddle) → two segments
    { 0,  1, -1, -1},  // 11: TL+TR+BL → BR outsider
    { 2,  3, -1, -1},  // 12: BR+BL (bottom half)
    { 1,  2, -1, -1},  // 13: TL+BR+BL → TR outsider
    { 3,  0, -1, -1},  // 14: TR+BR+BL → TL outsider
    {-1, -1, -1, -1},  // 15: all above → no contour
};
// clang-format on

/// Compute the 4-bit case index for a cell.
/// Corners: TL=bit0, TR=bit1, BR=bit2, BL=bit3.
inline int cellCase(double vTL, double vTR, double vBR, double vBL,
                    double iso) {
    int c = 0;
    if (vTL >= iso) c |= 1;   // bit 0
    if (vTR >= iso) c |= 2;   // bit 1
    if (vBR >= iso) c |= 4;   // bit 2
    if (vBL >= iso) c |= 8;   // bit 3
    return c;
}

/// Linearly interpolate position along edge `edgeIdx` of cell (row, col).
/// Returns (px, py) in data coordinates.
inline void interpolateEdge(int edgeIdx,
                             int col, int row,
                             double x0, double y0, double dx, double dy,
                             double vTL, double vTR, double vBR, double vBL,
                             double iso,
                             double& px, double& py) {
    double t;
    double cx = x0 + static_cast<double>(col) * dx;
    double cy = y0 + static_cast<double>(row) * dy;

    switch (edgeIdx) {
    case 0: // bottom edge: BL → BR (horizontal, y = cy + dy)
        t  = (iso - vBL) / (vBR - vBL);
        px = cx + t * dx;
        py = cy + dy;
        break;
    case 1: // right edge: BR → TR (vertical, x = cx + dx)
        t  = (iso - vBR) / (vTR - vBR);
        px = cx + dx;
        py = cy + dy - t * dy;  // going upward
        break;
    case 2: // top edge: TR → TL (horizontal, y = cy)
        t  = (iso - vTR) / (vTL - vTR);
        px = cx + dx - t * dx;  // going leftward
        py = cy;
        break;
    case 3: // left edge: TL → BL (vertical, x = cx)
        t  = (iso - vTL) / (vBL - vTL);
        px = cx;
        py = cy + t * dy;       // going downward
        break;
    default:
        px = 0; py = 0;
        break;
    }

    // Clamp t to [0,1] to handle floating-point edge cases
    // This prevents segments from leaving the cell
}

// Clamp helper: re-clamp edge interpolation to cell bounds
inline void clampToCell(int edgeIdx,
                         int col, int row,
                         double x0, double y0, double dx, double dy,
                         double& px, double& py) {
    double cx = x0 + static_cast<double>(col) * dx;
    double cy = y0 + static_cast<double>(row) * dy;

    switch (edgeIdx) {
    case 0: py = cy + dy; px = std::max(cx, std::min(cx + dx, px)); break;
    case 1: px = cx + dx; py = std::max(cy, std::min(cy + dy, py)); break;
    case 2: py = cy;      px = std::max(cx, std::min(cx + dx, px)); break;
    case 3: px = cx;      py = std::max(cy, std::min(cy + dy, py)); break;
    default: break;
    }
}

} // anonymous namespace

// ──── Compute segments ────

std::vector<ContourSegment>
computeContourSegments(const double* grid, int rows, int cols,
                       double x0, double y0, double dx, double dy,
                       double isovalue) {
    std::vector<ContourSegment> segments;

    if (!grid || rows < 2 || cols < 2) return segments;
    if (!std::isfinite(isovalue)) return segments;

    // Reserve space: average ~1 segment per 4 cells for typical grids
    segments.reserve(static_cast<size_t>(rows * cols) / 2);

    for (int r = 0; r < rows - 1; ++r) {
        for (int c = 0; c < cols - 1; ++c) {
            // Corner values
            double vTL = grid[r * cols + c];
            double vTR = grid[r * cols + c + 1];
            double vBR = grid[(r + 1) * cols + c + 1];
            double vBL = grid[(r + 1) * cols + c];

            // Skip cells with NaN corners
            if (!std::isfinite(vTL) || !std::isfinite(vTR) ||
                !std::isfinite(vBR) || !std::isfinite(vBL))
                continue;

            int kase = cellCase(vTL, vTR, vBR, vBL, isovalue);
            const int* edges = CASE_EDGES[kase];

            // Process up to 2 segments
            for (int seg = 0; seg < 2; ++seg) {
                int eA = edges[seg * 2];
                int eB = edges[seg * 2 + 1];
                if (eA < 0 || eB < 0) break;

                double px1, py1, px2, py2;
                interpolateEdge(eA, c, r, x0, y0, dx, dy,
                                vTL, vTR, vBR, vBL, isovalue, px1, py1);
                clampToCell(eA, c, r, x0, y0, dx, dy, px1, py1);

                interpolateEdge(eB, c, r, x0, y0, dx, dy,
                                vTL, vTR, vBR, vBL, isovalue, px2, py2);
                clampToCell(eB, c, r, x0, y0, dx, dy, px2, py2);

                segments.push_back({px1, py1, px2, py2});
            }
        }
    }

    return segments;
}

// ──── Compute linked contours ────

std::vector<ContourPath>
computeContours(const double* grid, int rows, int cols,
                double x0, double y0, double dx, double dy,
                double isovalue) {
    // Step 1: get raw segments
    auto segments = computeContourSegments(grid, rows, cols,
                                           x0, y0, dx, dy, isovalue);
    if (segments.empty()) return {};

    // Step 2: compute linking tolerance
    double diag = std::sqrt(dx * dx + dy * dy);
    double tol  = std::max(1e-12, diag * 1e-9);

    // Step 3: greedy path linking
    std::vector<bool>              used(segments.size(), false);
    std::vector<ContourPath>       paths;

    for (size_t i = 0; i < segments.size(); ++i) {
        if (used[i]) continue;

        used[i] = true;
        ContourPath path;
        path.xs.push_back(segments[i].x1);
        path.ys.push_back(segments[i].y1);
        path.xs.push_back(segments[i].x2);
        path.ys.push_back(segments[i].y2);

        // Extend the path forward from the last endpoint
        bool extended = true;
        while (extended) {
            extended = false;
            double endX = path.xs.back();
            double endY = path.ys.back();

            for (size_t j = 0; j < segments.size(); ++j) {
                if (used[j]) continue;

                double d1 = std::sqrt(
                    (segments[j].x1 - endX) * (segments[j].x1 - endX) +
                    (segments[j].y1 - endY) * (segments[j].y1 - endY));

                double d2 = std::sqrt(
                    (segments[j].x2 - endX) * (segments[j].x2 - endX) +
                    (segments[j].y2 - endY) * (segments[j].y2 - endY));

                if (d1 < tol) {
                    used[j] = true;
                    path.xs.push_back(segments[j].x2);
                    path.ys.push_back(segments[j].y2);
                    extended = true;
                    break;
                } else if (d2 < tol) {
                    used[j] = true;
                    path.xs.push_back(segments[j].x1);
                    path.ys.push_back(segments[j].y1);
                    extended = true;
                    break;
                }
            }
        }

        // Determine if closed (start ≈ end)
        if (path.xs.size() >= 3) {
            double dClose = std::sqrt(
                (path.xs.front() - path.xs.back()) * (path.xs.front() - path.xs.back()) +
                (path.ys.front() - path.ys.back()) * (path.ys.front() - path.ys.back()));
            path.isClosed = (dClose < tol);
        }

        paths.push_back(std::move(path));
    }

    return paths;
}

// ──── Compute contour levels ────

std::vector<double> computeContourLevels(double dataMin, double dataMax,
                                         int numLevels) {
    std::vector<double> levels;
    if (numLevels < 1) return levels;
    if (!std::isfinite(dataMin) || !std::isfinite(dataMax)) return levels;
    if (dataMin == dataMax) {
        // Degenerate range: return single level
        levels.push_back(dataMin);
        return levels;
    }

    double lo = std::min(dataMin, dataMax);
    double hi = std::max(dataMin, dataMax);

    levels.reserve(numLevels);
    if (numLevels == 1) {
        levels.push_back((lo + hi) * 0.5);
    } else {
        for (int i = 0; i < numLevels; ++i) {
            double t = static_cast<double>(i) / static_cast<double>(numLevels - 1);
            levels.push_back(lo + t * (hi - lo));
        }
    }
    return levels;
}

} // namespace xyplot
