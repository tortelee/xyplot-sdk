// ============================================================
// test_transform.cpp — Coordinate transform unit tests
// ============================================================
// Owner: Agent C
// Tests: normalize, denormalize, transform, inverseTransform,
//        transformPoints, transformArray
//
// Coverage:
//   - Linear transform: data → [0,1] → device
//   - Log10/Ln transform
//   - Reversed axes
//   - Degenerate ranges
//   - Batch transforms
//   - Round-trip: transform → inverseTransform
//   - NaN handling for log(≤0)
// ============================================================
#include "../src/coordinate_transform.h"
#include <cassert>
#include <cstdio>
#include <cmath>
#include <algorithm>

// ──── Test helpers ────
static int g_testsPassed = 0;
static int g_testsFailed = 0;

#define TEST(name) \
    do { \
        printf("  TEST: %s ... ", name); \
    } while(0)

#define PASS() \
    do { \
        printf("PASSED\n"); \
        g_testsPassed++; \
    } while(0)

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

#define CHECK_NAN(v) \
    do { \
        if (!std::isnan(v)) { \
            printf("FAILED\n    Expected NaN, got %g\n", (double)(v)); \
            g_testsFailed++; \
            return; \
        } \
    } while(0)

// ──── normalize tests ────

void test_normalize_linear_mid() {
    TEST("normalize Linear: 5 in [0,10] → 0.5");
    double n = xyplot::normalize(5.0, 0.0, 10.0);
    CHECK_CLOSE(n, 0.5, 1e-10);
    PASS();
}

void test_normalize_linear_min() {
    TEST("normalize Linear: min → 0.0");
    double n = xyplot::normalize(0.0, 0.0, 10.0);
    CHECK_CLOSE(n, 0.0, 1e-10);
    PASS();
}

void test_normalize_linear_max() {
    TEST("normalize Linear: max → 1.0");
    double n = xyplot::normalize(10.0, 0.0, 10.0);
    CHECK_CLOSE(n, 1.0, 1e-10);
    PASS();
}

void test_normalize_linear_reversed() {
    TEST("normalize Linear reversed: 5 in [10,0] → 0.5");
    double n = xyplot::normalize(5.0, 10.0, 0.0);
    CHECK_CLOSE(n, 0.5, 1e-10);
    PASS();
}

void test_normalize_linear_degenerate() {
    TEST("normalize Linear degenerate range → 0.5");
    // When min == max, normalize returns 0.5 (centered)
    double n = xyplot::normalize(5.0, 5.0, 5.0);
    CHECK_CLOSE(n, 0.5, 1e-10);
    PASS();
}

void test_normalize_log10() {
    TEST("normalize Log10: 10 in [1,100] → 0.5");
    double n = xyplot::normalize(10.0, 1.0, 100.0, xyplot::ScaleType::Log10);
    // log10(10)=1, log10(1)=0, log10(100)=2, (1-0)/(2-0) = 0.5
    CHECK_CLOSE(n, 0.5, 1e-10);
    PASS();
}

void test_normalize_log10_negative() {
    TEST("normalize Log10: negative input → NaN");
    double n = xyplot::normalize(-1.0, 1.0, 100.0, xyplot::ScaleType::Log10);
    CHECK_NAN(n);
    PASS();
}

// ──── denormalize tests ────

void test_denormalize_mid() {
    TEST("denormalize 0.5 from [100,500] → 300");
    double d = xyplot::denormalize(0.5, 100.0, 500.0);
    CHECK_CLOSE(d, 300.0, 1e-10);
    PASS();
}

void test_denormalize_boundaries() {
    TEST("denormalize boundaries");
    double d0 = xyplot::denormalize(0.0, 100.0, 500.0);
    double d1 = xyplot::denormalize(1.0, 100.0, 500.0);
    CHECK_CLOSE(d0, 100.0, 1e-10);
    CHECK_CLOSE(d1, 500.0, 1e-10);
    PASS();
}

