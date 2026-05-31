// ============================================================
// examples/minimal/main.cpp — XYPlot 最小示例
// ============================================================
// Owner: Agent F (文档 & 示例)
//
// 此示例展示 XYPlot SDK 的最小使用方式:
//   1. 实现一个控制台 IRenderDevice（打印所有渲染调用）
//   2. 创建 Plot，添加数据，设置样式
//   3. 调用 Plot::render()
//
// 零外部依赖，只用 C++17 标准库 + xyplot 头文件。
//
// 编译 (gcc/clang):
//   g++ -std=c++17 -I../../include main.cpp -L../../build -lxyplot -o minimal_example
//
// 编译 (MSVC):
//   cl /std:c++17 /I../../include main.cpp /link ../../build/xyplot.lib
//
// 运行:
//   ./minimal_example
// ============================================================

#include <xyplot/xyplot.h>

#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

// ═══════════════════════════════════════════════════════════
// ConsoleRenderDevice — 将渲染调用打印到控制台
// ═══════════════════════════════════════════════════════════
//
// 此设备实现 IRenderDevice 的 8 个纯虚方法，将所有调用
// 序列打印为可读的控制台输出。适合:
//   - 学习 SDK 的渲染调用顺序
//   - 调试自定义图类型
//   - 验证数据变换结果
//
// 对于实际产品，将此替换为你的渲染后端实现:
//   - Qt:       使用 QPainter 实现（见 ../qt_backend/）
//   - OpenGL:   使用 glDrawArrays / glDrawElements
//   - Skia:     使用 SkCanvas
//   - Direct2D: 使用 ID2D1RenderTarget
//
class ConsoleRenderDevice : public xyplot::IRenderDevice {
public:
    // ── 帧生命周期 ──
    void beginFrame() override {
        m_output.push_back("=== beginFrame ===");
    }
    void endFrame() override {
        m_output.push_back("=== endFrame ===");
        // 打印所有记录
        for (auto& line : m_output) {
            std::printf("%s\n", line.c_str());
        }
        m_output.clear();
    }

    // ── 裁剪 ──
    void setClipRect(double x, double y, double w, double h) override {
        char buf[256];
        std::snprintf(buf, sizeof(buf),
            "  setClipRect: [%.0f, %.0f, %.0f, %.0f]", x, y, w, h);
        m_output.push_back(buf);
    }
    void resetClip() override {
        m_output.push_back("  resetClip");
    }

    // ── 折线 ──
    void drawPolyline(const double* xs, const double* ys,
                      int count, const xyplot::LineStyle& style) override {
        char buf[512];
        std::snprintf(buf, sizeof(buf),
            "  drawPolyline: %d points, width=%.1f, color=(%d,%d,%d), dash=%d",
            count, style.width,
            style.color.r, style.color.g, style.color.b,
            static_cast<int>(style.dash));

        // 打印首尾点坐标以验证变换
        if (count >= 2) {
            std::snprintf(buf + std::strlen(buf), sizeof(buf) - std::strlen(buf),
                "\n    first: (%.1f, %.1f), last: (%.1f, %.1f)",
                xs[0], ys[0], xs[count-1], ys[count-1]);
        }
        m_output.push_back(buf);
    }

    // ── 标记点 ──
    void drawMarkers(const double* xs, const double* ys,
                     int count, const xyplot::MarkerStyle& style) override {
        char buf[512];
        std::snprintf(buf, sizeof(buf),
            "  drawMarkers: %d points, size=%.1f, shape=%d, "
            "fill=(%d,%d,%d,%d), edge=(%d,%d,%d)",
            count, style.size, static_cast<int>(style.shape),
            style.fillColor.r, style.fillColor.g,
            style.fillColor.b, style.fillColor.a,
            style.edgeColor.r, style.edgeColor.g, style.edgeColor.b);

        // 打印首点坐标
        if (count >= 1) {
            std::snprintf(buf + std::strlen(buf), sizeof(buf) - std::strlen(buf),
                "\n    first point at: (%.1f, %.1f)", xs[0], ys[0]);
        }
        m_output.push_back(buf);
    }

    // ── 文本 ──
    void drawText(double x, double y, const char* text,
                  const xyplot::FontDesc& font,
                  const xyplot::TextStyle& style) override {
        char buf[512];
        std::snprintf(buf, sizeof(buf),
            "  drawText: \"%s\" at (%.0f, %.0f), "
            "fontSize=%.0f, bold=%d, italic=%d, "
            "hAlign=%d, vAlign=%d",
            text, x, y,
            font.size, font.bold, font.italic,
            static_cast<int>(style.hAlign),
            static_cast<int>(style.vAlign));
        m_output.push_back(buf);
    }

    // ── 填充矩形 ──
    void fillRect(double x, double y, double w, double h,
                  const xyplot::FillStyle& style) override {
        char buf[256];
        std::snprintf(buf, sizeof(buf),
            "  fillRect: [%.0f, %.0f, %.0f, %.0f], "
            "color=(%d,%d,%d,%d)",
            x, y, w, h,
            style.color.r, style.color.g,
            style.color.b, style.color.a);
        m_output.push_back(buf);
    }

private:
    std::vector<std::string> m_output;
};

// ═══════════════════════════════════════════════════════════
// 工具: 生成测试数据
// ═══════════════════════════════════════════════════════════

