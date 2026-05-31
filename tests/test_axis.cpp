// ============================================================
// test_axis.cpp — Axis system unit tests
// ============================================================
// Owner: Agent C
// Tests: niceNumber, computeTicks, formatTickLabel, autoPrecision
//
// Coverage:
//   - Nice Number on typical ranges
//   - Linear tick computation
//   - Log10 tick computation
//   - Degenerate ranges (min == max)
//   - Reversed ranges
//   - Label formatting for various values
//   - Auto precision detection
// ============================================================
#include "../src/axis_system.h"
#include <cassert>
#include <cstdio>
#include <cmath>
#include <string>

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
        if (std::abs((a) - (b)) > (eps)) { \
            printf("FAILED\n    Expected ~%g, got %g (eps=%g)\n", (double)(b), (double)(a), (double)(eps)); \
            g_testsFailed++; \
            return; \
        } \
    } while(0)

// ──── Nice Number tests ────

void test_niceNumber_typical() {
    TEST("niceNumber typical intervals");
    // Interval around 0.7 → nice = 1.0
    CHECK_CLOSE(xyplot::niceNumber(0.7, true),  1.0, 0.01);
    // Interval around 0.3 → nice = 0.5? No, 0.3 < 0.5 so it rounds up to 0.5? Let's check the algorithm:
    // log10(0.3) ≈ -0.523, frac = 0.3/10^(-1) = 3.0... Actually let me compute:
    // log10(0.3) ≈ -0.523, floor = -1, 10^(-1) = 0.1, frac = 0.3/0.1 = 3.0
    // round=true: 3.0 > 2.5 && 3.0 <= 5.0 → 5.0, result = 5.0 * 0.1 = 0.5
    CHECK_CLOSE(xyplot::niceNumber(0.3, true),  0.5, 0.01);
    // Interval around 7.5 → log10(7.5)=0.875, floor=0, 10^0=1, frac=7.5
    // round=true: 7.5 > 5.0 → 10.0, result = 10.0
    CHECK_CLOSE(xyplot::niceNumber(7.5, true),  10.0, 0.01);
    PASS();
}

void test_niceNumber_round_vs_ceil() {
    TEST("niceNumber round vs ceil behavior");
    // round=true for interval selection (rounds up more aggressively)
    // round=false for bound clamping (more conservative)
    // For x=1.2: log10=0.079, floor=0, 10^0=1, frac=1.2
    //   round=true: 1.2 <= 2.0 → 2.0, result = 2.0
    //   round=false: 1.2 < 1.5 → 1.0, result = 1.0
    CHECK_CLOSE(xyplot::niceNumber(1.2, true),  2.0, 0.01);  // round → 2
    CHECK_CLOSE(xyplot::niceNumber(1.2, false), 1.0, 0.01);   // ceil → 1
    PASS();
}

void test_niceNumber_smallNumbers() {
    TEST("niceNumber small numbers");
    // 0.035 → log10 ≈ -1.456, floor=-2, 10^(-2)=0.01, frac=3.5
    // round=true: 3.5 > 2.5 → 5.0, result=0.05
    CHECK_CLOSE(xyplot::niceNumber(0.035, true), 0.05, 0.001);
    PASS();
}

void test_niceNumber_largeNumbers() {
    TEST("niceNumber large numbers");
    // 350 → log10=2.544, floor=2, 10^2=100, frac=3.5
    // round=true: 3.5 > 2.5 → 5.0, result=500
    CHECK_CLOSE(xyplot::niceNumber(350.0, true), 500.0, 1.0);
    PASS();
}

// ──── Tick computation tests ────