// ──── transform tests ────

void test_transform_linear() {
    TEST("transform Linear: data 5 in [0,10] → device 50 in [0,100]");
    double d = xyplot::transform(5.0, 0.0, 10.0, 0.0, 100.0);
    CHECK_CLOSE(d, 50.0, 1e-10);
    PASS();
}

void test_transform_reversed_axis() {
    TEST("transform reversed data axis: 2 in [10,0] → device 80 in [0,100]");
    // data 2 in range [10,0]: normalize → (2-10)/(0-10) = 0.8, denormalize → 80
    double d = xyplot::transform(2.0, 10.0, 0.0, 0.0, 100.0);
    CHECK_CLOSE(d, 80.0, 1e-10);
    PASS();
}

void test_transform_log10() {
    TEST("transform Log10: 100 in [1,10000] → normalized 2/4=0.5, device...");
    double d = xyplot::transform(100.0, 1.0, 10000.0, 0.0, 1000.0,
                                 xyplot::ScaleType::Log10);
    // log10(100)=2, log10(1)=0, log10(10000)=4, n=(2-0)/(4-0)=0.5
    // d = 0 + 0.5*1000 = 500
    CHECK_CLOSE(d, 500.0, 1e-10);
    PASS();
}

// ──── inverseTransform tests ────

void test_inverse_linear_roundtrip() {
    TEST("inverseTransform: round-trip data → device → data");
    double original = 5.0;
    double device = xyplot::transform(original, 0.0, 10.0, 0.0, 100.0);
    double recovered = xyplot::inverseTransform(device, 0.0, 10.0, 0.0, 100.0);
    CHECK_CLOSE(recovered, original, 1e-10);
    PASS();
}

void test_inverse_log10_roundtrip() {
    TEST("inverseTransform Log10: round-trip");
    double original = 50.0;
    double device = xyplot::transform(original, 1.0, 1000.0, 0.0, 500.0,
                                       xyplot::ScaleType::Log10);
    double recovered = xyplot::inverseTransform(device, 1.0, 1000.0, 0.0, 500.0,
                                                 xyplot::ScaleType::Log10);
    CHECK_CLOSE(recovered, original, 1e-8);
    PASS();
}

void test_inverse_degenerate() {
    TEST("inverseTransform degenerate device → returns dataMin");
    double recovered = xyplot::inverseTransform(50.0, 0.0, 10.0, 100.0, 100.0);
    CHECK_CLOSE(recovered, 0.0, 1e-10);
    PASS();
}

// ──── Batch transform tests ────

void test_transformPoints_basic() {
    TEST("transformPoints: 3 points linear");
    const double xs[] = {0.0, 5.0, 10.0};
    const double ys[] = {0.0, 5.0, 10.0};
    double outX[3], outY[3];

    xyplot::transformPoints(xs, ys, 3,
                            0.0, 10.0,   // data X range
                            0.0, 10.0,   // data Y range
                            0.0, 100.0,  // device X range
                            0.0, 100.0,  // device Y range
                            outX, outY);

    // X: 0→0, 5→50, 10→100
    CHECK_CLOSE(outX[0], 0.0,   1e-10);
    CHECK_CLOSE(outX[1], 50.0,  1e-10);
    CHECK_CLOSE(outX[2], 100.0, 1e-10);

    // Y (inverted): data 0→device 100, data 5→device 50, data 10→device 0
    CHECK_CLOSE(outY[0], 100.0, 1e-10);
    CHECK_CLOSE(outY[1], 50.0,  1e-10);
    CHECK_CLOSE(outY[2], 0.0,   1e-10);
    PASS();
}

void test_transformPoints_null_safety() {
    TEST("transformPoints null pointers → no crash");
    xyplot::transformPoints(nullptr, nullptr, 0,
                            0.0, 1.0, 0.0, 1.0,
                            0.0, 1.0, 0.0, 1.0,
                            nullptr, nullptr);
    PASS();
}

