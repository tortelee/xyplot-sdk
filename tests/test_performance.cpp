// ============================================================
// test_performance.cpp — Performance baseline tests
// Owner: Agent E — Backend & Integration
// Phase C: 质量深化 (Day 2 上午)
// ============================================================
// Tests:
//   1. 1M 点坐标变换耗时 (期望 < 50ms Debug, < 10ms Release)
//   2. 1M 点折线渲染路径 (transform + drawPolyline 调用序列)
//   3. 10M 点 DataTable 内存占用
//   4. Axis computeTicks 耗时 (极端范围 1e-15 ~ 1e15)
// ============================================================
#include "xyplot/xyplot.h"
#include "../src/axis_system.h"
#include "../src/coordinate_transform.h"
#include "../src/datatable.h"
#include "../backends/recording/recording_device.h"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <vector>
#include <string>

using namespace xyplot;

// ==================================================================
// Timing helper
// ==================================================================
class Timer {
public:
    void start() { m_start = std::chrono::high_resolution_clock::now(); }
    double elapsedMs() const {
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double, std::milli>(end - m_start).count();
    }
private:
    std::chrono::high_resolution_clock::time_point m_start;
};

// ==================================================================
// Test results
// ==================================================================
namespace {

int testsPassed = 0;
int testsFailed = 0;

void check(bool condition, const char* name, const char* detail = "") {
    if (condition) {
        testsPassed++;
    } else {
        testsFailed++;
        std::printf("  FAIL: %s", name);
        if (detail && detail[0]) std::printf(" — %s", detail);
        std::printf("\n");
    }
}

#define TEST(name) std::printf("[PERF] %s\n", name)
#define CHECK(cond) check((cond), #cond)
#define CHECK_MSG(cond, msg) check((cond), #cond, msg)

} // anonymous namespace

// ==================================================================
// Test 1: 1M 点坐标变换耗时
// ==================================================================
void test_transform_1m_points() {
    TEST("1M points coordinate transform");

    const int N = 1'000'000;
    std::vector<double> xs(N), ys(N);
    for (int i = 0; i < N; i++) {
        xs[i] = static_cast<double>(i) * 0.001;
        ys[i] = std::sin(static_cast<double>(i) * 0.0001);
    }

    std::vector<double> outX(N), outY(N);

    Timer t;
    t.start();
    transformPoints(xs.data(), ys.data(), N,
                    0.0, 1000.0,    // data range x: 0→1000
                    -1.0, 1.0,      // data range y: -1→1
                    0.0, 800.0,     // device x: 0→800
                    600.0, 0.0,     // device y: 600→0 (inverted)
                    outX.data(), outY.data(),
                    ScaleType::Linear, ScaleType::Linear);
    double elapsed = t.elapsedMs();

    std::printf("  Time: %.2f ms for %d points (%.2f ns/point)\n",
                elapsed, N, elapsed * 1e6 / N);

    // Verify a few outputs
    CHECK_MSG(outX[0] >= 0.0 && outX[0] < 1.0, "first X mapped");
    CHECK_MSG(outX[N-1] > 790.0, "last X mapped near 800");
    CHECK_MSG(outY[0] >= 299.0 && outY[0] <= 301.0, "first Y mapped (sin(0)=0 → 300)");

    // Benchmark expectation: < 50ms in Debug
#ifdef NDEBUG
    double threshold = 10.0;  // Release: < 10ms
#else
    double threshold = 50.0;  // Debug: < 50ms
#endif
    CHECK_MSG(elapsed < threshold, "within time threshold");

    std::printf("  Baseline recorded: transformPoints(1M) = %.2f ms\n\n", elapsed);
}

// ==================================================================
// Test 2: 1M 点折线渲染路径
// ==================================================================
void test_render_pipeline_1m_points() {
    TEST("1M points render pipeline (Recording backend)");

    const int N = 1'000'000;
    std::vector<double> xs(N), ys(N);
    for (int i = 0; i < N; i++) {
        xs[i] = static_cast<double>(i);
        ys[i] = std::sqrt(static_cast<double>(i + 1));
    }

    Plot plot;
    plot.addLineSeries("1M Curve", xs.data(), ys.data(), N);
    plot.setAxisRange(0, static_cast<double>(N), 0, std::sqrt(static_cast<double>(N)));

    RecordingDevice rec;

    Timer t;
    t.start();
    plot.render(rec);
    double elapsed = t.elapsedMs();

    int polyCount = rec.callCount(RecordingDevice::DrawPolyline);
    int textCount = rec.callCount(RecordingDevice::DrawText);
    int totalCalls = rec.totalCalls();

    std::printf("  Time: %.2f ms for %d points\n", elapsed, N);
    std::printf("  DrawPolyline calls: %d, DrawText calls: %d, Total: %d\n",
                polyCount, textCount, totalCalls);

    CHECK(rec.hasValidFrameSequence());
    CHECK(polyCount > 0);
    CHECK(totalCalls > 0);

    // Render pipeline should be reasonably fast
    // Expect < 200ms in Debug (mostly transform + recording overhead)
#ifdef NDEBUG
    double threshold = 50.0;   // Release: < 50ms
#else
    double threshold = 200.0;  // Debug: < 200ms
#endif
    CHECK_MSG(elapsed < threshold, "within render threshold");

    std::printf("  Baseline recorded: render(1M line points) = %.2f ms\n\n", elapsed);
}