/// 生成正弦波数据: y = A * sin(2π * x / period)
void generateSineWave(int n, double A, double period,
                      std::vector<double>& xs, std::vector<double>& ys) {
    xs.resize(n);
    ys.resize(n);
    for (int i = 0; i < n; ++i) {
        xs[i] = static_cast<double>(i) * 4.0 * 3.14159 / static_cast<double>(n - 1);
        ys[i] = A * std::sin(2.0 * 3.14159 * xs[i] / period);
    }
}

/// 生成带噪声的线性数据
void generateNoisyLine(int n, double slope, double intercept, double noise,
                       std::vector<double>& xs, std::vector<double>& ys) {
    xs.resize(n);
    ys.resize(n);
    // 简单确定性"噪声"模式（避免 std::rand 的平台差异）
    for (int i = 0; i < n; ++i) {
        xs[i] = static_cast<double>(i);
        double nz = ((i * 1103515245 + 12345) & 0x7fffffff) / 2147483647.0;
        ys[i] = slope * xs[i] + intercept + noise * (nz - 0.5);
    }
}

// ═══════════════════════════════════════════════════════════
// 主程序
// ═══════════════════════════════════════════════════════════
int main() {
    std::printf("╔═════════════════════════════════════════════╗\n");
    std::printf("║  XYPlot SDK — Minimal Example               ║\n");
    std::printf("║  Console Render Device — 打印渲染调用序列    ║\n");
    std::printf("╚═════════════════════════════════════════════╝\n\n");

    // ==========================================
    // 示例 1: 简单折线图 + 散点图
    // ==========================================
    {
        std::printf("─── 示例 1: 正弦波 + 噪声散点 ───\n\n");

        xyplot::Plot plot;

        // 生成正弦波数据
        std::vector<double> sxs, sys;
        generateSineWave(100, 10.0, 10.0, sxs, sys);
        plot.addLineSeries("Sine Wave", sxs.data(), sys.data(),
                           static_cast<int>(sxs.size()));

        // 生成噪声散点数据
        std::vector<double> nxs, nys;
        generateNoisyLine(30, 2.0, 0.0, 5.0, nxs, nys);
        plot.addScatterSeries("Noisy Data", nxs.data(), nys.data(),
                              static_cast<int>(nxs.size()));

        // 配置轴和标题
        plot.setTitle("Minimal Example: Sine Wave + Noise");
        plot.xAxisSetLabel("Time (s)");
        plot.yAxisSetLabel("Amplitude");

        // 渲染
        ConsoleRenderDevice console;
        plot.render(console);

        std::printf("\n");
    }

    // ==========================================
    // 示例 2: 自定义样式
    // ==========================================
    {
        std::printf("─── 示例 2: 自定义线条样式 ───\n\n");

        xyplot::Plot plot;

        double xs[] = {0.0, 1.0, 2.0, 3.0, 4.0};
        double ys[] = {0.0, 2.0, 1.0, 3.0, 2.0};

        int sid = plot.addLineSeries("Styled Line", xs, ys, 5);

        // 自定义样式: 红色虚线, 线宽 3px
        xyplot::LineStyle customStyle;
        customStyle.color = xyplot::Color{255, 0, 0, 255};
        customStyle.width = 3.0;
        customStyle.dash = xyplot::LineStyle::DashLine;
        plot.setSeriesStyle(sid, customStyle);

        plot.setTitle("Custom Style Demo");
        plot.xAxisSetLabel("X");
        plot.yAxisSetLabel("Y");

        ConsoleRenderDevice console;
        plot.render(console);

        std::printf("\n");
    }

    // ==========================================
    // 示例 3: 多 Y 轴
    // ==========================================
    {
        std::printf("─── 示例 3: 多 Y 轴 ───\n\n");

        xyplot::Plot plot;

        // 左侧 Y 轴数据 (温度)
        double tempX[] = {0.0, 1.0, 2.0, 3.0, 4.0};
        double tempY[] = {20.0, 22.0, 25.0, 24.0, 26.0};
        plot.addLineSeries("Temperature (°C)", tempX, tempY, 5);

        // 右侧 Y 轴数据 (百分比)
        double humidX[] = {0.0, 1.0, 2.0, 3.0, 4.0};
        double humidY[] = {60.0, 55.0, 65.0, 70.0, 58.0};
        plot.addLineSeries("Humidity (%)", humidX, humidY, 5);

        // 添加右轴
        plot.yAxisAddRight("Humidity (%)");
        plot.setAxisRange(0.0, 5.0, 60.0, 70.0, 1);  // 右轴范围

        plot.setTitle("Multi Y-Axis Demo");
        plot.xAxisSetLabel("Time (h)");
        plot.yAxisSetLabel("Temperature (°C)");

        ConsoleRenderDevice console;
        plot.render(console);

        std::printf("\n");
    }

    // ==========================================
    // 示例 4: 空图 + 边界情况
    // ==========================================
    {
        std::printf("─── 示例 4: 空图（无数据）───\n\n");

        xyplot::Plot plot;
        plot.setTitle("Empty Plot");
        plot.setAxisRange(0.0, 10.0, 0.0, 10.0);

        ConsoleRenderDevice console;
        plot.render(console);
        // 空图不崩溃 — 只渲染轴线和标题
        std::printf("\n");
    }

    std::printf("═══════════════════════════════════════════\n");
    std::printf("  All examples completed successfully.\n");
    std::printf("═══════════════════════════════════════════\n");

    return 0;
}
