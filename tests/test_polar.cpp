// ============================================================
// test_polar.cpp — Polar coordinate transform unit tests
// ============================================================
// Owner: Agent C
// Phase: B1 — P1 chart type expansion
// Tests: polarToCartesian, polarToCartesianBatch
// Coverage: cardinal angles, negative radius, edge cases, batch
// ============================================================
#include "../src/polar_transform.h"
#include <cassert>
#include <cstdio>
#include <cmath>
#include <vector>

static int g_testsPassed = 0;
static int g_testsFailed = 0;

// π constant for the math-averse
static constexpr double PI = 3.14159265358979323846;

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

// ──── Test 1: θ = 0° → (r, 0) ────
void test_angle_0() {
    TEST("theta=0 deg → (r, 0)");
    double x, y;
    xyplot::polarToCartesian(5.0, 0.0, x, y);
    CHECK_CLOSE(x, 5.0, 1e-12);
    CHECK_CLOSE(y, 0.0, 1e-12);
    PASS();
}

// ──── Test 2: θ = 90° (π/2) → (0, r) ────
void test_angle_90() {
    TEST("theta=90 deg → (0, r)");
    double x, y;
    xyplot::polarToCartesian(3.0, PI / 2.0, x, y);
    CHECK_CLOSE(x, 0.0, 1e-12);
    CHECK_CLOSE(y, 3.0, 1e-12);
    PASS();
}

// ──── Test 3: θ = 180° (π) → (-r, 0) ────
void test_angle_180() {
    TEST("theta=180 deg → (-r, 0)");
    double x, y;
    xyplot::polarToCartesian(4.0, PI, x, y);
    CHECK_CLOSE(x, -4.0, 1e-12);
    CHECK_CLOSE(y, 0.0, 1e-12);
    PASS();
}

// ──── Test 4: θ = 270° (3π/2) → (0, -r) ────
void test_angle_270() {
    TEST("theta=270 deg → (0, -r)");
    double x, y;
    xyplot::polarToCartesian(2.0, 3.0 * PI / 2.0, x, y);
    CHECK_CLOSE(x, 0.0, 1e-12);
    CHECK_CLOSE(y, -2.0, 1e-12);
    PASS();
}

// ──── Test 5: θ = 45° → (r/√2, r/√2) ────
void test_angle_45() {
    TEST("theta=45 deg → (r/√2, r/√2)");
    double x, y;
    double r = std::sqrt(2.0);  // r = √2 → output should be (1, 1)
    xyplot::polarToCartesian(r, PI / 4.0, x, y);
    CHECK_CLOSE(x, 1.0, 1e-12);
    CHECK_CLOSE(y, 1.0, 1e-12);
    PASS();
}

// ──── Test 6: θ = -90° → (0, -r) ────
void test_negative_angle() {
    TEST("theta=-90 deg → (0, -r)");
    double x, y;
    xyplot::polarToCartesian(5.0, -PI / 2.0, x, y);
    CHECK_CLOSE(x, 0.0, 1e-12);
    CHECK_CLOSE(y, -5.0, 1e-12);
    PASS();
}

// ──── Test 7: θ > 2π (periodic) ────
void test_large_angle() {
    TEST("theta=450 deg (2π+90) → same as 90 deg");
    double x1, y1, x2, y2;
    xyplot::polarToCartesian(3.0, PI / 2.0, x1, y1);         // 90°
    xyplot::polarToCartesian(3.0, PI / 2.0 + 2.0 * PI, x2, y2); // 450°
    CHECK_CLOSE(x1, x2, 1e-12);
    CHECK_CLOSE(y1, y2, 1e-12);
    PASS();
}

// ──── Test 8: Zero radius → origin ────
void test_zero_radius() {
    TEST("r=0 → (0, 0) regardless of theta");
    double x, y;
    xyplot::polarToCartesian(0.0, 2.5, x, y);
    CHECK_CLOSE(x, 0.0, 1e-12);
    CHECK_CLOSE(y, 0.0, 1e-12);
    // Also at odd angle
    xyplot::polarToCartesian(0.0, PI / 3.0, x, y);
    CHECK_CLOSE(x, 0.0, 1e-12);
    CHECK_CLOSE(y, 0.0, 1e-12);
    PASS();
}

// ──── Test 9: Negative radius → |r| with θ + π ────
void test_negative_radius() {
    TEST("r=-5, theta=0 → same as r=5, theta=π");
    double xn, yn, xp, yp;
    xyplot::polarToCartesian(-5.0, 0.0, xn, yn);     // r=-5, θ=0
    xyplot::polarToCartesian(5.0, PI, xp, yp);        // r=5, θ=π
    CHECK_CLOSE(xn, xp, 1e-12);
    CHECK_CLOSE(yn, yp, 1e-12);
    PASS();
}

// ──── Test 10: Negative radius at 90° ────
void test_negative_radius_90() {
    TEST("r=-3, theta=π/2 → same as r=3, theta=270°");
    double xn, yn, xp, yp;
    xyplot::polarToCartesian(-3.0, PI / 2.0, xn, yn);
    xyplot::polarToCartesian(3.0, 3.0 * PI / 2.0, xp, yp);
    CHECK_CLOSE(xn, xp, 1e-12);
    CHECK_CLOSE(yn, yp, 1e-12);
    PASS();
}

// ──── Test 11: Large radius ────
void test_large_radius() {
    TEST("large r=1e8 → correct scale");
    double x, y;
    xyplot::polarToCartesian(1e8, PI / 4.0, x, y);
    double expected = 1e8 / std::sqrt(2.0);
    CHECK_CLOSE(x, expected, 1.0);   // 1 unit tolerance for large values
    CHECK_CLOSE(y, expected, 1.0);
    PASS();
}

