// ============================================================
// examples/blend2d_demo/blend2d_render_device.h — Blend2D 渲染设备实现
// ============================================================
// Owner: Agent F (文档 & 示例)
//
// 此头文件提供基于 Blend2D 的 IRenderDevice 完整实现。
// Blend2D 是一个高性能 2D 矢量图形库，提供 GPU 级性能的 CPU 渲染。
//
// 用法:
//   1. 安装 Blend2D (https://blend2d.com/)
//   2. 创建 Blend2DRenderDevice
//   3. 调用 Plot::render(blDevice)
//   4. 将结果保存为 PNG: blDevice.image().writeToFile("output.png")
//
// 依赖: Blend2D (https://github.com/blend2d/blend2d)
//
// 注意: Blend2D 为可选依赖。若未安装，此示例不会编译。
//       对于无外部依赖的方案，请参考 ../minimal/ 中的示例。
// ============================================================

#pragma once

#include <xyplot/xyplot.h>

// Blend2D 为可选依赖。若未安装，此文件将无法编译。
// 包含此头文件前请确保 Blend2D 已安装并在 CMake 中检测到。
#if __has_include(<blend2d.h>)
#include <blend2d.h>

#include <vector>
#include <cstring>
#include <cmath>
#include <cstdio>

// ═══════════════════════════════════════════════════════════
// Blend2DRenderDevice — 基于 Blend2D 的渲染设备
// ═══════════════════════════════════════════════════════════
//
// 渲染到 Blend2D 的 BLImage 上。典型用法:
//
//   BLImage img(800, 600, BL_FORMAT_PRGB32);
//   Blend2DRenderDevice device(img);
//   plot.render(device);
//   img.writeToFile("chart.png");
//
class Blend2DRenderDevice : public xyplot::IRenderDevice {
public:
    /// 构造并绑定到 BLImage。
    /// BLImage 必须在设备生命周期内保持有效。
    explicit Blend2DRenderDevice(BLImage& image)
        : m_image(image)
        , m_context(image)
    {
        m_context.setCompOp(BL_COMP_OP_SRC_OVER);
    }

    // ── 帧生命周期 ──
    void beginFrame() override {
        m_context.clearAll();
    }

    void endFrame() override {
        m_context.end();
    }

    // ── 裁剪 ──
    void setClipRect(double x, double y, double w, double h) override {
        m_context.save();
        m_context.clipToRect(BLRect(x, y, w, h));
    }

    void resetClip() override {
        m_context.restore();
    }

    // ── 折线绘制 ──
    void drawPolyline(const double* xs, const double* ys,
                      int count, const xyplot::LineStyle& style) override {
        if (count < 1) return;

        // 构建 BLPath
        BLPath path;
        path.moveTo(xs[0], ys[0]);
        for (int i = 1; i < count; ++i) {
            path.lineTo(xs[i], ys[i]);
        }

        // 设置样式
        m_context.setStrokeStyle(makeStroke(style));
        m_context.setFillStyle(BLRgba32(0, 0, 0, 0));  // 透明填充
        m_context.setStrokeWidth(style.width);

        // 虚线样式
        if (style.dash != xyplot::LineStyle::SolidLine) {
            setDashPattern(style.dash, style.width);
        }

        m_context.strokePath(path);

        // 重置虚线
        if (style.dash != xyplot::LineStyle::SolidLine) {
            double dashArray[1] = {0};
            m_context.setStrokeDashArray(dashArray, 0);
        }
    }

