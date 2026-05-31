// ============================================================
// examples/qt_backend/qt_render_device.h — Qt 渲染设备实现
// ============================================================
// Owner: Agent F (文档 & 示例)
//
// 此头文件提供基于 QPainter 的 IRenderDevice 完整实现。
//
// 用法:
//   1. 在你的 QWidget::paintEvent() 中创建 QtRenderDevice
//   2. 传入 QPainter 指针
//   3. 调用 Plot::render(qtDevice) 即可在 Qt 窗口中绘图
//
// 依赖: Qt5 或 Qt6 (Widgets 模块)
//
// 支持:
//   - 完整的 8 个纯虚方法实现
//   - 文本度量 (QFontMetricsF 精确测量)
//   - 裁剪区域 (QPainter::setClipRect)
//   - 虚线样式映射
//   - 标记形状绘制 (Circle, Square, Diamond, Triangle, Cross, Plus)
// ============================================================

#pragma once

#include <xyplot/xyplot.h>

#include <QPainter>
#include <QFont>
#include <QFontMetricsF>
#include <QPen>
#include <QBrush>
#include <QPainterPath>
#include <QPointF>
#include <QVector>
#include <QString>
#include <cmath>

// ═══════════════════════════════════════════════════════════
// QtRenderDevice — 基于 QPainter 的渲染设备
// ═══════════════════════════════════════════════════════════
class QtRenderDevice : public xyplot::IRenderDevice {
public:
    /// 构造并绑定 QPainter。
    /// QPainter 必须已在 begin() 之后，且在 paintEvent() 中创建。
    explicit QtRenderDevice(QPainter* painter)
        : m_painter(painter)
    {
        if (m_painter) {
            m_painter->setRenderHint(QPainter::Antialiasing, true);
            m_painter->setRenderHint(QPainter::TextAntialiasing, true);
        }
    }

    // ── 帧生命周期 ──
    void beginFrame() override {
        if (m_painter) m_painter->save();
    }

    void endFrame() override {
        if (m_painter) m_painter->restore();
    }

    // ── 裁剪 ──
    void setClipRect(double x, double y, double w, double h) override {
        if (m_painter) {
            m_painter->save();
            m_painter->setClipRect(QRectF(x, y, w, h));
        }
    }

    void resetClip() override {
        if (m_painter) m_painter->restore();
    }

    // ── 折线绘制 ──
    void drawPolyline(const double* xs, const double* ys,
                      int count, const xyplot::LineStyle& style) override {
        if (!m_painter || count < 1) return;

        QPen pen = makePen(style);
        m_painter->setPen(pen);
        m_painter->setBrush(Qt::NoBrush);

        // 构建 QPainterPath 实现高效折线（含虚线）
        QPainterPath path;
        path.moveTo(xs[0], ys[0]);
        for (int i = 1; i < count; ++i) {
            path.lineTo(xs[i], ys[i]);
        }
        m_painter->drawPath(path);
    }

    // ── 标记点绘制 ──
    void drawMarkers(const double* xs, const double* ys,
                     int count, const xyplot::MarkerStyle& style) override {
        if (!m_painter || count < 1) return;

        QBrush fillBrush = makeBrush(style.fillColor);
        QPen edgePen = makePen(style.edgeColor, style.edgeWidth);
        m_painter->setPen(edgePen);
        m_painter->setBrush(fillBrush);

        double halfSize = style.size * 0.5;

        for (int i = 0; i < count; ++i) {
            double cx = xs[i];
            double cy = ys[i];

            switch (style.shape) {
            case xyplot::MarkerStyle::Circle:
                m_painter->drawEllipse(QPointF(cx, cy),
                                       halfSize, halfSize);
                break;

            case xyplot::MarkerStyle::Square:
                m_painter->drawRect(QRectF(cx - halfSize, cy - halfSize,
                                           style.size, style.size));
                break;

            case xyplot::MarkerStyle::Diamond: {
                QPolygonF diamond;
                diamond << QPointF(cx, cy - halfSize)       // top
                        << QPointF(cx + halfSize, cy)        // right
                        << QPointF(cx, cy + halfSize)        // bottom
                        << QPointF(cx - halfSize, cy);       // left
                m_painter->drawPolygon(diamond);
                break;
            }

            case xyplot::MarkerStyle::Triangle: {
                QPolygonF tri;
                tri << QPointF(cx, cy - halfSize)            // top
                    << QPointF(cx + halfSize, cy + halfSize) // bottom-right
                    << QPointF(cx - halfSize, cy + halfSize);// bottom-left
                m_painter->drawPolygon(tri);
                break;
            }

            case xyplot::MarkerStyle::Cross: {
                QPainterPath cross;
                cross.moveTo(cx - halfSize, cy - halfSize);
                cross.lineTo(cx + halfSize, cy + halfSize);
                cross.moveTo(cx + halfSize, cy - halfSize);
                cross.lineTo(cx - halfSize, cy + halfSize);
                m_painter->drawPath(cross);
                break;
            }

            case xyplot::MarkerStyle::Plus: {
                QPainterPath plus;
                plus.moveTo(cx, cy - halfSize);
                plus.lineTo(cx, cy + halfSize);
                plus.moveTo(cx - halfSize, cy);
                plus.lineTo(cx + halfSize, cy);
                m_painter->drawPath(plus);
                break;
            }
            }
        }
    }