// ==================================================================
// Test 3: 10M 点 DataTable 内存占用
// ==================================================================
void test_datatable_memory_10m() {
    TEST("10M points DataTable memory estimation");

    const int ROWS = 10'000'000;
    const int COLS = 2;  // x, y

    // Estimate: sizeof(double) * ROWS * COLS + vector overhead
    size_t rawDataSize = sizeof(double) * static_cast<size_t>(ROWS) * COLS;
    size_t vectorOverhead = sizeof(std::vector<double>) * COLS
                          + sizeof(void*) * COLS;  // ~minimal

    std::printf("  Estimated raw data: %.2f MB (%zu bytes per double × %d rows × %d cols)\n",
                rawDataSize / 1e6, sizeof(double), ROWS, COLS);
    std::printf("  Estimated with vector overhead: ~%.2f MB\n",
                (rawDataSize + vectorOverhead) / 1e6);

    // Create actual DataTable with 100K rows (avoid OOM on 10M in test)
    const int SAMPLE_ROWS = 100'000;
    std::vector<double> data(SAMPLE_ROWS * COLS);
    for (int i = 0; i < SAMPLE_ROWS * COLS; i++)
        data[i] = static_cast<double>(i) * 0.001;

    const char* colNames[] = { "X", "Y" };

    Timer t;
    t.start();
    DataTable table = DataTable::fromMemory(data.data(), SAMPLE_ROWS, COLS, colNames);
    double createTime = t.elapsedMs();

    std::printf("  Sample: %d rows × %d cols created in %.2f ms\n",
                SAMPLE_ROWS, COLS, createTime);
    CHECK(table.rowCount() == SAMPLE_ROWS);
    CHECK(table.colCount() == COLS);
    CHECK(table.column("X") != nullptr);
    CHECK(table.column("Y") != nullptr);

    // Estimate for full 10M rows
    double perRowMs = createTime / SAMPLE_ROWS;
    double estimated10M = perRowMs * ROWS;
    std::printf("  Estimated 10M rows create time: %.0f ms (%.2f ns/row)\n",
                estimated10M, perRowMs * 1e6);
    std::printf("  Estimated 10M rows memory: ~%.2f MB\n",
                (rawDataSize + vectorOverhead) / 1e6);

    CHECK_MSG(createTime < 200.0, "100K DataTable creation < 200ms");
    std::printf("  Baseline recorded: DataTable(100K×2) = %.2f ms | 10M est. ~%.0f MB\n\n",
                createTime, (rawDataSize + vectorOverhead) / 1e6);
}

// ==================================================================
// Test 4: Axis computeTicks 极限范围
// ==================================================================
void test_axis_compute_ticks_extreme() {
    TEST("Axis computeTicks — extreme ranges");

    struct TestCase {
        const char* name;
        double lo, hi;
        ScaleType scale;
    };

    TestCase cases[] = {
        { "1e-15 → 1e15 (Linear)",   1e-15, 1e15, ScaleType::Linear },
        { "1e-15 → 1e15 (Log10)",    1e-15, 1e15, ScaleType::Log10  },
        { "0 → 1 (degenerate low)",  0.0,   1.0,  ScaleType::Linear },
        { "1000 → 1000 (equal)",     1000.0, 1000.0, ScaleType::Linear },
        { "1e-6 → 1e-3 (small)",     1e-6,  1e-3, ScaleType::Linear },
        { "1e9 → 1e12 (large)",      1e9,   1e12, ScaleType::Linear },
        { "-1e6 → 1e6 (negative)",   -1e6,  1e6,  ScaleType::Linear },
    };

    Timer t;
    double totalTime = 0;
    int totalTicks = 0;

    for (auto& tc : cases) {
        AxisConfig cfg;
        cfg.dataMin = tc.lo;
        cfg.dataMax = tc.hi;
        cfg.scaleType = tc.scale;
        cfg.targetMajorTicks = 5;

        t.start();
        AxisTicks ticks = computeTicks(cfg);
        double elapsed = t.elapsedMs();
        totalTime += elapsed;
        totalTicks += static_cast<int>(ticks.majorTicks.size());

        std::printf("  %-35s → %2zu major ticks, %.3f ms\n",
                    tc.name, ticks.majorTicks.size(), elapsed);

        CHECK_MSG(!ticks.majorTicks.empty(), "has major ticks");
        CHECK_MSG(ticks.labels.size() == ticks.majorTicks.size(), "labels match ticks");
        CHECK_MSG(elapsed < 1.0, "computeTicks < 1ms per call");
    }

    std::printf("  Total: %.3f ms for %d cases, %d ticks\n\n",
                totalTime, (int)(sizeof(cases)/sizeof(cases[0])), totalTicks);
    CHECK_MSG(totalTime < 10.0, "all axis computations < 10ms total");
}

// ==================================================================
// Summary
// ==================================================================
void print_summary() {
    std::printf("═══════════════════════════════════════════\n");
    std::printf(" Performance Baseline Summary\n");
    std::printf("═══════════════════════════════════════════\n");
    std::printf(" Platform: %s | %s\n",
#ifdef NDEBUG
                "Release",
#else
                "Debug",
#endif
#ifdef _WIN32
                "Windows"
#else
                "Linux"
#endif
    );
    std::printf(" Compiler: C++17\n");
    std::printf("═══════════════════════════════════════════\n");
}

// ==================================================================
// Main
// ==================================================================
int main() {
    print_summary();
    std::printf("\n");

    test_transform_1m_points();
    test_render_pipeline_1m_points();
    test_datatable_memory_10m();
    test_axis_compute_ticks_extreme();

    std::printf("═══════════════════════════════════════════\n");
    std::printf(" Results: %d passed, %d failed\n", testsPassed, testsFailed);
    std::printf("═══════════════════════════════════════════\n");

    return testsFailed > 0 ? 1 : 0;
}