    // ── 标记点绘制 ──
    void drawMarkers(const double* xs, const double* ys,
                     int count, const xyplot::MarkerStyle& style) override {
        if (count < 1) return;

        double halfSize = style.size * 0.5;
        BLRgba32 fillColor(style.fillColor.r, style.fillColor.g,
                           style.fillColor.b, style.fillColor.a);
        BLRgba32 edgeColor(style.edgeColor.r, style.edgeColor.g,
                           style.edgeColor.b, style.edgeColor.a);

        m_context.setStrokeWidth(style.edgeWidth);
        m_context.setStrokeStyle(edgeColor);

        for (int i = 0; i < count; ++i) {
            double cx = xs[i];
            double cy = ys[i];

            switch (style.shape) {
            case xyplot::MarkerStyle::Circle: {
                BLCircle circle(cx, cy, halfSize);
                m_context.setFillStyle(fillColor);
                m_context.fillGeometry(BL_GEOMETRY_TYPE_CIRCLE, &circle);
                m_context.setFillStyle(BLRgba32(0,0,0,0));
                m_context.strokeGeometry(BL_GEOMETRY_TYPE_CIRCLE, &circle);
                break;
            }
            case xyplot::MarkerStyle::Square: {
                BLRect rect(cx - halfSize, cy - halfSize,
                            style.size, style.size);
                m_context.setFillStyle(fillColor);
                m_context.fillRect(rect);
                m_context.strokeRect(rect);
                break;
            }
            case xyplot::MarkerStyle::Diamond: {
                BLPath diamond;
                diamond.moveTo(cx, cy - halfSize);
                diamond.lineTo(cx + halfSize, cy);
                diamond.lineTo(cx, cy + halfSize);
                diamond.lineTo(cx - halfSize, cy);
                diamond.close();
                m_context.setFillStyle(fillColor);
                m_context.fillPath(diamond);
                m_context.setFillStyle(BLRgba32(0,0,0,0));
                m_context.strokePath(diamond);
                break;
            }
            case xyplot::MarkerStyle::Triangle: {
                BLPath tri;
                tri.moveTo(cx, cy - halfSize);
                tri.lineTo(cx + halfSize, cy + halfSize);
                tri.lineTo(cx - halfSize, cy + halfSize);
                tri.close();
                m_context.setFillStyle(fillColor);
                m_context.fillPath(tri);
                m_context.setFillStyle(BLRgba32(0,0,0,0));
                m_context.strokePath(tri);
                break;
            }
            case xyplot::MarkerStyle::Cross: {
                BLPath cross;
                cross.moveTo(cx - halfSize, cy - halfSize);
                cross.lineTo(cx + halfSize, cy + halfSize);
                cross.moveTo(cx + halfSize, cy - halfSize);
                cross.lineTo(cx - halfSize, cy + halfSize);
                m_context.strokePath(cross);
                break;
            }
            case xyplot::MarkerStyle::Plus: {
                BLPath plus;
                plus.moveTo(cx, cy - halfSize);
                plus.lineTo(cx, cy + halfSize);
                plus.moveTo(cx - halfSize, cy);
                plus.lineTo(cx + halfSize, cy);
                m_context.strokePath(plus);
                break;
            }
            }
        }
    }

    // ── 文本绘制 ──
    void drawText(double x, double y, const char* text,
                  const xyplot::FontDesc& font,
                  const xyplot::TextStyle& style) override {
        if (!text || !*text) return;

        BLFont blFont;
        blFont.createFromFace(nullptr, font.size);  // 使用默认字体

        BLTextMetrics tm;
        BLGlyphBuffer glyphBuf;

        // 简化的字形排版
        BLRgba32 textColor(style.color.r, style.color.g,
                           style.color.b, style.color.a);

        double drawX = x;
        double drawY = y;

        // 对齐调整
        double textW = static_cast<double>(std::strlen(text)) * font.size * 0.6;
        if (style.hAlign == xyplot::TextStyle::Center) {
            drawX -= textW * 0.5;
        } else if (style.hAlign == xyplot::TextStyle::Right) {
            drawX -= textW;
        }
        // vAlign: 默认 Bottom = y is baseline

        m_context.setFillStyle(textColor);
        m_context.fillUtf8Text(BLPoint(drawX, drawY), blFont,
                               text, std::strlen(text));
    }

    // ── 填充矩形 ──
    void fillRect(double x, double y, double w, double h,
                  const xyplot::FillStyle& style) override {
        BLRgba32 color(style.color.r, style.color.g,
                       style.color.b, style.color.a);
        m_context.setFillStyle(color);
        m_context.fillRect(BLRect(x, y, w, h));
    }

    // ── 文本度量 ──
    void textExtent(const char* text, const xyplot::FontDesc& font,
                    double* w, double* h) override {
        if (!text || !*text) {
            if (w) *w = 0;
            if (h) *h = 0;
            return;
        }

        BLFont blFont;
        blFont.createFromFace(nullptr, font.size);
        BLTextMetrics tm = blFont.metrics();

        // 估算文本尺寸
        if (w) *w = static_cast<double>(std::strlen(text)) * font.size * 0.58;
        if (h) *h = tm.fullHeight();
    }

private:
    BLImage& m_image;
    BLContext m_context;

    /// 将 XYPlot LineStyle 转换为 Blend2D stroke
    BLRgba32 makeStroke(const xyplot::LineStyle& style) {
        return BLRgba32(style.color.r, style.color.g,
                        style.color.b, style.color.a);
    }

    /// 设置虚线模式
    void setDashPattern(xyplot::LineStyle::DashStyle dash, double width) {
        double dashPattern[4];
        int count = 0;

        switch (dash) {
        case xyplot::LineStyle::DashLine:
            dashPattern[0] = width * 4.0;
            dashPattern[1] = width * 2.0;
            count = 2;
            break;
        case xyplot::LineStyle::DotLine:
            dashPattern[0] = width;
            dashPattern[1] = width * 2.0;
            count = 2;
            break;
        case xyplot::LineStyle::DashDotLine:
            dashPattern[0] = width * 4.0;
            dashPattern[1] = width * 2.0;
            dashPattern[2] = width;
            dashPattern[3] = width * 2.0;
            count = 4;
            break;
        default:
            return;
        }

        m_context.setStrokeDashArray(dashPattern, count);
    }
};

#endif // __has_include(<blend2d.h>)
