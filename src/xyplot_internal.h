// ============================================================
// xyplot_internal.h — Agent D 内部接口定义
// ============================================================
// Owner: Agent D (图类型)
// 用途: 定义 IPlotType 接口 + 坐标变换适配器（委托 Agent C 实现）
// 依赖: include/xyplot/types.h, include/xyplot/irender_device.h (已冻结)
//       coordinate_transform.h (Agent C), axis_system.h (Agent C)
// ============================================================
#pragma once
#include "xyplot/xyplot.h"
#include "coordinate_transform.h"
#include <vector>
#include <string>
#include <memory>
#include <functional>

namespace xyplot {
namespace internal {

// ============================================================
// Section 1: 渲染上下文数据结构
// ============================================================

/// 单个 Series 的渲染数据（轻量指针视图）
struct SeriesRenderData {
    const double* xs = nullptr;
    const double* ys = nullptr;
    int count = 0;
    LineStyle lineStyle{};
    MarkerStyle markerStyle{};
    int yAxisIndex = 0;
    const char* name = "";
};

/// 轴配置（用于坐标变换）
struct AxisRenderConfig {
    double xMin = 0, xMax = 1;
    double yMin = 0, yMax = 1;      // 当前 Y 轴的 data range
    ScaleType xScale = ScaleType::Linear;
    ScaleType yScale = ScaleType::Linear;
};

/// 设备空间绘图区域
struct DevicePlotArea {
    double left = 0, top = 0, width = 0, height = 0;
};

// ============================================================
// Section 2: IPlotType — 图类型抽象接口
// ============================================================
/// 每种图类型（Line / Scatter / ...）只需实现此接口。
/// Agent D 实现，Agent E 通过 PlotRegistry 调用。
class IPlotType {
public:
    virtual ~IPlotType() = default;

    /// 返回图类型名称（如 "Line", "Scatter"）
    virtual const char* typeName() const = 0;

    /// 渲染一个 Series
    /// @param device  渲染设备（由 Agent E 传入）
    /// @param data    该 Series 的 xs/ys 数据 + 样式
    /// @param axis    当前轴配置（范围 & 刻度类型）
    /// @param area    设备坐标系下的绘图区域
    virtual void render(IRenderDevice& device,
                        const SeriesRenderData& data,
                        const AxisRenderConfig& axis,
                        const DevicePlotArea& area) = 0;
};

// ============================================================
// Section 3: 坐标变换适配器（委托 Agent C 的 coordinate_transform.h）
// ============================================================
// 所有变换逻辑由 Agent C 的 xyplot::transform() / xyplot::transformPoints() 实现。
// 此命名空间提供 thin wrapper，将 Agent D 的 struct 参数适配为 Agent C 的 flat API。
namespace transform {

/// 将 data X 坐标映射到设备 X 坐标
inline double dataToDeviceX(double dataX, const AxisRenderConfig& axis,
                             const DevicePlotArea& area) {
    return xyplot::transform(dataX, axis.xMin, axis.xMax,
                             area.left, area.left + area.width,
                             axis.xScale);
}

/// 将 data Y 坐标映射到设备 Y 坐标（Y 轴方向翻转：data 增大 → device 上移）
inline double dataToDeviceY(double dataY, const AxisRenderConfig& axis,
                             const DevicePlotArea& area) {
    // xyplot::transform() 是通用线性映射，手动传入反转的 device 范围来翻转 Y
    return xyplot::transform(dataY, axis.yMin, axis.yMax,
                             area.top + area.height, area.top,
                             axis.yScale);
}

/// 批量坐标变换：data → device
inline void transformPoints(const double* srcXs, const double* srcYs, int count,
                             const AxisRenderConfig& axis,
                             const DevicePlotArea& area,
                             double* outXs, double* outYs) {
    // Agent C 的 transformPoints() 内置 Y 轴反转
    // deviceYMin/top → dataMax, deviceYMax/bottom → dataMin
    xyplot::transformPoints(srcXs, srcYs, count,
                            axis.xMin, axis.xMax,
                            axis.yMin, axis.yMax,
                            area.left, area.left + area.width,
                            area.top, area.top + area.height,
                            outXs, outYs,
                            axis.xScale, axis.yScale);
}

} // namespace transform

// ============================================================
// Section 4: 图例条目
// ============================================================
struct LegendEntry {
    std::string name;
    Color swatchColor;
    LineStyle lineStyle{};   // 用于绘制图例中的线段样本
};

// ============================================================
// Section 5: Y 轴信息
// ============================================================
struct YAxisInfo {
    double dataMin = 0, dataMax = 1;
    ScaleType scale = ScaleType::Linear;
    std::string label;
    bool isRight = false;    // false = 左侧 Y 轴, true = 右侧 Y 轴
};

// ============================================================
// Section 6: MultiAxisManager — 多 Y 轴管理器
// ============================================================
class MultiAxisManager {
public:
    MultiAxisManager();

    int addRightAxis(const char* label = "");
    int count() const;
    int leftAxisCount() const;
    int rightAxisCount() const;
    const YAxisInfo& axis(int index) const;
    YAxisInfo& axis(int index);
    void setLabel(int index, const char* label);
    void setRange(int index, double yMin, double yMax);
    void setScale(int index, ScaleType scale);
    void autoRange(int axisIndex, const std::vector<SeriesRenderData>& seriesList);

    void renderAxisLabels(IRenderDevice& device,
                          const DevicePlotArea& area,
                          double leftMargin, double rightMargin) const;

    void renderTicksAndGrid(int axisIndex, IRenderDevice& device,
                            const DevicePlotArea& area, bool isX) const;

private:
    std::vector<YAxisInfo> m_axes;
};

// ============================================================
// Section 7: LegendRenderer — 图例布局与渲染
// ============================================================
class LegendRenderer {
public:
    struct ItemLayout {
        double swatchX, swatchY;
        double swatchW, swatchH;
        double textX, textY;
        const LegendEntry* entry;
    };

    std::vector<ItemLayout> layout(const std::vector<LegendEntry>& entries,
                                    const DevicePlotArea& plotArea,
                                    double fontSize = 11.0);

    void render(IRenderDevice& device,
                const std::vector<LegendEntry>& entries,
                const DevicePlotArea& plotArea,
                double fontSize = 11.0);

    const Rect& boundingBox() const;

private:
    Rect m_boundingBox;
    static double estimateTextWidth(const char* text, double fontSize);
};

// ============================================================
// Section 8: PlotRegistry — 便捷函数声明
// ============================================================
using PlotTypeFactory = std::function<std::unique_ptr<IPlotType>()>;

std::unique_ptr<IPlotType> createPlotType(const std::string& name);
bool registerPlotType(const std::string& name, PlotTypeFactory factory);
std::vector<std::string> listPlotTypes();

// ============================================================
// Section 9: 工厂函数声明
// ============================================================
std::unique_ptr<IPlotType> createLinePlot();
std::unique_ptr<IPlotType> createScatterPlot();

} // namespace internal
} // namespace xyplot
