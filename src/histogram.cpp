// ============================================================
// histogram.cpp — HistogramPlot 实现 (B1)
// ============================================================
// Owner: Agent D (图类型)
// 职责: 直方图渲染 — 自动分箱 + fillRect 绘制柱
// 依赖: IPlotType 接口, IRenderDevice, 坐标变换
// ============================================================
#include "xyplot_internal.h"
#include <vector>
#include <cmath>
#include <algorithm>

namespace xyplot {
namespace internal {

class HistogramPlot : public IPlotType {
public:
    const char* typeName() const override { return "Histogram"; }

    void render(IRenderDevice& device,
                const SeriesRenderData& data,
                const AxisRenderConfig& axis,
                const DevicePlotArea& area) override {
        if (data.count < 1 || !data.ys) return;

        // 1. 自动分箱
        //    使用 Sturges 公式: bins = ceil(log2(n) + 1)
        int numBins = static_cast<int>(std::ceil(std::log2(data.count) + 1.0));
        if (numBins < 3) numBins = 3;
        if (numBins > 50) numBins = 50;

        // 2. 找到数据范围
        double minVal = data.ys[0], maxVal = data.ys[0];
        for (int i = 1; i < data.count; i++) {
            if (data.ys[i] < minVal) minVal = data.ys[i];
            if (data.ys[i] > maxVal) maxVal = data.ys[i];
        }
        double dataRange = maxVal - minVal;
        if (dataRange <= 0.0) dataRange = 1.0;
        // 扩展 1% 避免边界点落在 bin 外
        minVal -= dataRange * 0.01;
        maxVal += dataRange * 0.01;
        dataRange = maxVal - minVal;

        double binWidth = dataRange / static_cast<double>(numBins);

        // 3. 计数落入每个 bin 的数据点
        std::vector<int> counts(static_cast<size_t>(numBins), 0);
        for (int i = 0; i < data.count; i++) {
            int binIdx = static_cast<int>((data.ys[i] - minVal) / binWidth);
            if (binIdx < 0) binIdx = 0;
            if (binIdx >= numBins) binIdx = numBins - 1;
            counts[static_cast<size_t>(binIdx)]++;
        }

        // 4. 渲染柱状图
        FillStyle fill;
        fill.color = data.lineStyle.color;
        if (fill.color.r == 0 && fill.color.g == 0 &&
            fill.color.b == 0 && fill.color.a == 255) {
            fill.color = {80, 160, 80};  // 默认绿色
        }

        for (int i = 0; i < numBins; i++) {
            if (counts[static_cast<size_t>(i)] == 0) continue;

            double binStart = minVal + static_cast<double>(i) * binWidth;
            double binEnd = binStart + binWidth;

            // 变换到设备坐标
            double xLeft = transform::dataToDeviceX(binStart, axis, area);
            double xRight = transform::dataToDeviceX(binEnd, axis, area);
            // Y 轴: 频率作为高度
            AxisRenderConfig freqAxis = axis;
            freqAxis.yMin = 0;
            freqAxis.yMax = *std::max_element(counts.begin(), counts.end());
            if (freqAxis.yMax <= 0) freqAxis.yMax = 1;

            double yTop = transform::dataToDeviceY(
                static_cast<double>(counts[static_cast<size_t>(i)]), freqAxis, area);
            double yBase = transform::dataToDeviceY(0.0, freqAxis, area);

            double barLeft = std::min(xLeft, xRight);
            double barW = std::abs(xRight - xLeft);
            double barTop = std::min(yTop, yBase);
            double barH = std::abs(yBase - yTop);

            if (barW < 0.5) barW = 0.5;
            if (barH < 0.5) barH = 0.5;

            device.fillRect(barLeft, barTop, barW, barH, fill);
        }
    }
};

std::unique_ptr<IPlotType> createHistogramPlot() {
    return std::make_unique<HistogramPlot>();
}

} // namespace internal
} // namespace xyplot
