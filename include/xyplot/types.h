// ============================================================
// xyplot/types.h — 基础类型定义 (冻结 v1.0)
// ============================================================
#pragma once
#include <cstdint>

namespace xyplot {

// ──── 颜色 ────
struct Color {
    uint8_t r = 0, g = 0, b = 0, a = 255;

    static Color fromHex(uint32_t hex) {
        return {static_cast<uint8_t>((hex >> 16) & 0xFF),
                static_cast<uint8_t>((hex >> 8) & 0xFF),
                static_cast<uint8_t>(hex & 0xFF),
                255};
    }
};

// ──── 线段样式 ────
struct LineStyle {
    double width = 1.0;
    Color color{};
    enum DashStyle { SolidLine, DashLine, DotLine, DashDotLine };
    DashStyle dash = SolidLine;
};

// ──── 标记样式 ────
struct MarkerStyle {
    enum Shape { Circle, Square, Diamond, Triangle, Cross, Plus };
    Shape shape = Circle;
    double size = 6.0;
    Color fillColor{};
    Color edgeColor{};
    double edgeWidth = 1.0;
};

// ──── 填充样式 ────
struct FillStyle {
    Color color{200, 200, 200};
};

// ──── 字体描述 ────
struct FontDesc {
    double size = 12.0;
    bool bold = false;
    bool italic = false;
};

// ──── 文本样式 ────
struct TextStyle {
    Color color{};
    enum Align { Left, Center, Right };
    Align hAlign = Left;
    enum VAlign { Top, Middle, Bottom };
    VAlign vAlign = Bottom;
};

// ──── 刻度类型 ────
enum class ScaleType { Linear, Log10, Ln };

// ──── 数据空间点 ────
struct DataPoint { double x = 0, y = 0; };

// ──── 设备空间矩形 ────
struct Rect { double x = 0, y = 0, w = 0, h = 0; };

} // namespace xyplot
