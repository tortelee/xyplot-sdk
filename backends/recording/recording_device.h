// ============================================================
// recording_device.h — Recording render device
// Owner: Agent E — Backend & Integration
// ============================================================
// Implements IRenderDevice by recording all draw calls into
// in-memory structures. Used for:
//   1. Unit tests — verify expected draw call sequences
//   2. Debugging — inspect what the render pipeline produces
//   3. Minimal examples — "pseudo-device" that doesn't need a real GPU
//
// Usage:
//   RecordingDevice rec;
//   plot.render(rec);
//   // Inspect rec.calls() or rec.callCount(drawPolyline)
// ============================================================
#pragma once
#include "xyplot/irender_device.h"
#include <vector>
#include <string>
#include <cstring>
#include <cstdio>

namespace xyplot {

class RecordingDevice : public IRenderDevice {
public:
    // ── Call type enumeration ──
    enum CallType {
        BeginFrame = 0,
        EndFrame,
        SetClipRect,
        ResetClip,
        DrawPolyline,
        DrawMarkers,
        DrawText,
        FillRect,
        TextExtent,
        FillPolygon,   // B2
        DrawImage      // B2
    };

    static const char* callTypeName(CallType t) {
        switch (t) {
        case BeginFrame:    return "beginFrame";
        case EndFrame:      return "endFrame";
        case SetClipRect:   return "setClipRect";
        case ResetClip:     return "resetClip";
        case DrawPolyline:  return "drawPolyline";
        case DrawMarkers:   return "drawMarkers";
        case DrawText:      return "drawText";
        case FillRect:      return "fillRect";
        case TextExtent:    return "textExtent";
        case FillPolygon:   return "fillPolygon";
        case DrawImage:     return "drawImage";
        default:            return "unknown";
        }
    }

    // ── Recorded call entry ──
    struct RecordedCall {
        CallType type = BeginFrame;
        int callIndex = 0;

        // Parameters (union-like; only relevant fields are set per type)
        double px = 0, py = 0, pw = 0, ph = 0;  // setClipRect / fillRect / drawText
        int polylineCount = 0;                     // drawPolyline / drawMarkers point count
        double polylineFirstX = 0, polylineFirstY = 0;
        double polylineLastX = 0, polylineLastY = 0;
        LineStyle::DashStyle lineDash = LineStyle::SolidLine;
        double lineWidth = 0;
        std::string text;                           // drawText
        double fontSize = 0;
        bool fontBold = false;
        MarkerStyle::Shape markerShape = MarkerStyle::Circle;
        double markerSize = 0;
        Color fillColor;
    };

    // ── IRenderDevice implementation ──

    void beginFrame() override {
        RecordedCall c;
        c.type = BeginFrame;
        c.callIndex = nextIndex();
        m_calls.push_back(c);
    }

    void endFrame() override {
        RecordedCall c;
        c.type = EndFrame;
        c.callIndex = nextIndex();
        m_calls.push_back(c);
    }

    void setClipRect(double x, double y, double w, double h) override {
        RecordedCall c;
        c.type = SetClipRect;
        c.callIndex = nextIndex();
        c.px = x; c.py = y; c.pw = w; c.ph = h;
        m_calls.push_back(c);
        m_clipX = x; m_clipY = y; m_clipW = w; m_clipH = h;
        m_clipping = true;
    }

    void resetClip() override {
        RecordedCall c;
        c.type = ResetClip;
        c.callIndex = nextIndex();
        m_calls.push_back(c);
        m_clipping = false;
    }

    void drawPolyline(const double* xs, const double* ys,
                      int count, const LineStyle& style) override {
        RecordedCall c;
        c.type = DrawPolyline;
        c.callIndex = nextIndex();
        c.polylineCount = count;
        c.lineWidth = style.width;
        c.lineDash = style.dash;
        if (count > 0) {
            c.polylineFirstX = xs[0];
            c.polylineFirstY = ys[0];
            c.polylineLastX = xs[count - 1];
            c.polylineLastY = ys[count - 1];
        }
        m_calls.push_back(c);
    }

