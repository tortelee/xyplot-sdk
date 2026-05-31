// ============================================================
// line_plot.cpp — LinePlot 实现
// ============================================================
// Owner: Agent D (图类型)
// 职责: 折线图渲染 — 将 Series 数据映射到设备坐标并调用 drawPolyline
// 依赖: IPlotType 接口, IRenderDevice, 坐标变换
// ============================================================
#include "xyplot_internal.h"
#include <vector>
#include <cmath>
#include <algorithm>

namespace xyplot {
namespace internal {

class LinePlot : public IPlotType {
public:
    const char* typeName() const override { return "Line"; }

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

        // 2. 调用设备绘制折线
        device.drawPolyline(dx.data(), dy.data(), data.count, data.lineStyle);

        // 3. 如果有标记样式（标记大小 > 0），叠加绘制标记
        if (data.markerStyle.size > 0.0) {
            device.drawMarkers(dx.data(), dy.data(), data.count,
                              data.markerStyle);
        }
    }
};

// ============================================================
// 工厂函数（供 PlotRegistry 注册）
// ============================================================
std::unique_ptr<IPlotType> createLinePlot() {
    return std::make_unique<LinePlot>();
}

} // namespace internal
} // namespace xyplot
