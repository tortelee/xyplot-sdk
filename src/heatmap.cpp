// ============================================================
// heatmap.cpp — HeatmapPlot 实现 (B2)
// ============================================================
// Owner: Agent D (图类型)
// 职责: 热力图渲染 — drawImage 像素渲染 + 色条图例
// 依赖: IPlotType 接口, IRenderDevice (drawImage + fillRect + drawText)
// ============================================================
#include "xyplot_internal.h"
#include <vector>
#include <cstring>
#include <algorithm>
#include <cmath>

namespace xyplot {
namespace internal {

class HeatmapPlot : public IPlotType {
public:
    const char* typeName() const override { return "Heatmap"; }

    void render(IRenderDevice& device,
                const SeriesRenderData& data,
                const AxisRenderConfig& axis,
                const DevicePlotArea& area) override {
        (void)axis;  // 热力图使用网格+色条，不依赖轴配置
        int rows = data.gridRows;
        int cols = data.gridCols;
        if (rows < 1 || cols < 1 || !data.ys) return;

        // 1. 找到数据范围
        int totalCells = rows * cols;
        double minVal = data.ys[0], maxVal = data.ys[0];
        for (int i = 1; i < totalCells; i++) {
            if (data.ys[i] < minVal) minVal = data.ys[i];
            if (data.ys[i] > maxVal) maxVal = data.ys[i];
        }
        double valRange = maxVal - minVal;
        if (valRange <= 0.0) valRange = 1.0;

        // 2. 生成 RGBA 图像
        //    使用 Jet 色条: 蓝→青→绿→黄→红
        std::vector<uint8_t> rgba(static_cast<size_t>(cols * rows * 4));
        for (int r = 0; r < rows; r++) {
            for (int c = 0; c < cols; c++) {
                double val = data.ys[r * cols + c];
                double t = (val - minVal) / valRange;
                if (t < 0.0) t = 0.0;
                if (t > 1.0) t = 1.0;

                uint8_t R, G, B;
                jetColorMap(t, R, G, B);

                int idx = (r * cols + c) * 4;
                rgba[static_cast<size_t>(idx)] = R;
                rgba[static_cast<size_t>(idx + 1)] = G;
                rgba[static_cast<size_t>(idx + 2)] = B;
                rgba[static_cast<size_t>(idx + 3)] = 255;
            }
        }

        // 3. 渲染热力图到设备
        double hmLeft = area.left;
        double hmTop = area.top;
        double hmW = area.width;
        double hmH = area.height;

        // 预留色条空间
        double colorBarWidth = 30.0;
        double hmPlotW = hmW - colorBarWidth - 10.0;
        if (hmPlotW < 20.0) hmPlotW = hmW;

        device.drawImage(hmLeft, hmTop, hmPlotW, hmH,
                        rgba.data(), cols, rows);

        // 4. 渲染色条
        renderColorBar(device, hmLeft + hmPlotW + 6.0, hmTop,
                       colorBarWidth - 6.0, hmH, minVal, maxVal);
    }

private:
    // Jet colormap: 蓝(0) → 青 → 绿 → 黄 → 红(1)
    static void jetColorMap(double t, uint8_t& r, uint8_t& g, uint8_t& b) {
        if (t < 0.25) {
            // 蓝 → 青
            double s = t / 0.25;
            r = 0;
            g = static_cast<uint8_t>(s * 255.0);
            b = 255;
        } else if (t < 0.5) {
            // 青 → 绿
            double s = (t - 0.25) / 0.25;
            r = 0;
            g = 255;
            b = static_cast<uint8_t>((1.0 - s) * 255.0);
        } else if (t < 0.75) {
            // 绿 → 黄
            double s = (t - 0.5) / 0.25;
            r = static_cast<uint8_t>(s * 255.0);
            g = 255;
            b = 0;
        } else {
            // 黄 → 红
            double s = (t - 0.75) / 0.25;
            r = 255;
            g = static_cast<uint8_t>((1.0 - s) * 255.0);
            b = 0;
        }
    }

    static void renderColorBar(IRenderDevice& device,
                               double x, double y, double w, double h,
                               double minVal, double maxVal) {
        // 色条梯度 — 逐像素高度的 fillRect 条带
        int steps = 50;
        double stepH = h / static_cast<double>(steps);
        for (int i = 0; i < steps; i++) {
            double t = 1.0 - static_cast<double>(i) / static_cast<double>(steps - 1);
            uint8_t R, G, B;
            jetColorMap(t, R, G, B);

            FillStyle fs;
            fs.color = {R, G, B, 255};
            device.fillRect(x, y + static_cast<double>(i) * stepH, w, stepH + 0.5, fs);
        }

        // 色条边框
        LineStyle borderStyle;
        borderStyle.width = 1.0;
        borderStyle.color = {0, 0, 0};
        double bx[] = {x, x + w, x + w, x, x};
        double by[] = {y, y, y + h, y + h, y};
        device.drawPolyline(bx, by, 5, borderStyle);

        // 标签
        FontDesc labelFont;
        labelFont.size = 9;
        TextStyle labelStyle;
        labelStyle.hAlign = TextStyle::Left;

        char buf[32];
        std::snprintf(buf, sizeof(buf), "%.1f", maxVal);
        device.drawText(x + w + 3, y + 8, buf, labelFont, labelStyle);

        std::snprintf(buf, sizeof(buf), "%.1f", minVal);
        device.drawText(x + w + 3, y + h, buf, labelFont, labelStyle);
    }
};

std::unique_ptr<IPlotType> createHeatmapPlot() {
    return std::make_unique<HeatmapPlot>();
}

} // namespace internal
} // namespace xyplot
