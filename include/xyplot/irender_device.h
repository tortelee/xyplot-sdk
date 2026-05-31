// ============================================================
// xyplot/irender_device.h — 渲染设备抽象接口 (冻结 v1.0)
// ============================================================
// P0 必需: 8 个纯虚方法
// P0 可降级: 1 个虚方法 (textExtent, 有默认实现)
// 延期至 P1+: fillPolygon, drawImage 等
//
// 变更规则: 冻结期内只增不删。新增方法必须为带默认实现的虚方法。
// ============================================================
#pragma once
#include "types.h"
#include <cstring>

namespace xyplot {

class IRenderDevice {
public:
    virtual ~IRenderDevice() = default;

    // ──── 帧生命周期 (P0 必需 1/8, 2/8) ────
    virtual void beginFrame() = 0;
    virtual void endFrame() = 0;

    // ──── 裁剪 (P0 必需 3/8, 4/8) ────
    virtual void setClipRect(double x, double y, double w, double h) = 0;
    virtual void resetClip() = 0;

    // ──── 折线 (P0 必需 5/8) ────
    virtual void drawPolyline(const double* xs, const double* ys,
                              int count, const LineStyle& style) = 0;

    // ──── 标记点 (P0 必需 6/8) ────
    virtual void drawMarkers(const double* xs, const double* ys,
                             int count, const MarkerStyle& style) = 0;

    // ──── 文本 (P0 必需 7/8) ────
    virtual void drawText(double x, double y, const char* text,
                          const FontDesc& font, const TextStyle& style) = 0;

    // ──── 填充矩形 (P0 必需 8/8) ────
    virtual void fillRect(double x, double y, double w, double h,
                          const FillStyle& style) = 0;

    // ──── 文本度量 (P0 可降级 — 有默认估算实现) ────
    virtual void textExtent(const char* text, const FontDesc& font,
                            double* w, double* h) {
        double approxCharWidth = font.size * 0.6;
        if (w) *w = static_cast<double>(std::strlen(text ? text : "")) * approxCharWidth;
        if (h) *h = font.size * 1.25;
    }

    // ──── P1 扩展 (B2 — 带默认空实现) ────

    // 填充任意多边形（Area Plot 使用）
    virtual void fillPolygon(const double* xs, const double* ys,
                             int count, const FillStyle& style) {
        (void)xs; (void)ys; (void)count; (void)style;
    }

    // 渲染像素图像（Heatmap 使用）
    // rgba: RGBA 格式，imgW×imgH 像素 → 映射到设备空间 (x,y,w,h)
    virtual void drawImage(double x, double y, double w, double h,
                           const uint8_t* rgba, int imgW, int imgH) {
        (void)x; (void)y; (void)w; (void)h;
        (void)rgba; (void)imgW; (void)imgH;
    }
};

} // namespace xyplot
