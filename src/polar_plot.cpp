// ============================================================
// polar_plot.cpp — PolarPlot 实现 (B1)
// ============================================================
// Owner: Agent D (图类型)
// 职责: 极坐标图渲染 — 极坐标变换 + drawPolyline
// 依赖: IPlotType 接口, IRenderDevice, 坐标变换
//
// 极坐标变换桩 — Agent C 的 polar_transform.h 就绪后可替换
// ============================================================
#include "xyplot_internal.h"
#include <vector>
#include <cmath>

namespace xyplot {
namespace internal {

// ============================================================
// 内联极坐标变换（Agent C 提供 polar_transform.h 后替换）
// ============================================================
namespace {

inline void polarToCartesian(double r, double theta,
                             double& x, double& y) {
    x = r * std::cos(theta);
    y = r * std::sin(theta);
}

inline void polarToCartesianBatch(const double* r, const double* theta,
                                  int count, double* outX, double* outY) {
    for (int i = 0; i < count; i++) {
        outX[i] = r[i] * std::cos(theta[i]);
        outY[i] = r[i] * std::sin(theta[i]);
    }
}

} // anonymous namespace

// ============================================================
// PolarPlot 实现
// ============================================================
class PolarPlot : public IPlotType {
public:
    const char* typeName() const override { return "Polar"; }

    void render(IRenderDevice& device,
                const SeriesRenderData& data,
                const AxisRenderConfig& axis,
                const DevicePlotArea& area) override {
        (void)axis;  // 极坐标使用独立的笛卡尔映射，不依赖 axis 配置
        if (data.count < 1 || !data.xs || !data.ys) return;

        // 1. 极坐标变换：theta = xs (弧度), r = ys
        std::vector<double> cartX(static_cast<size_t>(data.count));
        std::vector<double> cartY(static_cast<size_t>(data.count));
        polarToCartesianBatch(data.ys, data.xs, data.count,
                              cartX.data(), cartY.data());

        // 2. 确定笛卡尔坐标范围
        double maxR = 0.0;
        for (int i = 0; i < data.count; i++) {
            double r = std::abs(data.ys[i]);
            if (r > maxR) maxR = r;
        }
        if (maxR <= 0.0) maxR = 1.0;
        // 添加 10% 边距
        maxR *= 1.1;

        // 3. 映射到设备坐标
        //    使用 plotArea 的中心作为极点
        double cx = area.left + area.width / 2.0;
        double cy = area.top + area.height / 2.0;
        double scale = std::min(area.width, area.height) / (2.0 * maxR);

        std::vector<double> dx(static_cast<size_t>(data.count));
        std::vector<double> dy(static_cast<size_t>(data.count));
        for (int i = 0; i < data.count; i++) {
            dx[i] = cx + cartX[i] * scale;
            dy[i] = cy - cartY[i] * scale;  // Y 轴翻转
        }

        // 4. 渲染极坐标折线
        device.drawPolyline(dx.data(), dy.data(), data.count, data.lineStyle);

        // 5. 如果有关闭标记且数据形成闭环，叠加标记
        if (data.markerStyle.size > 0.0) {
            MarkerStyle style = data.markerStyle;
            // 从 lineStyle.color 继承颜色 (BUG-006 fix)
            if (style.fillColor.r == 0 && style.fillColor.g == 0 &&
                style.fillColor.b == 0 && style.fillColor.a == 255) {
                style.fillColor = data.lineStyle.color;
                style.fillColor.a = 200;
            }
            if (style.edgeColor.r == 0 && style.edgeColor.g == 0 &&
                style.edgeColor.b == 0 && style.edgeColor.a == 255) {
                style.edgeColor = data.lineStyle.color;
            }
            device.drawMarkers(dx.data(), dy.data(), data.count, style);
        }
    }
};

std::unique_ptr<IPlotType> createPolarPlot() {
    return std::make_unique<PolarPlot>();
}

} // namespace internal
} // namespace xyplot