// ──── Test 12: Very small radius ────
void test_small_radius() {
    TEST("small r=1e-10 → correct scale near origin");
    double x, y;
    xyplot::polarToCartesian(1e-10, PI / 3.0, x, y);
    CHECK_CLOSE(x, 1e-10 * 0.5, 1e-22);
    CHECK_CLOSE(y, 1e-10 * std::sqrt(3.0) / 2.0, 1e-22);
    PASS();
}

// ──── Test 13: NaN input → NaN output ────
void test_nan_input() {
    TEST("NaN input → NaN output");
    double x, y;
    xyplot::polarToCartesian(std::nan(""), 1.0, x, y);
    CHECK(std::isnan(x));
    CHECK(std::isnan(y));

    xyplot::polarToCartesian(1.0, std::nan(""), x, y);
    CHECK(std::isnan(x));
    CHECK(std::isnan(y));
    PASS();
}

// ──── Test 14: Inf input → NaN output ────
void test_inf_input() {
    TEST("Inf input → NaN output");
    double x, y;
    double inf = std::numeric_limits<double>::infinity();
    xyplot::polarToCartesian(inf, 1.0, x, y);
    CHECK(std::isnan(x));
    CHECK(std::isnan(y));

    xyplot::polarToCartesian(1.0, inf, x, y);
    CHECK(std::isnan(x));
    CHECK(std::isnan(y));
    PASS();
}

// ──── Test 15: Batch transform — 4 points ────
void test_batch_basic() {
    TEST("polarToCartesianBatch: 4 cardinal points");
    const double r[]     = {1.0, 1.0, 1.0, 1.0};
    const double theta[] = {0.0, PI / 2.0, PI, 3.0 * PI / 2.0};
    double outX[4], outY[4];

    xyplot::polarToCartesianBatch(r, theta, 4, outX, outY);

    CHECK_CLOSE(outX[0], 1.0,  1e-12);
    CHECK_CLOSE(outY[0], 0.0,  1e-12);
    CHECK_CLOSE(outX[1], 0.0,  1e-12);
    CHECK_CLOSE(outY[1], 1.0,  1e-12);
    CHECK_CLOSE(outX[2], -1.0, 1e-12);
    CHECK_CLOSE(outY[2], 0.0,  1e-12);
    CHECK_CLOSE(outX[3], 0.0,  1e-12);
    CHECK_CLOSE(outY[3], -1.0, 1e-12);
    PASS();
}

// ──── Test 16: Batch transform — null safety ────
void test_batch_null() {
    TEST("polarToCartesianBatch: null inputs → no crash");
    xyplot::polarToCartesianBatch(nullptr, nullptr, 0, nullptr, nullptr);
    PASS();
}

// ──── Test 17: Batch transform — large count ────
void test_batch_large() {
    TEST("polarToCartesianBatch: 10k points consistency");
    const int N = 10000;
    std::vector<double> r(N, 2.0);
    std::vector<double> theta(N);
    std::vector<double> outX(N), outY(N);

    for (int i = 0; i < N; ++i) {
        theta[i] = 2.0 * PI * static_cast<double>(i) / static_cast<double>(N);
    }

    xyplot::polarToCartesianBatch(r.data(), theta.data(), N,
                                   outX.data(), outY.data());

    // All points at r=2, so x² + y² = 4 for each
    for (int i = 0; i < N; ++i) {
        double magSq = outX[i] * outX[i] + outY[i] * outY[i];
        CHECK_CLOSE(magSq, 4.0, 1e-10);
    }
    PASS();
}

// ──── Test 18: All quadrants ────
void test_all_quadrants() {
    TEST("4 points check sign: Q1, Q2, Q3, Q4");
    double x, y;

    // Q1: 30° → x>0, y>0
    xyplot::polarToCartesian(1.0, PI / 6.0, x, y);
    CHECK(x > 0.0); CHECK(y > 0.0);

    // Q2: 120° → x<0, y>0
    xyplot::polarToCartesian(1.0, 2.0 * PI / 3.0, x, y);
    CHECK(x < 0.0); CHECK(y > 0.0);

    // Q3: 210° → x<0, y<0
    xyplot::polarToCartesian(1.0, 7.0 * PI / 6.0, x, y);
    CHECK(x < 0.0); CHECK(y < 0.0);

    // Q4: 300° → x>0, y<0
    xyplot::polarToCartesian(1.0, 5.0 * PI / 3.0, x, y);
    CHECK(x > 0.0); CHECK(y < 0.0);
    PASS();
}

// ──── Main ────
int main() {
    printf("=== Polar Transform Unit Tests (Phase B1) ===\n\n");

    printf("Cardinal angles:\n");
    test_angle_0();
    test_angle_90();
    test_angle_180();
    test_angle_270();
    test_angle_45();

    printf("\nEdge angles:\n");
    test_negative_angle();
    test_large_angle();

    printf("\nSpecial radii:\n");
    test_zero_radius();
    test_negative_radius();
    test_negative_radius_90();
    test_large_radius();
    test_small_radius();

    printf("\nError handling:\n");
    test_nan_input();
    test_inf_input();

    printf("\nBatch transform:\n");
    test_batch_basic();
    test_batch_null();
    test_batch_large();

    printf("\nQuadrants:\n");
    test_all_quadrants();

    printf("\n──────────────────────────\n");
    printf("  %d passed, %d failed\n", g_testsPassed, g_testsFailed);
    printf("──────────────────────────\n");

    return g_testsFailed > 0 ? 1 : 0;
}
