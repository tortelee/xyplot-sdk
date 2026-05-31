// ============================================================
// blend2d_device.h — Blend2D reference backend
// Owner: Agent E — Backend & Integration
// ============================================================
// Implements IRenderDevice using the Blend2D 2D rendering library.
// This is the "real pixel" backend — produces actual visible output.
//
// Blend2D is an optional dependency. When BLEND2D_AVAILABLE is
// not defined, this header provides a stub that compiles but
// produces no output (useful for projects without Blend2D).
//
// Build requirements (when enabled):
//   - Blend2D library installed
//   - CMake: find_package(blend2d) or FetchContent
//   - #define BLEND2D_AVAILABLE before including this header
//
// Usage:
//   #define BLEND2D_AVAILABLE
//   #include "backends/blend2d/blend2d_device.h"
//   BLImage img(800, 600, BL_FORMAT_PRGB32);
//   Blend2DDevice device(img);
//   plot.render(device);
// ============================================================
#pragma once
#include "xyplot/irender_device.h"
#include <cstring>
#include <cmath>
#include <cstdio>
#include <vector>
#include <algorithm>

// ==================================================================
// Blend2D conditional compilation
// ==================================================================
#ifdef BLEND2D_AVAILABLE
  #include <blend2d.h>
#endif

namespace xyplot {

#ifdef BLEND2D_AVAILABLE

// ==================================================================
// Full Blend2D implementation
// ==================================================================
class Blend2DDevice : public IRenderDevice {
public:
    /// Construct with an existing BLImage (the render target).
    /// The image must remain valid for the lifetime of the device.
    explicit Blend2DDevice(BLImage& image)
        : m_image(&image), m_context(image) {}

    /// Construct with own internal image.
    Blend2DDevice(int width, int height)
        : m_ownImage(width, height, BL_FORMAT_PRGB32),
          m_image(&m_ownImage),
          m_context(m_ownImage) {}

    ~Blend2DDevice() override = default;

    // ── Frame lifecycle ──
    void beginFrame() override {
        m_context.clearAll();
    }

    void endFrame() override {
        m_context.flush(BL_CONTEXT_FLUSH_SYNC);
    }

    // ── Clipping ──
    void setClipRect(double x, double y, double w, double h) override {
        m_clipBox = BLBox(x, y, x + w, y + h);
        m_context.save();
        m_context.clipToRect(m_clipBox);
        m_clipping = true;
    }

    void resetClip() override {
        if (m_clipping) {
            m_context.restore();
            m_clipping = false;
        }
    }

    // ── Polyline ──
    void drawPolyline(const double* xs, const double* ys,
                      int count, const LineStyle& style) override {
        if (count < 2) return;

        // Build path
        BLPath path;
        path.moveTo(xs[0], ys[0]);
        for (int i = 1; i < count; i++)
            path.lineTo(xs[i], ys[i]);

        // Stroke style
        BLRgba32 color(style.color.r, style.color.g,
                       style.color.b, style.color.a);

        BLArray<double> dashArray;
        if (style.dash == LineStyle::DashLine) {
            double pattern[] = { 6.0, 4.0 };
            dashArray = BLArray<double>(pattern, 2);
        } else if (style.dash == LineStyle::DotLine) {
            double pattern[] = { 2.0, 4.0 };
            dashArray = BLArray<double>(pattern, 2);
        } else if (style.dash == LineStyle::DashDotLine) {
            double pattern[] = { 6.0, 4.0, 2.0, 4.0 };
            dashArray = BLArray<double>(pattern, 4);
        }

        m_context.setStrokeStyle(color);
        m_context.setStrokeWidth(style.width);
        if (!dashArray.empty())
            m_context.setStrokeDashArray(dashArray);

        m_context.strokePath(path);

        // Reset dash
        if (!dashArray.empty())
            m_context.setStrokeDashArray(BLArray<double>());
    }