    void drawMarkers(const double* xs, const double* ys,
                     int count, const MarkerStyle& style) override {
        RecordedCall c;
        c.type = DrawMarkers;
        c.callIndex = nextIndex();
        c.polylineCount = count;
        c.markerShape = style.shape;
        c.markerSize = style.size;
        if (count > 0) {
            c.polylineFirstX = xs[0];
            c.polylineFirstY = ys[0];
            c.polylineLastX = xs[count - 1];
            c.polylineLastY = ys[count - 1];
        }
        m_calls.push_back(c);
    }

    void drawText(double x, double y, const char* text,
                  const FontDesc& font, const TextStyle& /*style*/) override {
        RecordedCall c;
        c.type = DrawText;
        c.callIndex = nextIndex();
        c.px = x; c.py = y;
        c.text = text ? text : "";
        c.fontSize = font.size;
        c.fontBold = font.bold;
        m_calls.push_back(c);
    }

    void fillRect(double x, double y, double w, double h,
                  const FillStyle& style) override {
        RecordedCall c;
        c.type = FillRect;
        c.callIndex = nextIndex();
        c.px = x; c.py = y; c.pw = w; c.ph = h;
        c.fillColor = style.color;
        m_calls.push_back(c);
    }

    // ── B2: fillPolygon ──
    void fillPolygon(const double* xs, const double* ys,
                     int count, const FillStyle& style) override {
        RecordedCall c;
        c.type = FillPolygon;
        c.callIndex = nextIndex();
        c.polylineCount = count;
        c.fillColor = style.color;
        if (count > 0) {
            c.polylineFirstX = xs[0];
            c.polylineFirstY = ys[0];
            c.polylineLastX = xs[count - 1];
            c.polylineLastY = ys[count - 1];
        }
        m_calls.push_back(c);
    }

    // ── B2: drawImage ──
    void drawImage(double x, double y, double w, double h,
                   const uint8_t* rgba, int imgW, int imgH) override {
        RecordedCall c;
        c.type = DrawImage;
        c.callIndex = nextIndex();
        c.px = x; c.py = y; c.pw = w; c.ph = h;
        c.polylineCount = imgW;      // reuse: imgW
        c.markerSize = static_cast<double>(imgH);  // reuse: imgH
        // Store first RGBA pixel as a sample
        if (rgba && imgW > 0 && imgH > 0) {
            c.fillColor = { rgba[0], rgba[1], rgba[2], rgba[3] };
        }
        m_calls.push_back(c);
    }

    void textExtent(const char* text, const FontDesc& font,
                    double* w, double* h) override {
        RecordedCall c;
        c.type = TextExtent;
        c.callIndex = nextIndex();
        c.text = text ? text : "";
        c.fontSize = font.size;
        c.fontBold = font.bold;
        m_calls.push_back(c);
        // Delegate to base class default implementation
        IRenderDevice::textExtent(text, font, w, h);
        // Also store the computed extent
        if (w) c.pw = *w;
        if (h) c.ph = *h;
        // Update the recorded call
        m_calls.back() = c;
    }

    // ── Inspection API ──

    /// Return all recorded calls.
    const std::vector<RecordedCall>& calls() const { return m_calls; }

    /// Return number of recorded calls of a specific type.
    int callCount(CallType type) const {
        int count = 0;
        for (auto& c : m_calls)
            if (c.type == type) count++;
        return count;
    }

    /// Total number of recorded calls.
    int totalCalls() const { return static_cast<int>(m_calls.size()); }

    /// Reset all recorded calls (for reuse).
    void reset() {
        m_calls.clear();
        m_callCounter = 0;
        m_clipping = false;
    }