    // ── 文本绘制 ──
    void drawText(double x, double y, const char* text,
                  const xyplot::FontDesc& font,
                  const xyplot::TextStyle& style) override {
        if (!m_painter || !text) return;

        QFont qFont = makeFont(font);
        QPen textPen = makePen(style.color, 1.0);
        m_painter->setFont(qFont);
        m_painter->setPen(textPen);

        QString qText = QString::fromUtf8(text);
        QFontMetricsF fm(qFont);

        // 根据对齐调整绘制位置
        double drawX = x;
        double drawY = y;

        if (style.hAlign == xyplot::TextStyle::Center) {
            drawX -= fm.horizontalAdvance(qText) * 0.5;
        } else if (style.hAlign == xyplot::TextStyle::Right) {
            drawX -= fm.horizontalAdvance(qText);
        }

        if (style.vAlign == xyplot::TextStyle::Middle) {
            drawY += fm.ascent() * 0.5 - fm.descent() * 0.5;
        } else if (style.vAlign == xyplot::TextStyle::Top) {
            drawY += fm.ascent();
        }
        // Bottom (default): y 已在基线处

        m_painter->drawText(QPointF(drawX, drawY), qText);
    }

    // ── 填充矩形 ──
    void fillRect(double x, double y, double w, double h,
                  const xyplot::FillStyle& style) override {
        if (!m_painter) return;

        m_painter->setPen(Qt::NoPen);
        m_painter->setBrush(makeBrush(style.color));
        m_painter->drawRect(QRectF(x, y, w, h));
    }

    // ── 文本度量（覆盖默认估算，提供精确布局） ──
    void textExtent(const char* text, const xyplot::FontDesc& font,
                    double* w, double* h) override {
        if (!text) {
            if (w) *w = 0;
            if (h) *h = 0;
            return;
        }

        QFont qFont = makeFont(font);
        QFontMetricsF fm(qFont);
        QString qText = QString::fromUtf8(text);

        if (w) *w = fm.horizontalAdvance(qText);
        if (h) *h = fm.height();
    }

private:
    QPainter* m_painter;

    // ── 工具: 创建 QPen ──
    static QPen makePen(const xyplot::Color& color, double width = 1.0) {
        return QPen(QColor(color.r, color.g, color.b, color.a), width);
    }

    static QPen makePen(const xyplot::LineStyle& style) {
        QPen pen(QColor(style.color.r, style.color.g,
                        style.color.b, style.color.a),
                 style.width);

        switch (style.dash) {
        case xyplot::LineStyle::SolidLine:
            pen.setStyle(Qt::SolidLine);
            break;
        case xyplot::LineStyle::DashLine:
            pen.setStyle(Qt::DashLine);
            break;
        case xyplot::LineStyle::DotLine:
            pen.setStyle(Qt::DotLine);
            break;
        case xyplot::LineStyle::DashDotLine:
            pen.setStyle(Qt::DashDotLine);
            break;
        }
        return pen;
    }

    // ── 工具: 创建 QBrush ──
    static QBrush makeBrush(const xyplot::Color& color) {
        return QBrush(QColor(color.r, color.g, color.b, color.a));
    }

    // ── 工具: 创建 QFont ──
    static QFont makeFont(const xyplot::FontDesc& font) {
        QFont qFont;
        qFont.setPixelSize(static_cast<int>(font.size));
        qFont.setBold(font.bold);
        qFont.setItalic(font.italic);
        return qFont;
    }
};
