// ============================================================
// bar_plot.cpp — BarPlot 实现 (B1)
// ============================================================
// Owner: Agent D (图类型)
// 职责: 柱状图渲染 — 使用 fillRect 绘制垂直柱子
// 依赖: IPlotType 接口, IRenderDevice, 坐标变换
// ============================================================
#include "xyplot_internal.h"
#include <vector>
#include <cmath>
#include <algorithm>

namespace xyplot {
namespace internal {

class BarPlot : public IPlotType {
public:
    const char* typeName() const override { return "Bar"; }

    void render(IRenderDevice& device,
                const SeriesRenderData& data,
                const AxisRenderConfig& axis,
                const DevicePlotArea& area) override {
        if (data.count < 1 || !data.xs || !data.ys) return;

        // 1. 计算柱宽
        double width = data.barWidth;
        if (width <= 0.0 && data.count > 1) {
            // 自动宽度：相邻 x 间距的 70%
            double totalSpan = data.xs[data.count - 1] - data.xs[0];
            if (totalSpan <= 0) totalSpan = static_cast<double>(data.count);
            width = (totalSpan / static_cast<double>(data.count - 1)) * 0.7;
        }
        if (width <= 0.0) width = 1.0;

        double halfWidth = width / 2.0;

        // 2. 确定基线
        double baseline = axis.yMin;
        if (baseline > 0.0 && axis.yMax > 0.0) baseline = 0.0;

        // 3. 变换基线到设备坐标
        double baseY = transform::dataToDeviceY(baseline, axis, area);

        // 4. 逐柱渲染
        FillStyle fill;
        fill.color = data.lineStyle.color;
        if (fill.color.r == 0 && fill.color.g == 0 &&
            fill.color.b == 0 && fill.color.a == 255) {
            fill.color = {80, 120, 200};
        }

        for (int i = 0; i < data.count; i++) {
            double xLeft = transform::dataToDeviceX(data.xs[i] - halfWidth, axis, area);
            double xRight = transform::dataToDeviceX(data.xs[i] + halfWidth, axis, area);
            double yTop = transform::dataToDeviceY(data.ys[i], axis, area);

            double barLeft = std::min(xLeft, xRight);
            double barW = std::abs(xRight - xLeft);
            double barTop = std::min(yTop, baseY);
            double barH = std::abs(baseY - yTop);

            if (barW < 0.5) barW = 0.5;
            if (barH < 0.5) barH = 0.5;

            device.fillRect(barLeft, barTop, barW, barH, fill);
        }
    }
};

std::unique_ptr<IPlotType> createBarPlot() {
    return std::make_unique<BarPlot>();
}

} // namespace internal
} // namespace xyplot
