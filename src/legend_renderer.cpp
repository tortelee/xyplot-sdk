// ============================================================
// legend_renderer.cpp — 图例布局与渲染
// ============================================================
// Owner: Agent D (图类型)
// 职责: 计算图例布局并在 IRenderDevice 上渲染图例项
// 依赖: types.h, irender_device.h
// ============================================================
#include "xyplot_internal.h"
#include <cstring>

namespace xyplot {
namespace internal {

// ============================================================
// LegendRenderer 实现
// ============================================================

std::vector<LegendRenderer::ItemLayout>
LegendRenderer::layout(const std::vector<LegendEntry>& entries,
                       const DevicePlotArea& plotArea,
                       double fontSize) {
    std::vector<ItemLayout> items;
    if (entries.empty()) return items;

    const double swatchW = 16.0;
    const double swatchH = 12.0;
    const double gapX = 6.0;
    const double gapY = 4.0;
    const double marginRight = 10.0;
    const double marginTop = 5.0;

    items.reserve(entries.size());

    double maxTextWidth = 0.0;
    for (auto& e : entries) {
        double tw = estimateTextWidth(e.name.c_str(), fontSize);
        if (tw > maxTextWidth) maxTextWidth = tw;
    }

    double totalWidth = swatchW + gapX + maxTextWidth;
    double startX = plotArea.left + plotArea.width - totalWidth - marginRight;
    double startY = plotArea.top + marginTop;

    for (size_t i = 0; i < entries.size(); i++) {
        ItemLayout item;
        item.swatchX = startX;
        item.swatchY = startY + static_cast<double>(i) * (fontSize * 1.4 + gapY);
        item.swatchW = swatchW;
        item.swatchH = swatchH;
        item.textX = startX + swatchW + gapX;
        item.textY = item.swatchY + swatchH - 2.0;
        item.entry = &entries[i];
        items.push_back(item);
    }

    m_boundingBox.x = startX;
    m_boundingBox.y = startY;
    m_boundingBox.w = totalWidth;
    m_boundingBox.h = entries.size() * (fontSize * 1.4 + gapY) - gapY;

    return items;
}

void LegendRenderer::render(IRenderDevice& device,
                            const std::vector<LegendEntry>& entries,
                            const DevicePlotArea& plotArea,
                            double fontSize) {
    if (entries.empty()) return;

    auto items = layout(entries, plotArea, fontSize);

    FillStyle bgFill;
    bgFill.color = {255, 255, 255, 220};
    device.fillRect(m_boundingBox.x - 4, m_boundingBox.y - 2,
                   m_boundingBox.w + 8, m_boundingBox.h + 4,
                   bgFill);

    FontDesc legendFont;
    legendFont.size = fontSize;
    TextStyle legendStyle;

    for (auto& item : items) {
        FillStyle swatchFill;
        swatchFill.color = item.entry->swatchColor;
        device.fillRect(item.swatchX, item.swatchY,
                       item.swatchW, item.swatchH, swatchFill);

        if (item.entry->lineStyle.width > 0.0) {
            double midY = item.swatchY + item.swatchH / 2.0;
            double lx[] = {item.swatchX + 1, item.swatchX + item.swatchW - 1};
            double ly[] = {midY, midY};
            LineStyle sampleLine = item.entry->lineStyle;
            sampleLine.width = 1.5;
            device.drawPolyline(lx, ly, 2, sampleLine);
        }

        device.drawText(item.textX, item.textY,
                       item.entry->name.c_str(),
                       legendFont, legendStyle);
    }
}

const Rect& LegendRenderer::boundingBox() const {
    return m_boundingBox;
}

double LegendRenderer::estimateTextWidth(const char* text, double fontSize) {
    if (!text) return 0.0;
    size_t len = std::strlen(text);
    return static_cast<double>(len) * fontSize * 0.6;
}

} // namespace internal
} // namespace xyplot