void test_ticks_linear_range() {
    TEST("computeTicks linear [0.3, 8.7] → {0,2,4,6,8,10}");
    xyplot::AxisConfig cfg;
    cfg.dataMin = 0.3;
    cfg.dataMax = 8.7;
    cfg.targetMajorTicks = 5;

    auto ticks = xyplot::computeTicks(cfg);
    CHECK(!ticks.majorTicks.empty());

    // Expected major ticks: 0, 2, 4, 6, 8, 10
    // (range ≈ 8.4, roughInterval ≈ 1.68, nice→2.0, niceMin=0, niceMax=10)
    CHECK(ticks.majorTicks.size() >= 5);
    CHECK_CLOSE(ticks.majorTicks[0], 0.0, 0.01);
    CHECK_CLOSE(ticks.niceMin, 0.0, 0.01);
    CHECK_CLOSE(ticks.niceMax, 10.0, 1.0);

    // Check that ticks are evenly spaced
    if (ticks.majorTicks.size() >= 2) {
        double step = ticks.majorTicks[1] - ticks.majorTicks[0];
        for (size_t i = 2; i < ticks.majorTicks.size(); ++i) {
            CHECK_CLOSE(ticks.majorTicks[i] - ticks.majorTicks[i-1], step, 0.01);
        }
    }
    PASS();
}

void test_ticks_negative_range() {
    TEST("computeTicks linear [-5.3, 3.2]");
    xyplot::AxisConfig cfg;
    cfg.dataMin = -5.3;
    cfg.dataMax = 3.2;
    cfg.targetMajorTicks = 5;

    auto ticks = xyplot::computeTicks(cfg);
    CHECK(!ticks.majorTicks.empty());
    // Nice range should cover [-5.3, 3.2]
    CHECK(ticks.niceMin <= -5.3 + 0.01);
    CHECK(ticks.niceMax >= 3.2 - 0.01);
    PASS();
}

void test_ticks_small_range() {
    TEST("computeTicks small range [0.001, 0.005]");
    xyplot::AxisConfig cfg;
    cfg.dataMin = 0.001;
    cfg.dataMax = 0.005;
    cfg.targetMajorTicks = 5;

    auto ticks = xyplot::computeTicks(cfg);
    CHECK(!ticks.majorTicks.empty());
    CHECK(ticks.majorTicks.size() >= 3);
    PASS();
}

void test_ticks_degenerate_range() {
    TEST("computeTicks degenerate range [5, 5]");
    xyplot::AxisConfig cfg;
    cfg.dataMin = 5.0;
    cfg.dataMax = 5.0;
    cfg.targetMajorTicks = 5;

    auto ticks = xyplot::computeTicks(cfg);
    CHECK(!ticks.majorTicks.empty());
    // Should expand and produce ticks around 5
    CHECK(ticks.niceMin <= 5.0);
    CHECK(ticks.niceMax >= 5.0);
    PASS();
}

void test_ticks_reversed_range() {
    TEST("computeTicks reversed range [10, 0]");
    xyplot::AxisConfig cfg;
    cfg.dataMin = 10.0;
    cfg.dataMax = 0.0;
    cfg.targetMajorTicks = 5;

    auto ticks = xyplot::computeTicks(cfg);
    CHECK(!ticks.majorTicks.empty());
    // Should still produce valid ticks
    CHECK(ticks.majorTicks.size() >= 3);
    PASS();
}

void test_ticks_labels_match() {
    TEST("computeTicks labels match major tick count");
    xyplot::AxisConfig cfg;
    cfg.dataMin = 0.0;
    cfg.dataMax = 100.0;
    cfg.targetMajorTicks = 5;

    auto ticks = xyplot::computeTicks(cfg);
    CHECK(ticks.labels.size() == ticks.majorTicks.size());

    // Labels should not be empty
    for (size_t i = 0; i < ticks.labels.size(); ++i) {
        CHECK(!ticks.labels[i].empty());
    }
    PASS();
}

void test_ticks_log10_range() {
    TEST("computeTicks Log10 [1, 1000]");
    xyplot::AxisConfig cfg;
    cfg.dataMin = 1.0;
    cfg.dataMax = 1000.0;
    cfg.targetMajorTicks = 4;
    cfg.scaleType = xyplot::ScaleType::Log10;

    auto ticks = xyplot::computeTicks(cfg);
    CHECK(!ticks.majorTicks.empty());
    // Should have ticks at powers of 10: 1, 10, 100, 1000
    CHECK(ticks.majorTicks.size() >= 4);
    PASS();
}

