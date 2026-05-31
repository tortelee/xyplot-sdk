// ============================================================
// svg_device.h — SVG Render Device (zero-dependency)
// ============================================================
// Owner: Project Lead
// Usage:
//   #include "backends/svg/svg_device.h"
//   SvgDevice svg(800, 600, "output.svg");
//   plot.render(svg);
//   svg.finish();  // writes output.svg
//
// Pure C++17, zero dependencies. Open output.svg in any browser.
// ============================================================
#pragma once
#include "xyplot/irender_device.h"
#include "xyplot/types.h"
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <cmath>
#include <cstring>
#include <cstdio>

namespace xyplot {

class SvgDevice : public IRenderDevice {
    int m_width, m_height;
    std::string m_filename;
    std::ostringstream m_body;
    std::string m_currentColor;
    double m_currentWidth = 1.0;

    std::string colorStr(const Color& c) {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "rgb(%d,%d,%d)", c.r, c.g, c.b);
        return buf;
    }

    std::string dashArray(LineStyle::DashStyle d) {
        switch (d) {
            case LineStyle::DashLine: return " stroke-dasharray=\"8,4\"";
            case LineStyle::DotLine: return " stroke-dasharray=\"2,4\"";
            case LineStyle::DashDotLine: return " stroke-dasharray=\"8,4,2,4\"";
            default: return "";
        }
    }

    std::string markerPath(MarkerStyle::Shape shape, double cx, double cy, double half) {
        std::ostringstream s;
        switch (shape) {
            case MarkerStyle::Circle:
                s << "<circle cx=\"" << cx << "\" cy=\"" << cy << "\" r=\"" << half << "\"";
                break;
            case MarkerStyle::Square:
                s << "<rect x=\"" << (cx - half) << "\" y=\"" << (cy - half)
                  << "\" width=\"" << (half * 2) << "\" height=\"" << (half * 2) << "\"";
                break;
            case MarkerStyle::Diamond:
                s << "<polygon points=\""
                  << cx << "," << (cy - half) << " "
                  << (cx + half) << "," << cy << " "
                  << cx << "," << (cy + half) << " "
                  << (cx - half) << "," << cy << "\"";
                break;
            case MarkerStyle::Triangle:
                s << "<polygon points=\""
                  << cx << "," << (cy - half) << " "
                  << (cx + half) << "," << (cy + half) << " "
                  << (cx - half) << "," << (cy + half) << "\"";
                break;
            case MarkerStyle::Cross:
                s << "<line x1=\"" << (cx - half) << "\" y1=\"" << cy
                  << "\" x2=\"" << (cx + half) << "\" y2=\"" << cy << "\"";
                m_body << s.str() << " stroke=\"" << m_currentColor
                       << "\" stroke-width=\"" << m_currentWidth << "\"/>";
                s.str(""); s.clear();
                s << "<line x1=\"" << cx << "\" y1=\"" << (cy - half)
                  << "\" x2=\"" << cx << "\" y2=\"" << (cy + half) << "\"";
                break;
            case MarkerStyle::Plus:
                half *= 0.7;
                s << "<line x1=\"" << (cx - half) << "\" y1=\"" << (cy - half)
                  << "\" x2=\"" << (cx + half) << "\" y2=\"" << (cy + half) << "\"";
                m_body << s.str() << " stroke=\"" << m_currentColor
                       << "\" stroke-width=\"" << m_currentWidth << "\"/>";
                s.str(""); s.clear();
                s << "<line x1=\"" << (cx + half) << "\" y1=\"" << (cy - half)
                  << "\" x2=\"" << (cx - half) << "\" y2=\"" << (cy + half) << "\"";
                break;
        }
        return s.str();
    }

    std::string escapeXml(const std::string& s) {
        std::string out;
        for (char c : s) {
            switch (c) {
                case '<': out += "&lt;"; break;
                case '>': out += "&gt;"; break;
                case '&': out += "&amp;"; break;
                case '"': out += "&quot;"; break;
                default: out += c;
            }
        }
        return out;
    }

public:
    SvgDevice(int width, int height, const std::string& filename)
        : m_width(width), m_height(height), m_filename(filename) {}

