// ============================================================
// step_plot.cpp — StepPlot 实现 (B1)
// ============================================================
// Owner: Agent D (图类型)
// 职责: 阶梯图渲染 — 使用 drawPolyline 绘制阶梯状路径
// 依赖: IPlotType 接口, IRenderDevice, 坐标变换
// ============================================================
#include "xyplot_internal.h"
#include <vector>
#include <algorithm>

namespace xyplot {
namespace internal {

class StepPlot : public IPlotType {
public:
    const char* typeName() const override { return "Step"; }

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

        // 2. 构建阶梯路径 — 每个数据段产生 2 个中间点
        //    先水平后垂直 (stepPreHorizontal=true):
        //      (x0,y0) → (x1,y0) → (x1,y1)
        //    先垂直后水平 (stepPreHorizontal=false):
        //      (x0,y0) → (x0,y1) → (x1,y1)
        int stepCount = 1 + (data.count - 1) * 2;
        std::vector<double> sx(static_cast<size_t>(stepCount));
        std::vector<double> sy(static_cast<size_t>(stepCount));

        sx[0] = dx[0];
        sy[0] = dy[0];
        int outIdx = 1;

        for (int i = 1; i < data.count; i++) {
            if (data.stepPreHorizontal) {
                // 先水平: (x[i-1],y[i-1]) → (x[i],y[i-1]) → (x[i],y[i])
                sx[static_cast<size_t>(outIdx)] = dx[i];
                sy[static_cast<size_t>(outIdx)] = dy[i - 1];
                outIdx++;
            } else {
                // 先垂直: (x[i-1],y[i-1]) → (x[i-1],y[i]) → (x[i],y[i])
                sx[static_cast<size_t>(outIdx)] = dx[i - 1];
                sy[static_cast<size_t>(outIdx)] = dy[i];
                outIdx++;
            }
            sx[static_cast<size_t>(outIdx)] = dx[i];
            sy[static_cast<size_t>(outIdx)] = dy[i];
            outIdx++;
        }

        // 3. 渲染阶梯折线
        device.drawPolyline(sx.data(), sy.data(), stepCount, data.lineStyle);
    }
};

std::unique_ptr<IPlotType> createStepPlot() {
    return std::make_unique<StepPlot>();
}

} // namespace internal
} // namespace xyplot