    /// Dump all recorded calls to a human-readable string.
    std::string dump() const {
        std::string out;
        char buf[256];
        for (auto& c : m_calls) {
            switch (c.type) {
            case BeginFrame:
                out += "[beginFrame]\n";
                break;
            case EndFrame:
                out += "[endFrame]\n";
                break;
            case SetClipRect:
                std::snprintf(buf, sizeof(buf),
                    "[setClipRect] x=%.1f y=%.1f w=%.1f h=%.1f\n",
                    c.px, c.py, c.pw, c.ph);
                out += buf;
                break;
            case ResetClip:
                out += "[resetClip]\n";
                break;
            case DrawPolyline:
                std::snprintf(buf, sizeof(buf),
                    "[drawPolyline] count=%d first=(%.1f,%.1f) last=(%.1f,%.1f) width=%.1f\n",
                    c.polylineCount, c.polylineFirstX, c.polylineFirstY,
                    c.polylineLastX, c.polylineLastY, c.lineWidth);
                out += buf;
                break;
            case DrawMarkers:
                std::snprintf(buf, sizeof(buf),
                    "[drawMarkers] count=%d shape=%d size=%.1f\n",
                    c.polylineCount, (int)c.markerShape, c.markerSize);
                out += buf;
                break;
            case DrawText:
                std::snprintf(buf, sizeof(buf),
                    "[drawText] x=%.1f y=%.1f text=\"%s\" fontSize=%.1f\n",
                    c.px, c.py, c.text.c_str(), c.fontSize);
                out += buf;
                break;
            case FillRect:
                std::snprintf(buf, sizeof(buf),
                    "[fillRect] x=%.1f y=%.1f w=%.1f h=%.1f color=(%d,%d,%d)\n",
                    c.px, c.py, c.pw, c.ph,
                    (int)c.fillColor.r, (int)c.fillColor.g, (int)c.fillColor.b);
                out += buf;
                break;
            case TextExtent:
                std::snprintf(buf, sizeof(buf),
                    "[textExtent] text=\"%s\" fontSize=%.1f → w=%.1f h=%.1f\n",
                    c.text.c_str(), c.fontSize, c.pw, c.ph);
                out += buf;
                break;
            case FillPolygon:
                std::snprintf(buf, sizeof(buf),
                    "[fillPolygon] count=%d first=(%.1f,%.1f) color=(%d,%d,%d)\n",
                    c.polylineCount, c.polylineFirstX, c.polylineFirstY,
                    (int)c.fillColor.r, (int)c.fillColor.g, (int)c.fillColor.b);
                out += buf;
                break;
            case DrawImage:
                std::snprintf(buf, sizeof(buf),
                    "[drawImage] x=%.1f y=%.1f w=%.1f h=%.1f img=%dx%d firstPixel=(%d,%d,%d,%d)\n",
                    c.px, c.py, c.pw, c.ph,
                    c.polylineCount, (int)c.markerSize,
                    (int)c.fillColor.r, (int)c.fillColor.g,
                    (int)c.fillColor.b, (int)c.fillColor.a);
                out += buf;
                break;
            }
        }
        return out;
    }

    /// Verify that beginFrame was called before endFrame.
    bool hasValidFrameSequence() const {
        int beginCount = callCount(BeginFrame);
        int endCount = callCount(EndFrame);
        return beginCount > 0 && beginCount == endCount;
    }

    /// Verify that clip operations are balanced.
    bool hasBalancedClip() const {
        int setCount = callCount(SetClipRect);
        int resetCount = callCount(ResetClip);
        return setCount == resetCount;
    }

    /// Check current clipping state.
    bool isClipping() const { return m_clipping; }
    double clipX() const { return m_clipX; }
    double clipY() const { return m_clipY; }
    double clipW() const { return m_clipW; }
    double clipH() const { return m_clipH; }

private:
    std::vector<RecordedCall> m_calls;
    int m_callCounter = 0;
    double m_clipX = 0, m_clipY = 0, m_clipW = 0, m_clipH = 0;
    bool m_clipping = false;

    int nextIndex() { return m_callCounter++; }
};

} // namespace xyplot
