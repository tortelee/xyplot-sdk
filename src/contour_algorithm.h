// ============================================================
// contour_algorithm.h — Marching Squares contour extraction
// ============================================================
// Owner: Agent C (核心算法)
// Phase: B2 — 扩展 IRenderDevice，新增 3 种图类型
// Consumer: Agent D (ContourPlot)
//
// Marching Squares: extract isoline segments from a 2D scalar grid.
// Input:  grid[row×col] + isovalue → Output: contour polylines.
//
// Pure computation — no IRenderDevice dependency.
// ============================================================
#pragma once
#include <vector>
#include <cstddef>

namespace xyplot {

// ──── Contour segment (raw output of Marching Squares) ────
struct ContourSegment {
    double x1 = 0, y1 = 0;
    double x2 = 0, y2 = 0;
};

// ──── Contour path (linked segments forming a polyline) ────
struct ContourPath {
    std::vector<double> xs;
    std::vector<double> ys;
    bool isClosed = false;
};

// ──── API ────

/// Run Marching Squares on a scalar grid for a single isovalue.
/// Returns raw line segments. Each segment is a 2-point line crossing
/// one grid cell where the isovalue lies between corner values.
///
/// @param grid     row-major grid data, rows × cols
/// @param rows     number of grid rows
/// @param cols     number of grid columns
/// @param x0, y0   grid origin in data space
/// @param dx, dy   cell spacing in x and y directions
/// @param isovalue contour value to extract
std::vector<ContourSegment>
computeContourSegments(const double* grid, int rows, int cols,
                       double x0, double y0, double dx, double dy,
                       double isovalue);

/// Same as computeContourSegments, but links segments into connected
/// polylines. Paths are maximally extended and marked as open or closed.
///
/// Linking uses endpoint proximity (1e-9 × cell diagonal) and is O(n²)
/// worst-case, but practical for typical grid resolutions.
std::vector<ContourPath>
computeContours(const double* grid, int rows, int cols,
                double x0, double y0, double dx, double dy,
                double isovalue);

/// Compute evenly spaced contour levels between min and max.
/// Includes both endpoints. For N levels, returns N values.
/// Example: computeContourLevels(1, 10, 5) → {1, 3.25, 5.5, 7.75, 10}
std::vector<double> computeContourLevels(double dataMin, double dataMax,
                                         int numLevels);

} // namespace xyplot
