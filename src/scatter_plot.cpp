// ============================================================
// scatter_plot.cpp — ScatterPlot 实现
// ============================================================
// Owner: Agent D (图类型)
// 职责: 散点图渲染 — 使用 drawMarkers 绘制离散数据点
// 依赖: IPlotType 接口, IRenderDevice, 坐标变换
// ============================================================
#include "xyplot_internal.h"
#include <vector>
#include <cmath>

namespace xyplot {
namespace internal {

class ScatterPlot : public IPlotType {
public:
    const char* typeName() const override { return "Scatter"; }

    void render(IRenderDevice& device,
                const SeriesRenderData& data,
                const AxisRenderConfig& axis,
                const DevicePlotArea& area) override {
        if (data.count < 1) return;

        // 1. 坐标变换：data → device
        std::vector<double> dx(static_cast<size_t>(data.count));
        std::vector<double> dy(static_cast<size_t>(data.count));
        transform::transformPoints(data.xs, data.ys, data.count,
                                    axis, area, dx.data(), dy.data());

        // 2. 使用标记样式绘制所有数据点
        MarkerStyle style = data.markerStyle;

        // 如果标记尺寸为 0，使用默认散点尺寸
        if (style.size <= 0.0) {
            style.size = 6.0;
            style.shape = MarkerStyle::Circle;
        }

        // 如果填充色为默认值 (黑色, a=255)，从线条颜色继承
        if (style.fillColor.r == 0 && style.fillColor.g == 0 &&
            style.fillColor.b == 0 && style.fillColor.a == 255) {
            style.fillColor = data.lineStyle.color;
            style.fillColor.a = 200;
        }

        // 如果边缘色为默认值，使用线条颜色
        if (style.edgeColor.r == 0 && style.edgeColor.g == 0 &&
            style.edgeColor.b == 0 && style.edgeColor.a == 255) {
            style.edgeColor = data.lineStyle.color;
        }

        device.drawMarkers(dx.data(), dy.data(), data.count, style);
    }
};

// ============================================================
// 工厂函数（供 PlotRegistry 注册）
// ============================================================
std::unique_ptr<IPlotType> createScatterPlot() {
    return std::make_unique<ScatterPlot>();
}

} // namespace internal
} // namespace xyplot
