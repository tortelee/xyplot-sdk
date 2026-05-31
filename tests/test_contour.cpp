// ============================================================
// test_contour.cpp — Marching Squares contour algorithm tests
// ============================================================
// Owner: Agent C
// Phase: B2 — Contour algorithm
// Tests: computeContourSegments, computeContours, computeContourLevels
// ============================================================
#include "../src/contour_algorithm.h"
#include <cassert>
#include <cstdio>
#include <cmath>
#include <algorithm>

static int g_testsPassed = 0;
static int g_testsFailed = 0;

#define TEST(name) \
    do { printf("  TEST: %s ... ", name); fflush(stdout); } while(0)

#define PASS() \
    do { printf("PASSED\n"); g_testsPassed++; } while(0)

#define CHECK(cond) \
    do { \
        if (!(cond)) { \
            printf("FAILED\n    Assertion: %s\n", #cond); \
            g_testsFailed++; \
            return; \
        } \
    } while(0)

#define CHECK_CLOSE(a, b, eps) \
    do { \
        double _a = (a), _b = (b); \
        if (std::abs(_a - _b) > (eps)) { \
            printf("FAILED\n    Expected ~%g, got %g (eps=%g)\n", _b, _a, (double)(eps)); \
            g_testsFailed++; \
            return; \
        } \
    } while(0)

// ──── Test 1: Constant grid → no segments ────
void test_constant_grid() {
    TEST("constant grid (all 5.0, iso=3.0) → 0 segments");
    double grid[9] = {5,5,5, 5,5,5, 5,5,5};  // 3×3
    auto segs = xyplot::computeContourSegments(grid, 3, 3,
                                               0,0, 1,1, 3.0);
    CHECK(segs.empty());

    // All below
    auto segs2 = xyplot::computeContourSegments(grid, 3, 3,
                                                 0,0, 1,1, 10.0);
    CHECK(segs2.empty());
    PASS();
}

// ──── Test 2: 2×2, one corner above → 1 segment ────
void test_one_corner_above() {
    TEST("2×2 grid, TL above iso → 1 segment crossing top and left edges");
    // Grid values: TL=10 (above), others=0 (below), iso=5
    double grid[4] = {10, 0,  0, 0};  // 2×2 row-major
    auto segs = xyplot::computeContourSegments(grid, 2, 2,
                                               0,0, 1,1, 5.0);
    CHECK(segs.size() == 1);

    auto& s = segs[0];
    // Case 1: edges 2(top) and 3(left)
    // Edge 2 (top): y≈0, x between 0 and 1
    CHECK_CLOSE(s.y1, 0.0, 0.01);
    // Edge 3 (left): x≈0, y between 0 and 1
    double xOnLeft  = std::abs(s.x1) < 0.01 ? s.x1 : s.x2;
    double xOnTop   = std::abs(s.x1) < 0.01 ? s.x2 : s.x1;
    double yOnLeft  = std::abs(s.x1) < 0.01 ? s.y1 : s.y2;
    double yOnTop   = std::abs(s.x1) < 0.01 ? s.y2 : s.y1;

    CHECK_CLOSE(xOnLeft, 0.0, 0.01);
    CHECK_CLOSE(yOnTop,  0.0, 0.01);
    // Interpolated positions
    CHECK(yOnLeft > 0.0 && yOnLeft < 1.0);
    CHECK(xOnTop  > 0.0 && xOnTop  < 1.0);
    PASS();
}

// ──── Test 3: 3×3 center peak → closed contour ────
void test_center_peak_closed() {
    TEST("3×3 center peak → closed contour around center");
    // Center high, border low
    double grid[9] = {0,0,0, 0,10,0, 0,0,0};
    auto paths = xyplot::computeContours(grid, 3, 3,
                                         0,0, 1,1, 5.0);
    CHECK(!paths.empty());

    // Should find a closed contour around the center
    bool hasClosed = false;
    for (auto& p : paths) {
        if (p.isClosed) { hasClosed = true; break; }
    }
    CHECK(hasClosed);

    // All contour points should be within [0,2] in both x and y
    for (auto& p : paths) {
        for (size_t i = 0; i < p.xs.size(); ++i) {
            CHECK(p.xs[i] >= -0.01 && p.xs[i] <= 2.01);
            CHECK(p.ys[i] >= -0.01 && p.ys[i] <= 2.01);
        }
    }
    PASS();
}

// ──── Test 4: 2×2 saddle point → 2 segments ────
void test_saddle_point() {
    TEST("2×2 saddle (TL+BR above) → 2 segments");
    // TL=10, BR=10 (above), TR=0, BL=0 (below), iso=5
    double grid[4] = {10, 0,  0, 10};  // 2×2: TL=10, TR=0, BL=0, BR=10
    auto segs = xyplot::computeContourSegments(grid, 2, 2,
                                               0,0, 1,1, 5.0);
    // Case 5: two segments
    CHECK(segs.size() == 2);
    PASS();
}

// ──── Test 5: 4×1 grid (rows < 2) → 0 segments ────
void test_too_few_rows() {
    TEST("4×1 grid (cols<2) → 0 segments");
    double grid[4] = {0, 1, 2, 3};
    auto segs = xyplot::computeContourSegments(grid, 4, 1,
                                               0,0, 1,1, 1.5);
    CHECK(segs.empty());
    PASS();
}

// ──── Test 6: 3×3, vertical split → contour along x=1 ────
void test_vertical_split() {
    TEST("3×3 vertical split → contour along x≈1");
    // Left column below (0), middle and right above (10), iso=5
    double grid[9] = {0,10,10, 0,10,10, 0,10,10};
    auto segs = xyplot::computeContourSegments(grid, 3, 3,
                                               0,0, 1,1, 5.0);
    CHECK(!segs.empty());

    // All segments should be near x≈1 (between col 0 and col 1)
    for (auto& s : segs) {
        double avgX = (s.x1 + s.x2) * 0.5;
        CHECK(avgX > 0.4 && avgX < 1.6); // near x=1
    }
    PASS();
}

// ──── Test 7: 2×2, three corners above → 1 segment ────
void test_three_corners_above() {
    TEST("2×2 grid, TL+TR+BR above → 1 segment");
    // Case 7: only BL is below
    double grid[4] = {10, 10,  0, 10};  // BL=0, others=10
    auto segs = xyplot::computeContourSegments(grid, 2, 2,
                                               0,0, 1,1, 5.0);
    CHECK(segs.size() == 1);
    // Contour separates BL, crossing bottom(edge0) and left(edge3)
    auto& s = segs[0];
    // Both endpoints should involve bottom (y=1) or left (x=0) edges
    bool hasBottomOrLeft = false;
    if ((std::abs(s.y1 - 1.0) < 0.01 || std::abs(s.x1 - 0.0) < 0.01) &&
        (std::abs(s.y2 - 1.0) < 0.01 || std::abs(s.x2 - 0.0) < 0.01))
        hasBottomOrLeft = true;
    CHECK(hasBottomOrLeft);
    PASS();
}

// ──── Test 8: computeContourLevels — normal range ────
void test_levels_normal() {
    TEST("computeContourLevels 0→100, 5 levels");
    auto levels = xyplot::computeContourLevels(0, 100, 5);
    CHECK(levels.size() == 5);
    CHECK_CLOSE(levels[0],   0.0, 1e-10);
    CHECK_CLOSE(levels[1],  25.0, 1e-10);
    CHECK_CLOSE(levels[2],  50.0, 1e-10);
    CHECK_CLOSE(levels[3],  75.0, 1e-10);
    CHECK_CLOSE(levels[4], 100.0, 1e-10);
    PASS();
}

// ──── Test 9: computeContourLevels — 1 level → midpoint ────
void test_levels_single() {
    TEST("computeContourLevels 3→7, 1 level → midpoint");
    auto levels = xyplot::computeContourLevels(3, 7, 1);
    CHECK(levels.size() == 1);
    CHECK_CLOSE(levels[0], 5.0, 1e-10);
    PASS();
}

// ──── Test 10: computeContourLevels — degenerate range ────
void test_levels_degenerate() {
    TEST("computeContourLevels min==max → single level");
    auto levels = xyplot::computeContourLevels(5.0, 5.0, 3);
    CHECK(levels.size() == 1);
    CHECK_CLOSE(levels[0], 5.0, 1e-10);
    PASS();
}

// ──── Test 11: computeContourLevels — reversed range ────
void test_levels_reversed() {
    TEST("computeContourLevels 100→0 (reversed) → ascending levels");
    auto levels = xyplot::computeContourLevels(100, 0, 3);
    CHECK(levels.size() == 3);
    CHECK(levels[0] <= levels[1]);
    CHECK(levels[1] <= levels[2]);
    CHECK_CLOSE(levels[0], 0.0,   1e-10);
    CHECK_CLOSE(levels[2], 100.0, 1e-10);
    PASS();
}

// ──── Test 12: NaN in grid → skip that cell ────
void test_nan_grid_cell() {
    TEST("NaN in one cell corner → cell skipped, others processed");
    // 4×3 grid: NaN only in one corner, many valid cells remain
    double nan = std::nan("");
    double grid[12] = {
         0,  0,  0,  0,
         0, 10, 10, nan,   // one NaN corner in the far-right cell
         0, 10, 10,  0
    };
    auto segs = xyplot::computeContourSegments(grid, 3, 4,
                                               0,0, 1,1, 5.0);
    // NaN cell skipped; surrounding valid cells still produce contours
    CHECK(!segs.empty());
    PASS();
}

// ──── Test 13: computeContours linked paths consistency ────
void test_linked_paths_consistency() {
    TEST("computeContours: linked paths have matching endpoints");
    // 4×4 grid with a radial pattern → closed contours
    // Center high, edges low
    double grid[16] = {
        0, 0, 0, 0,
        0, 5, 5, 0,
        0, 5, 5, 0,
        0, 0, 0, 0
    };
    auto paths = xyplot::computeContours(grid, 4, 4,
                                         0,0, 1,1, 2.5);
    CHECK(!paths.empty());

    // For closed paths, verify start ≈ end
    for (auto& p : paths) {
        if (p.isClosed) {
            double d = std::sqrt(
                (p.xs.front() - p.xs.back()) * (p.xs.front() - p.xs.back()) +
                (p.ys.front() - p.ys.back()) * (p.ys.front() - p.ys.back()));
            CHECK(d < 1e-8);
        }
        // Each path should have at least 2 points
        CHECK(p.xs.size() >= 2);
    }
    PASS();
}

// ──── Test 14: Multiple isovalues on same grid ────
void test_multiple_isovalues() {
    TEST("two different isovalues → different contour sets");
    // 4×4 grid: square plateau in center (values 8), border 0
    double grid[16] = {
        0, 0, 0, 0,
        0, 8, 8, 0,
        0, 8, 8, 0,
        0, 0, 0, 0
    };
    auto segs2 = xyplot::computeContourSegments(grid, 4, 4,
                                                 0,0, 1,1, 4.0);
    auto segs9 = xyplot::computeContourSegments(grid, 4, 4,
                                                 0,0, 1,1, 9.0);
    // iso=4: closed contour around the plateau → several segments
    // iso=9: all values < 9 → 0 segments
    CHECK(!segs2.empty());
    CHECK(segs9.empty());
    PASS();
}

// ──── Main ────
int main() {
    printf("=== Contour Algorithm Unit Tests (Phase B2) ===\n\n");

    printf("Segment extraction:\n");
    test_constant_grid();
    test_one_corner_above();
    test_center_peak_closed();
    test_saddle_point();
    test_too_few_rows();
    test_vertical_split();
    test_three_corners_above();
    test_nan_grid_cell();

    printf("\nPath linking:\n");
    test_linked_paths_consistency();

    printf("\nContour levels:\n");
    test_levels_normal();
    test_levels_single();
    test_levels_degenerate();
    test_levels_reversed();

    printf("\nMulti-isovalue:\n");
    test_multiple_isovalues();

    printf("\n──────────────────────────\n");
    printf("  %d passed, %d failed\n", g_testsPassed, g_testsFailed);
    printf("──────────────────────────\n");

    return g_testsFailed > 0 ? 1 : 0;
}
