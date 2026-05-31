// ============================================================
// error_bar.cpp — ErrorBar 实现 (B1)
// ============================================================
// Owner: Agent D (图类型)
// 职责: 误差线渲染 — 使用 drawPolyline 绘制垂直误差线和水平端帽
// 依赖: IPlotType 接口, IRenderDevice, 坐标变换
// ============================================================
#include "xyplot_internal.h"
#include <vector>
#include <cmath>

namespace xyplot {
namespace internal {

class ErrorBarPlot : public IPlotType {
public:
    const char* typeName() const override { return "ErrorBar"; }

    void render(IRenderDevice& device,
                const SeriesRenderData& data,
                const AxisRenderConfig& axis,
                const DevicePlotArea& area) override {
        if (data.count < 1 || !data.xs || !data.ys) return;

        // 1. 坐标变换
        std::vector<double> dx(static_cast<size_t>(data.count));
        std::vector<double> dy(static_cast<size_t>(data.count));
        transform::transformPoints(data.xs, data.ys, data.count,
                                    axis, area, dx.data(), dy.data());

        // 2. 计算误差范围（如果没有显式提供，使用标记尺寸作为默认误差值）
        double capWidth = 4.0; // 端帽半宽（设备像素）

        for (int i = 0; i < data.count; i++) {
            double lowVal, highVal;

            if (data.errorLow && data.errorHigh) {
                // 显式误差值（数据空间）
                lowVal = data.ys[i] - data.errorLow[i];
                highVal = data.ys[i] + data.errorHigh[i];
            } else if (data.errorLow) {
                // 对称误差
                lowVal = data.ys[i] - data.errorLow[i];
                highVal = data.ys[i] + data.errorLow[i];
            } else {
                // 默认：5% 误差
                double err = std::abs(data.ys[i]) * 0.05;
                if (err < 0.01) err = 0.01;
                lowVal = data.ys[i] - err;
                highVal = data.ys[i] + err;
            }

            // 3. 变换误差边界到设备坐标
            double yLow = transform::dataToDeviceY(lowVal, axis, area);
            double yHigh = transform::dataToDeviceY(highVal, axis, area);

            // 4. 绘制垂直误差线
            double vx[] = {dx[i], dx[i]};
            double vy[] = {yLow, yHigh};
            device.drawPolyline(vx, vy, 2, data.lineStyle);

            // 5. 绘制水平端帽
            double capTop[] = {dx[i] - capWidth, dx[i] + capWidth};
            double capYTop[] = {yHigh, yHigh};
            device.drawPolyline(capTop, capYTop, 2, data.lineStyle);

            double capBottom[] = {dx[i] - capWidth, dx[i] + capWidth};
            double capYBottom[] = {yLow, yLow};
            device.drawPolyline(capBottom, capYBottom, 2, data.lineStyle);
        }
    }
};

std::unique_ptr<IPlotType> createErrorBarPlot() {
    return std::make_unique<ErrorBarPlot>();
}

} // namespace internal
} // namespace xyplot