void test_transformPoints_log_x() {
    TEST("transformPoints Log10 X, Linear Y");
    const double xs[] = {1.0, 10.0, 100.0};
    const double ys[] = {0.0, 5.0, 10.0};
    double outX[3], outY[3];

    xyplot::transformPoints(xs, ys, 3,
                            1.0, 100.0,   // data X range (log)
                            0.0, 10.0,     // data Y range
                            0.0, 100.0,    // device X
                            0.0, 100.0,    // device Y
                            outX, outY,
                            xyplot::ScaleType::Log10,
                            xyplot::ScaleType::Linear);

    // X: log10 values: 0, 1, 2 → normalized: 0, 0.5, 1 → device: 0, 50, 100
    CHECK_CLOSE(outX[0], 0.0,   1e-10);
    CHECK_CLOSE(outX[1], 50.0,  1e-10);
    CHECK_CLOSE(outX[2], 100.0, 1e-10);
    PASS();
}

void test_transformPoints_degenerate_x() {
    TEST("transformPoints degenerate X range → centers");
    const double xs[] = {5.0, 5.0, 5.0};
    const double ys[] = {0.0, 5.0, 10.0};
    double outX[3], outY[3];

    xyplot::transformPoints(xs, ys, 3,
                            5.0, 5.0,    // degenerate X
                            0.0, 10.0,   // normal Y
                            0.0, 100.0, 0.0, 100.0,
                            outX, outY);

    // X should be centered at device midpoint
    CHECK_CLOSE(outX[0], 50.0, 1e-10);
    CHECK_CLOSE(outX[1], 50.0, 1e-10);
    CHECK_CLOSE(outX[2], 50.0, 1e-10);
    PASS();
}

// ──── transformArray tests ────

void test_transformArray_basic() {
    TEST("transformArray: 4 values linear");
    const double data[] = {0.0, 3.0, 6.0, 10.0};
    double out[4];

    xyplot::transformArray(data, 4, 0.0, 10.0, 0.0, 100.0, out);

    CHECK_CLOSE(out[0], 0.0,  1e-10);
    CHECK_CLOSE(out[1], 30.0, 1e-10);
    CHECK_CLOSE(out[2], 60.0, 1e-10);
    CHECK_CLOSE(out[3], 100.0, 1e-10);
    PASS();
}

void test_transformArray_null_safety() {
    TEST("transformArray null input → no crash");
    xyplot::transformArray(nullptr, 0, 0.0, 1.0, 0.0, 1.0, nullptr);
    PASS();
}

// ──── Main ────
int main() {
    printf("=== Coordinate Transform Unit Tests ===\n\n");

    printf("Normalize:\n");
    test_normalize_linear_mid();
    test_normalize_linear_min();
    test_normalize_linear_max();
    test_normalize_linear_reversed();
    test_normalize_linear_degenerate();
    test_normalize_log10();
    test_normalize_log10_negative();

    printf("\nDenormalize:\n");
    test_denormalize_mid();
    test_denormalize_boundaries();

    printf("\nFull Transform:\n");
    test_transform_linear();
    test_transform_reversed_axis();
    test_transform_log10();

    printf("\nInverse Transform:\n");
    test_inverse_linear_roundtrip();
    test_inverse_log10_roundtrip();
    test_inverse_degenerate();

    printf("\nBatch Transform (transformPoints):\n");
    test_transformPoints_basic();
    test_transformPoints_null_safety();
    test_transformPoints_log_x();
    test_transformPoints_degenerate_x();

    printf("\nArray Transform (transformArray):\n");
    test_transformArray_basic();
    test_transformArray_null_safety();

    printf("\n──────────────────────────\n");
    printf("  %d passed, %d failed\n", g_testsPassed, g_testsFailed);
    printf("──────────────────────────\n");

    return g_testsFailed > 0 ? 1 : 0;
}