    void beginFrame() override {}
    void endFrame() override {}

    void setClipRect(double x, double y, double w, double h) override {
        m_body << "<clipPath id=\"clip\">"
               << "<rect x=\"" << x << "\" y=\"" << y
               << "\" width=\"" << w << "\" height=\"" << h << "\"/>"
               << "</clipPath>\n";
        m_body << "<g clip-path=\"url(#clip)\">\n";
    }

    void resetClip() override {
        m_body << "</g>\n";
    }

    void drawPolyline(const double* xs, const double* ys,
                      int count, const LineStyle& style) override {
        if (count < 2) return;
        m_currentColor = colorStr(style.color);
        m_currentWidth = style.width;
        m_body << "<polyline points=\"";
        for (int i = 0; i < count; i++) {
            m_body << xs[i] << "," << ys[i] << " ";
        }
        m_body << "\" fill=\"none\" stroke=\"" << m_currentColor
               << "\" stroke-width=\"" << style.width << "\""
               << dashArray(style.dash) << " stroke-linecap=\"round\""
               << " stroke-linejoin=\"round\"/>\n";
    }

    void drawMarkers(const double* xs, const double* ys,
                     int count, const MarkerStyle& style) override {
        m_currentColor = colorStr(style.fillColor);
        m_currentWidth = style.edgeWidth;
        double half = style.size / 2.0;
        for (int i = 0; i < count; i++) {
            std::string path = markerPath(style.shape, xs[i], ys[i], half);
            if (style.shape == MarkerStyle::Cross || style.shape == MarkerStyle::Plus) {
                continue; // already written in markerPath
            }
            m_body << path
                   << " fill=\"" << colorStr(style.fillColor)
                   << "\" stroke=\"" << colorStr(style.edgeColor)
                   << "\" stroke-width=\"" << style.edgeWidth << "\"/>\n";
        }
    }

    void drawText(double x, double y, const char* text,
                  const FontDesc& font, const TextStyle& style) override {
        std::string anchor;
        switch (style.hAlign) {
            case TextStyle::Center: anchor = "text-anchor=\"middle\""; break;
            case TextStyle::Right:  anchor = "text-anchor=\"end\""; break;
            default: anchor = "text-anchor=\"start\""; break;
        }
        std::string weight = font.bold ? " font-weight=\"bold\"" : "";
        std::string fstyle = font.italic ? " font-style=\"italic\"" : "";
        m_body << "<text x=\"" << x << "\" y=\"" << y << "\""
               << " fill=\"" << colorStr(style.color) << "\""
               << " font-size=\"" << font.size << "px\""
               << " font-family=\"Arial, sans-serif\""
               << weight << fstyle << " " << anchor << ">"
               << escapeXml(text ? text : "") << "</text>\n";
    }

    void fillRect(double x, double y, double w, double h,
                  const FillStyle& style) override {
        m_body << "<rect x=\"" << x << "\" y=\"" << y
               << "\" width=\"" << w << "\" height=\"" << h
               << "\" fill=\"" << colorStr(style.color) << "\"/>\n";
    }

    // Write SVG to file
    void finish() {
        std::ofstream f(m_filename);
        f << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
          << "<svg xmlns=\"http://www.w3.org/2000/svg\""
          << " width=\"" << m_width << "\" height=\"" << m_height << "\""
          << " viewBox=\"0 0 " << m_width << " " << m_height << "\">\n"
          << "<defs>\n"  // clipPath defs will be here
          << "</defs>\n"
          << "<rect width=\"100%\" height=\"100%\" fill=\"white\"/>\n"
          << m_body.str()
          << "</svg>\n";
        f.close();
        std::printf("SVG written: %s (%dx%d)\n", m_filename.c_str(), m_width, m_height);
    }

    // Get current SVG content as string (for preview/debug)
    std::string svgString() const { return m_body.str(); }
};

} // namespace xyplot