    // ── Markers ──
    void drawMarkers(const double* xs, const double* ys,
                     int count, const MarkerStyle& style) override {
        double half = style.size / 2.0;
        BLRgba32 fillColor(style.fillColor.r, style.fillColor.g,
                           style.fillColor.b, style.fillColor.a);
        BLRgba32 edgeColor(style.edgeColor.r, style.edgeColor.g,
                           style.edgeColor.b, style.edgeColor.a);

        m_context.setFillStyle(fillColor);
        m_context.setStrokeStyle(edgeColor);
        m_context.setStrokeWidth(style.edgeWidth);

        for (int i = 0; i < count; i++) {
            double cx = xs[i], cy = ys[i];
            BLPath path;

            switch (style.shape) {
            case MarkerStyle::Circle: {
                BLCircle circle(cx, cy, half);
                path.addCircle(circle);
                break;
            }
            case MarkerStyle::Square: {
                BLBox box(cx - half, cy - half, cx + half, cy + half);
                path.addRect(box);
                break;
            }
            case MarkerStyle::Diamond: {
                path.moveTo(cx, cy - half);         // top
                path.lineTo(cx + half, cy);         // right
                path.lineTo(cx, cy + half);         // bottom
                path.lineTo(cx - half, cy);         // left
                path.close();
                break;
            }
            case MarkerStyle::Triangle: {
                path.moveTo(cx, cy - half);         // top
                path.lineTo(cx + half, cy + half);  // bottom-right
                path.lineTo(cx - half, cy + half);  // bottom-left
                path.close();
                break;
            }
            case MarkerStyle::Cross: {
                path.moveTo(cx - half, cy);
                path.lineTo(cx + half, cy);
                path.moveTo(cx, cy - half);
                path.lineTo(cx, cy + half);
                break;
            }
            case MarkerStyle::Plus: {
                const double d = half * 0.7071; // cos(45°)
                path.moveTo(cx - d, cy - d);
                path.lineTo(cx + d, cy + d);
                path.moveTo(cx + d, cy - d);
                path.lineTo(cx - d, cy + d);
                break;
            }
            }

            m_context.fillPath(path);
            if (style.edgeWidth > 0)
                m_context.strokePath(path);
        }
    }

    // ── Text ──
    void drawText(double x, double y, const char* text,
                  const FontDesc& font, const TextStyle& style) override {
        BLRgba32 color(style.color.r, style.color.g,
                       style.color.b, style.color.a);

        BLFont blFont;
        blFont.createFromFace(m_fontFace, static_cast<float>(font.size));

        // Measure text for alignment
        BLTextMetrics metrics;
        BLGlyphBuffer glyphs;
        m_context.setFillStyle(color);

        double drawX = x;
        double drawY = y;

        if (style.hAlign == TextStyle::Center ||
            style.hAlign == TextStyle::Right ||
            style.vAlign != TextStyle::Bottom) {
            // Use default text extent for alignment (Blend2D text metrics
            // require glyph layout which is expensive for a simple align)
            double tw = 0, th = 0;
            IRenderDevice::textExtent(text, font, &tw, &th);

            if (style.hAlign == TextStyle::Center) drawX -= tw / 2.0;
            else if (style.hAlign == TextStyle::Right) drawX -= tw;

            if (style.vAlign == TextStyle::Middle) drawY += th / 2.0;
            else if (style.vAlign == TextStyle::Top) drawY += th;
        }

        m_context.fillUtf8Text(BLPoint(drawX, drawY), blFont,
                               text, std::strlen(text));
    }

    // ── Fill rect ──
    void fillRect(double x, double y, double w, double h,
                  const FillStyle& style) override {
        BLRgba32 color(style.color.r, style.color.g,
                       style.color.b, style.color.a);
        m_context.setFillStyle(color);
        m_context.fillRect(BLRect(x, y, w, h));
    }

    // ── Accessors ──
    BLImage& image() { return *m_image; }
    const BLImage& image() const { return *m_image; }
    BLContext& context() { return m_context; }

private:
    BLImage m_ownImage;          // Owned image (when constructed with w×h)
    BLImage* m_image;            // Active image (owned or external)
    BLContext m_context;          // Blend2D rendering context
    BLFontFace m_fontFace;       // Default font face
    BLBox m_clipBox;
    bool m_clipping = false;
};

#else // !BLEND2D_AVAILABLE

// ==================================================================
// Blend2D stub (compiles without Blend2D, produces no output)
// ==================================================================
class Blend2DDevice : public IRenderDevice {
public:
    Blend2DDevice(int /*width*/, int /*height*/) {}
    explicit Blend2DDevice(void* /*externalImage*/) {}

    void beginFrame() override {}
    void endFrame() override {}
    void setClipRect(double, double, double, double) override { m_clipping = true; }
    void resetClip() override { m_clipping = false; }

    void drawPolyline(const double*, const double*, int,
                      const LineStyle&) override {}
    void drawMarkers(const double*, const double*, int,
                     const MarkerStyle&) override {}
    void drawText(double, double, const char*,
                  const FontDesc&, const TextStyle&) override {}
    void fillRect(double, double, double, double,
                  const FillStyle&) override {}

    bool isClipping() const { return m_clipping; }

private:
    bool m_clipping = false;
};

#endif // BLEND2D_AVAILABLE

} // namespace xyplot