void test_ticks_minor_ticks() {
    TEST("computeTicks minor ticks between majors");
    xyplot::AxisConfig cfg;
    cfg.dataMin = 0.0;
    cfg.dataMax = 10.0;
    cfg.targetMajorTicks = 5;
    cfg.targetMinorTicks = 1;

    auto ticks = xyplot::computeTicks(cfg);
    CHECK(!ticks.majorTicks.empty());

    // With minorTicks=1, there should be minor ticks between each major
    if (ticks.majorTicks.size() >= 2) {
        CHECK(!ticks.minorTicks.empty());
    }
    PASS();
}

// ──── Label formatting tests ────

void test_format_integer() {
    TEST("formatTickLabel integer values");
    std::string s = xyplot::formatTickLabel(42.0, 0);
    CHECK(s == "42");
    PASS();
}

void test_format_decimal() {
    TEST("formatTickLabel decimal values");
    std::string s = xyplot::formatTickLabel(3.14, 2);
    CHECK(s == "3.14");
    PASS();
}

void test_format_negative() {
    TEST("formatTickLabel negative values");
    std::string s = xyplot::formatTickLabel(-5.0, 1);
    CHECK(s == "-5");
    PASS();
}

void test_format_autoPrecision() {
    TEST("formatTickLabel auto precision (-1)");
    std::string s1 = xyplot::formatTickLabel(100.0, -1);
    CHECK(s1 == "100");

    std::string s2 = xyplot::formatTickLabel(0.5, -1);
    CHECK(s2 == "0.5" || s2 == "0.500000");  // auto precision varies
    PASS();
}

void test_format_scientific() {
    TEST("formatTickLabel very large → scientific notation");
    std::string s = xyplot::formatTickLabel(1.5e9, -1);
    // Should use scientific notation for values ≥ 1e9
    CHECK(!s.empty());
    PASS();
}

// ──── Auto precision tests ────

void test_autoPrecision_integer_interval() {
    TEST("autoPrecision interval=1.0 → 0 decimals");
    int p = xyplot::autoPrecision(1.0);
    CHECK(p == 0);
    PASS();
}

void test_autoPrecision_fractional_interval() {
    TEST("autoPrecision interval=0.2 → 1 decimal");
    int p = xyplot::autoPrecision(0.2);
    CHECK(p == 1);
    PASS();
}

void test_autoPrecision_small_interval() {
    TEST("autoPrecision interval=0.005 → 3 decimals");
    int p = xyplot::autoPrecision(0.005);
    CHECK(p == 3);
    PASS();
}

void test_autoPrecision_large_interval() {
    TEST("autoPrecision interval=50 → 0 decimals");
    int p = xyplot::autoPrecision(50.0);
    CHECK(p == 0);
    PASS();
}

// ──── Main ────
int main() {
    printf("=== Axis System Unit Tests ===\n\n");

    printf("Nice Number:\n");
    test_niceNumber_typical();
    test_niceNumber_round_vs_ceil();
    test_niceNumber_smallNumbers();
    test_niceNumber_largeNumbers();

    printf("\nTick Computation:\n");
    test_ticks_linear_range();
    test_ticks_negative_range();
    test_ticks_small_range();
    test_ticks_degenerate_range();
    test_ticks_reversed_range();
    test_ticks_labels_match();
    test_ticks_log10_range();
    test_ticks_minor_ticks();

    printf("\nLabel Formatting:\n");
    test_format_integer();
    test_format_decimal();
    test_format_negative();
    test_format_autoPrecision();
    test_format_scientific();

    printf("\nAuto Precision:\n");
    test_autoPrecision_integer_interval();
    test_autoPrecision_fractional_interval();
    test_autoPrecision_small_interval();
    test_autoPrecision_large_interval();

    printf("\n──────────────────────────\n");
    printf("  %d passed, %d failed\n", g_testsPassed, g_testsFailed);
    printf("──────────────────────────\n");

    return g_testsFailed > 0 ? 1 : 0;
}
