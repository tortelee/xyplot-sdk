// ============================================================
// multi_axis.cpp — 多 Y 轴管理器
// ============================================================
// Owner: Agent D (图类型)
// 职责: 管理多个 Y 轴（左侧主轴 + 右侧副轴）的配置、范围、标签
// 依赖: types.h, axis_system.h (Agent C), coordinate_transform.h (Agent C)
// ============================================================
#include "xyplot_internal.h"
#include "axis_system.h"
#include <stdexcept>

namespace xyplot {
namespace internal {

// ============================================================
// MultiAxisManager 实现
// ============================================================

MultiAxisManager::MultiAxisManager() {
    m_axes.push_back(YAxisInfo{});
}

int MultiAxisManager::addRightAxis(const char* label) {
    YAxisInfo info;
    info.label = label ? label : "";
    info.isRight = true;
    m_axes.push_back(info);
    return static_cast<int>(m_axes.size()) - 1;
}

int MultiAxisManager::count() const {
    return static_cast<int>(m_axes.size());
}

int MultiAxisManager::leftAxisCount() const {
    int n = 0;
    for (auto& a : m_axes) if (!a.isRight) n++;
    return n;
}

int MultiAxisManager::rightAxisCount() const {
    int n = 0;
    for (auto& a : m_axes) if (a.isRight) n++;
    return n;
}

const YAxisInfo& MultiAxisManager::axis(int index) const {
    return m_axes.at(static_cast<size_t>(index));
}

YAxisInfo& MultiAxisManager::axis(int index) {
    return m_axes.at(static_cast<size_t>(index));
}

void MultiAxisManager::setLabel(int index, const char* label) {
    m_axes.at(static_cast<size_t>(index)).label = label ? label : "";
}

void MultiAxisManager::setRange(int index, double yMin, double yMax) {
    auto& ax = m_axes.at(static_cast<size_t>(index));
    ax.dataMin = yMin;
    ax.dataMax = yMax;
}

void MultiAxisManager::setScale(int index, ScaleType scale) {
    m_axes.at(static_cast<size_t>(index)).scale = scale;
}

void MultiAxisManager::autoRange(int axisIndex,
                                 const std::vector<SeriesRenderData>& seriesList) {
    auto& ax = m_axes.at(static_cast<size_t>(axisIndex));
    bool hasData = false;
    double minVal = 0, maxVal = 0;

    for (auto& s : seriesList) {
        if (s.yAxisIndex != axisIndex) continue;
        if (s.count <= 0) continue;
        for (int i = 0; i < s.count; i++) {
            double v = s.ys[i];
            if (!hasData) {
                minVal = maxVal = v;
                hasData = true;
            } else {
                if (v < minVal) minVal = v;
                if (v > maxVal) maxVal = v;
            }
        }
    }

    if (hasData) {
        double margin = (maxVal - minVal) * 0.05;
        if (margin < 1e-10) margin = 1.0;
        ax.dataMin = minVal - margin;
        ax.dataMax = maxVal + margin;
    }
}

void MultiAxisManager::renderAxisLabels(IRenderDevice& device,
                                        const DevicePlotArea& area,
                                        double leftMargin, double /*rightMargin*/) const {
    FontDesc labelFont;
    labelFont.size = 12;

    for (size_t i = 0; i < m_axes.size(); i++) {
        if (m_axes[i].isRight) continue;
        if (m_axes[i].label.empty()) continue;

        TextStyle style;
        style.hAlign = TextStyle::Center;
        style.vAlign = TextStyle::Middle;

        double labelX = leftMargin - 10;
        double labelY = area.top + area.height / 2.0;

        int leftIdx = 0;
        for (size_t j = 0; j <= i; j++) {
            if (!m_axes[j].isRight) leftIdx++;
        }
        labelY += (leftIdx - 1) * 20;

        device.drawText(labelX, labelY,
                       m_axes[i].label.c_str(),
                       labelFont, style);
    }

    for (size_t i = 0; i < m_axes.size(); i++) {
        if (!m_axes[i].isRight) continue;
        if (m_axes[i].label.empty()) continue;

        TextStyle style;
        style.hAlign = TextStyle::Center;
        style.vAlign = TextStyle::Middle;

        double labelX = area.left + area.width + 10;
        double labelY = area.top + area.height / 2.0;

        int rightIdx = 0;
        for (size_t j = 0; j <= i; j++) {
            if (m_axes[j].isRight) rightIdx++;
        }
        labelY += (rightIdx - 1) * 20;

        device.drawText(labelX, labelY,
                       m_axes[i].label.c_str(),
                       labelFont, style);
    }
}

void MultiAxisManager::renderTicksAndGrid(int axisIndex, IRenderDevice& device,
                                          const DevicePlotArea& area, bool isX) const {
    const auto& ax = m_axes.at(static_cast<size_t>(axisIndex));

    double dataMin = isX ? 0.0 : ax.dataMin;
    double dataMax = isX ? 1.0 : ax.dataMax;
    (void)isX;

    if (dataMax <= dataMin) return;

    // ──── 使用 Agent C 的 computeTicks() 替代内联 Nice Number ────
    AxisConfig axisCfg;
    axisCfg.dataMin = dataMin;
    axisCfg.dataMax = dataMax;
    axisCfg.scaleType = ax.scale;
    axisCfg.targetMajorTicks = 5;
    axisCfg.targetMinorTicks = 0;

    AxisTicks ticks = computeTicks(axisCfg);

    LineStyle gridStyle;
    gridStyle.width = 0.5;
    gridStyle.color = {200, 200, 200};
    gridStyle.dash = LineStyle::DashLine;

    LineStyle axisStyle;
    axisStyle.width = 1.5;
    axisStyle.color = {0, 0, 0};

    if (!isX) {
        bool isRightAxis = ax.isRight;
        double tickX = isRightAxis ? (area.left + area.width) : area.left;

        for (size_t i = 0; i < ticks.majorTicks.size(); i++) {
            double tickVal = ticks.majorTicks[i];
            const std::string& label = ticks.labels[i];

            AxisRenderConfig tmpCfg;
            tmpCfg.yMin = dataMin;
            tmpCfg.yMax = dataMax;
            tmpCfg.yScale = ax.scale;
            double tickY = transform::dataToDeviceY(tickVal, tmpCfg, area);

            if (tickY < area.top - 5 || tickY > area.top + area.height + 5)
                continue;

            // 网格线（水平线横跨绘图区）
            double gx[] = {area.left, area.left + area.width};
            double gy[] = {tickY, tickY};
            device.drawPolyline(gx, gy, 2, gridStyle);

            // 刻度标记
            double tickLen = isRightAxis ? -5.0 : 5.0;
            double tx[] = {tickX, tickX + tickLen};
            double ty[] = {tickY, tickY};
            device.drawPolyline(tx, ty, 2, axisStyle);

            // 刻度标签（使用 Agent C 的 formatTickLabel 格式化）
            FontDesc tickFont;
            tickFont.size = 10;
            TextStyle tickStyle;
            tickStyle.hAlign = isRightAxis ? TextStyle::Left : TextStyle::Right;
            tickStyle.vAlign = TextStyle::Middle;
            double labelX = tickX + (isRightAxis ? 6.0 : -6.0);
            device.drawText(labelX, tickY, label.c_str(), tickFont, tickStyle);
        }

        // 轴线本体
        double axLineY[] = {area.top, area.top + area.height};
        double axLineX[] = {tickX, tickX};
        device.drawPolyline(axLineX, axLineY, 2, axisStyle);
    }
}

} // namespace internal
} // namespace xyplot
