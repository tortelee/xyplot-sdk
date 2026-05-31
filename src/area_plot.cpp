// ============================================================
// area_plot.cpp — AreaPlot 实现 (B2)
// ============================================================
// Owner: Agent D (图类型)
// 职责: 面积图渲染 — fillPolygon 填充区域 + drawPolyline 边界
// 依赖: IPlotType 接口, IRenderDevice (fillPolygon + drawPolyline), 坐标变换
// ============================================================
#include "xyplot_internal.h"
#include <vector>
#include <algorithm>

namespace xyplot {
namespace internal {

class AreaPlot : public IPlotType {
public:
    const char* typeName() const override { return "Area"; }

    void render(IRenderDevice& device,
                const SeriesRenderData& data,
                const AxisRenderConfig& axis,
                const DevicePlotArea& area) override {
        if (data.count < 2 || !data.xs || !data.ys) return;

        int n = data.count;

        // 1. 坐标变换
        std::vector<double> dx(static_cast<size_t>(n));
        std::vector<double> dy(static_cast<size_t>(n));
        transform::transformPoints(data.xs, data.ys, n,
                                    axis, area, dx.data(), dy.data());

        // 2. 确定基线 Y（设备坐标）
        double baseline = axis.yMin;
        if (baseline > 0.0 && axis.yMax > 0.0) baseline = 0.0;
        double baseY = transform::dataToDeviceY(baseline, axis, area);

        // 3. 构建填充多边形：边界 + 基线闭合
        //    (x[0],base) → (x[0],y[0]) → ... → (x[n-1],y[n-1]) → (x[n-1],base)
        int polyN = n * 2;
        std::vector<double> px(static_cast<size_t>(polyN));
        std::vector<double> py(static_cast<size_t>(polyN));

        // 正向：沿数据点
        for (int i = 0; i < n; i++) {
            px[static_cast<size_t>(i)] = dx[i];
            py[static_cast<size_t>(i)] = dy[i];
        }
        // 反向：沿基线返回
        for (int i = 0; i < n; i++) {
            px[static_cast<size_t>(n + i)] = dx[static_cast<size_t>(n - 1 - i)];
            py[static_cast<size_t>(n + i)] = baseY;
        }

        // 4. 填充样式
        FillStyle fill = data.areaFill;
        if (fill.color.r == 0 && fill.color.g == 0 &&
            fill.color.b == 0 && fill.color.a == 255) {
            // 默认：线条颜色的半透明版本
            fill.color = data.lineStyle.color;
            fill.color.a = 80;
        }

        // 5. 渲染填充区域
        device.fillPolygon(px.data(), py.data(), polyN, fill);

        // 6. 渲染边界线
        device.drawPolyline(dx.data(), dy.data(), n, data.lineStyle);
    }
};

std::unique_ptr<IPlotType> createAreaPlot() {
    return std::make_unique<AreaPlot>();
}

} // namespace internal
} // namespace xyplot
