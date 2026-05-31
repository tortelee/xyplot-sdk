// ============================================================
// examples/blend2d_demo/main.cpp — XYPlot Blend2D 渲染 Demo
// ============================================================
// Owner: Agent F (文档 & 示例)
//
// 此示例展示如何将 XYPlot 图表渲染为 PNG 文件，使用
// Blend2D 作为高性能 2D 渲染后端。
//
// 依赖: Blend2D (https://github.com/blend2d/blend2d)
//       若未安装，此文件将不会通过 CMake 构建。
//
// 编译 (使用 CMake):
//   在 CMakeLists.txt 中启用 examples/blend2d_demo 部分，
//   并确保 find_package(blend2d) 成功。
//
// 运行:
//   ./xyplot_blend2d_demo
//   输出: chart_output.png
//
// 备选方案: 如果 Blend2D 不可用，请参考 ../minimal/ 中的
//           控制台示例，无需任何外部依赖。
// ============================================================

#include <xyplot/xyplot.h>

// Blend2D 为可选依赖。若未安装，此示例不会编译。
// (CMakeLists.txt 在未检测到 Blend2D 时会跳过编译)
#if __has_include(<blend2d.h>)
#include <blend2d.h>
#include "blend2d_render_device.h"

#include <vector>
#include <cmath>
#include <cstdio>
#include <cstring>

// ═══════════════════════════════════════════════════════════
// 工具: 生成测试数据
// ═══════════════════════════════════════════════════════════

void generateDampedSine(int n, std::vector<double>& xs, std::vector<double>& ys) {
    xs.resize(n);
    ys.resize(n);
    for (int i = 0; i < n; ++i) {
        xs[i] = static_cast<double>(i) * 8.0 * 3.14159265 / (n - 1);
        double damping = std::exp(-xs[i] * 0.15);
        ys[i] = damping * std::sin(xs[i]) * 10.0;
    }
}

void generateGaussian(int n, double center, double sigma, double amp,
                      std::vector<double>& xs, std::vector<double>& ys) {
    xs.resize(n);
    ys.resize(n);
    for (int i = 0; i < n; ++i) {
        xs[i] = static_cast<double>(i) * 8.0 * 3.14159265 / (n - 1);
        double dx = (xs[i] - center) / sigma;
        ys[i] = amp * std::exp(-0.5 * dx * dx);
    }
}

void generateScatter(int n, std::vector<double>& xs, std::vector<double>& ys) {
    xs.resize(n);
    ys.resize(n);
    for (int i = 0; i < n; ++i) {
        xs[i] = static_cast<double>(i) * 8.0 * 3.14159265 / (n - 1);
        // 确定性"随机"偏移
        double noise = ((i * 1103515245 + 12345) & 0x7fffffff) / 2147483647.0;
        ys[i] = 5.0 * std::sin(xs[i]) + 3.0 * (noise - 0.5);
    }
}

// ═══════════════════════════════════════════════════════════
// 主程序
// ═══════════════════════════════════════════════════════════
int main() {
    std::printf("╔═════════════════════════════════════════════╗\n");
    std::printf("║  XYPlot SDK — Blend2D Rendering Demo        ║\n");
    std::printf("╚═════════════════════════════════════════════╝\n\n");

    // ==========================================
    // 创建图表
    // ==========================================
    xyplot::Plot plot;

    // -- 阻尼正弦波 (Line) --
    std::vector<double> sxs, sys;
    generateDampedSine(250, sxs, sys);
    plot.addLineSeries("Damped sin(x)", sxs.data(), sys.data(),
                       static_cast<int>(sxs.size()));

    // -- 高斯曲线 (Line, 虚线) --
    std::vector<double> gxs, gys;
    generateGaussian(200, 12.0, 3.0, 8.0, gxs, gys);
    int gid = plot.addLineSeries("Gaussian", gxs.data(), gys.data(),
                                  static_cast<int>(gxs.size()));
    xyplot::LineStyle gaussStyle;
    gaussStyle.color = xyplot::Color{255, 127, 14};
    gaussStyle.width = 2.5;
    gaussStyle.dash = xyplot::LineStyle::DashLine;
    plot.setSeriesStyle(gid, gaussStyle);

    // -- 散点图 --
    std::vector<double> nxs, nys;
    generateScatter(60, nxs, nys);
    plot.addScatterSeries("Noisy samples", nxs.data(), nys.data(),
                          static_cast<int>(nxs.size()));

    // -- 样式设置 --
    plot.setTitle("XYPlot Blend2D Demo — High Quality Vector Rendering");
    plot.xAxisSetLabel("x");
    plot.yAxisSetLabel("Amplitude");

    // ==========================================
    // 渲染到 Blend2D 图片
    // ==========================================
    const int kWidth  = 1200;
    const int kHeight = 800;

    BLImage image(kWidth, kHeight, BL_FORMAT_PRGB32);
    {
        Blend2DRenderDevice device(image);
        plot.render(device);
    }

    // ==========================================
    // 保存为 PNG
    // ==========================================
    const char* outputPath = "chart_output.png";

    // 确保 BLContext 已 flush（endFrame 已调用）
    BLResult result = image.writeToFile(outputPath);
    if (result == BL_SUCCESS) {
        std::printf("  ✅ Chart saved to: %s\n", outputPath);
        std::printf("     Size: %d × %d pixels\n", kWidth, kHeight);
    } else {
        std::printf("  ❌ Failed to save chart to: %s\n", outputPath);
        std::printf("     Blend2D error code: %d\n", static_cast<int>(result));
        return 1;
    }

    std::printf("\n");
    std::printf("═══════════════════════════════════════════\n");
    std::printf("  Demo completed successfully.\n");
    std::printf("  Open %s to view the chart.\n", outputPath);
    std::printf("═══════════════════════════════════════════\n");

    return 0;
}

#else // !__has_include(<blend2d.h>)

// ═══════════════════════════════════════════════════════════
// Blend2D 未安装时的回退
// ═══════════════════════════════════════════════════════════
#include <cstdio>

int main() {
    std::printf("╔═════════════════════════════════════════════╗\n");
    std::printf("║  XYPlot SDK — Blend2D Demo                  ║\n");
    std::printf("╚═════════════════════════════════════════════╝\n\n");

    std::printf("  ⚠️  Blend2D is not installed.\n\n");
    std::printf("  This example requires the Blend2D library:\n");
    std::printf("    https://github.com/blend2d/blend2d\n\n");
    std::printf("  To install Blend2D:\n");
    std::printf("    git clone https://github.com/blend2d/blend2d.git\n");
    std::printf("    cd blend2d && mkdir build && cd build\n");
    std::printf("    cmake .. && cmake --build . && cmake --install .\n\n");
    std::printf("  Or try the minimal example (no dependencies):\n");
    std::printf("    examples/minimal/main.cpp\n\n");

    std::printf("═══════════════════════════════════════════\n");
    std::printf("  Skipped — Blend2D not available.\n");
    std::printf("═══════════════════════════════════════════\n");

    return 0;
}

#endif // __has_include(<blend2d.h>)
