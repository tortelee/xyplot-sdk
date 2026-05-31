# API Reference — XYPlot SDK v0.1.0

**文档日期**：2026-05-31
**覆盖范围**：所有公开 API（`include/xyplot/` 下的头文件）

---

## 目录

1. [快速参考 — 最小集成](#1-快速参考--最小集成)
2. [类型系统 (types.h)](#2-类型系统-typesh)
3. [渲染设备接口 (irender_device.h)](#3-渲染设备接口-irender_deviceh)
4. [输入事件接口 (iinput_source.h)](#4-输入事件接口-iinput_sourceh)
5. [Plot 门面类 (plot.h)](#5-plot-门面类-plotth)
6. [主头文件 (xyplot.h)](#6-主头文件-xyploth)
7. [核心算法 API（内部头文件）](#7-核心算法-api内部头文件)
8. [图类型注册中心](#8-图类型注册中心)
9. [错误处理与契约](#9-错误处理与契约)

---

## 1. 快速参考 — 最小集成

客户集成 XYPlot SDK 需要做的最少事情：

```cpp
#include <xyplot/xyplot.h>   // 唯一需要的 include

// Step 1: 实现 IRenderDevice (8 个纯虚方法)
class MyDevice : public xyplot::IRenderDevice {
public:
    void beginFrame() override { /* ... */ }
    void endFrame() override { /* ... */ }
    void setClipRect(double x, double y, double w, double h) override { /* ... */ }
    void resetClip() override { /* ... */ }
    void drawPolyline(const double* xs, const double* ys,
                      int count, const xyplot::LineStyle& style) override { /* ... */ }
    void drawMarkers(const double* xs, const double* ys,
                     int count, const xyplot::MarkerStyle& style) override { /* ... */ }
    void drawText(double x, double y, const char* text,
                  const xyplot::FontDesc& font,
                  const xyplot::TextStyle& style) override { /* ... */ }
    void fillRect(double x, double y, double w, double h,
                  const xyplot::FillStyle& style) override { /* ... */ }
    // textExtent() 有默认实现，可选 override 以获精确布局
};

// Step 2: 创建 Plot，喂数据，渲染
xyplot::Plot plot;
plot.addLineSeries("Series 1", xs, ys, N);
plot.render(myDevice);
```

---

## 2. 类型系统 (types.h)

公共头文件 `include/xyplot/types.h` 定义了所有基础数据结构。所有类型在 `xyplot` 命名空间下。

### 2.1 Color

```cpp
struct Color {
    uint8_t r = 0, g = 0, b = 0, a = 255;  // RGBA, 每通道 8-bit

    static Color fromHex(uint32_t hex);     // e.g., 0xFF0000 → 红色
};
```

| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `r` | `uint8_t` | `0` | 红色通道 (0–255) |
| `g` | `uint8_t` | `0` | 绿色通道 (0–255) |
| `b` | `uint8_t` | `0` | 蓝色通道 (0–255) |
| `a` | `uint8_t` | `255` | 透明度通道 (0=全透明, 255=不透明) |

**工厂方法**：

| 方法 | 说明 |
|------|------|
| `Color::fromHex(0xFF0000)` | 从 24-bit RGB Hex 创建颜色，返回 `Color{255,0,0,255}` |

---

### 2.2 LineStyle

```cpp
struct LineStyle {
    double width = 1.0;                    // 线宽（设备空间像素）
    Color color{};                         // 线条颜色
    enum DashStyle { SolidLine, DashLine, DotLine, DashDotLine };
    DashStyle dash = SolidLine;            // 虚线样式
};
```

| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `width` | `double` | `1.0` | 线宽（像素） |
| `color` | `Color` | `{0,0,0,255}` | 线条颜色 |
| `dash` | `DashStyle` | `SolidLine` | 实线/虚线样式 |

**DashStyle 枚举值**：

| 值 | 说明 |
|------|------|
| `SolidLine` | 实线（默认） |
| `DashLine` | 虚线 |
| `DotLine` | 点线 |
| `DashDotLine` | 点划线 |

---

### 2.3 MarkerStyle

```cpp
struct MarkerStyle {
    enum Shape { Circle, Square, Diamond, Triangle, Cross, Plus };
    Shape shape = Circle;              // 标记形状
    double size = 6.0;                // 标记大小（像素）
    Color fillColor{};                // 填充色
    Color edgeColor{};                // 边缘色
    double edgeWidth = 1.0;          // 边缘宽度（像素）
};
```

**Shape 枚举值**：`Circle`, `Square`, `Diamond`, `Triangle`, `Cross`, `Plus`

---

### 2.4 FillStyle

```cpp
struct FillStyle {
    Color color{200, 200, 200};      // 填充颜色，默认浅灰
};
```

---

### 2.5 FontDesc

```cpp
struct FontDesc {
    double size = 12.0;              // 字号（设备空间像素）
    bool bold = false;               // 粗体
    bool italic = false;             // 斜体
};
```

---

### 2.6 TextStyle

```cpp
struct TextStyle {
    Color color{};                   // 文本颜色
    enum Align { Left, Center, Right };
    Align hAlign = Left;            // 水平对齐
    enum VAlign { Top, Middle, Bottom };
    VAlign vAlign = Bottom;         // 垂直对齐（相对于基线）
};
```

**绘制文本时**：`drawText(x, y, ...)` 中的 `(x, y)` 是文本的锚点。`hAlign` 和 `vAlign` 控制文本相对于锚点的对齐方式：
- `hAlign=Left`：x 为文本左边缘
- `hAlign=Center`：x 为文本水平中心
- `hAlign=Right`：x 为文本右边缘
- `vAlign=Bottom`：y 为文本基线（bottom of the font ascent）
- `vAlign=Middle`：y 为文本垂直中心
- `vAlign=Top`：y 为文本顶部

---

### 2.7 其他类型

```cpp
enum class ScaleType { Linear, Log10, Ln };  // 轴刻度类型

struct DataPoint { double x = 0, y = 0; };   // 数据空间中的点
struct Rect { double x = 0, y = 0, w = 0, h = 0; };  // 设备空间矩形
```

| 类型 | 说明 |
|------|------|
| `ScaleType::Linear` | 线性刻度 |
| `ScaleType::Log10` | 以 10 为底的对数刻度 |
| `ScaleType::Ln` | 自然对数刻度 |
| `Rect` | `(x, y)` = 左上角，`w, h` = 宽高 |

---

## 3. 渲染设备接口 (irender_device.h)

```cpp
class IRenderDevice {
public:
    virtual ~IRenderDevice() = default;

    // ──── 帧生命周期 (2) ────
    virtual void beginFrame() = 0;              // 开始一帧
    virtual void endFrame() = 0;                // 结束一帧

    // ──── 裁剪 (2) ────
    virtual void setClipRect(double x, double y, double w, double h) = 0;
    virtual void resetClip() = 0;

    // ──── 折线 (1) ────
    virtual void drawPolyline(const double* xs, const double* ys,
                              int count, const LineStyle& style) = 0;

    // ──── 标记点 (1) ────
    virtual void drawMarkers(const double* xs, const double* ys,
                             int count, const MarkerStyle& style) = 0;

    // ──── 文本 (1) ────
    virtual void drawText(double x, double y, const char* text,
                          const FontDesc& font, const TextStyle& style) = 0;

    // ──── 填充矩形 (1) ────
    virtual void fillRect(double x, double y, double w, double h,
                          const FillStyle& style) = 0;

    // ──── 文本度量 (可选) ────
    virtual void textExtent(const char* text, const FontDesc& font,
                            double* w, double* h);  // 有默认估算实现
};
```

### 3.1 方法详解

#### beginFrame() / endFrame()

帧生命周期方法。`beginFrame` 在渲染开始时调用，`endFrame` 在结束时调用。

**调用模式**：Plot::render() 始终成对调用 `beginFrame → ... → endFrame`。

```cpp
void beginFrame() override {
    // 在这里做: 清除缓冲区、初始化设备状态...
}

void endFrame() override {
    // 在这里做: 交换缓冲区、flush 绘制命令...
}
```

---

#### setClipRect(x, y, w, h) / resetClip()

裁剪区域。所有 `(x, y, w, h)` 均为设备空间像素坐标。

- `setClipRect`：设置当前裁剪矩形。裁剪区域外的图元不被绘制（需客户设备实现裁剪逻辑）
- `resetClip`：清除裁剪，恢复全屏绘制

**调用模式**：Plot 在绘制数据和网格前设置裁剪为绘图区，绘制完成后 reset。

```cpp
void setClipRect(double x, double y, double w, double h) override {
    // e.g., QPainter::setClipRect(QRectF(x, y, w, h))
}

void resetClip() override {
    // e.g., QPainter::setClipping(false)
}
```

---

#### drawPolyline(xs, ys, count, style)

绘制一条多段折线。所有覆盖场景：

| 场景 | 调用方式 |
|------|---------|
| 折线图曲线 | `count=N, style=用户配置` |
| 坐标轴线 | `count=2, style.width=1.5, dash=SolidLine` |
| 网格线 | `count=2, style.width=0.5, dash=DashLine` |
| 图例色块边框 | `count=5, style.width=0.5` |

**参数**：

| 参数 | 类型 | 说明 |
|------|------|------|
| `xs` | `const double*` | X 坐标数组（设备空间像素） |
| `ys` | `const double*` | Y 坐标数组（设备空间像素） |
| `count` | `int` | 点数 (≥ 1) |
| `style` | `const LineStyle&` | 线条样式 |

**注意**：`xs` 和 `ys` 各 `count` 个 double，坐标顺序保证连续（点在折线中按索引顺序连接）。

---

#### drawMarkers(xs, ys, count, style)

在指定位置绘制标记点。覆盖场景：

| 场景 | 调用方式 |
|------|---------|
| 散点图 | `count=N, style=MarkerStyle` |
| 折线图数据点 | `count=N, style.size>0 时叠加` |

---

#### drawText(x, y, text, font, style)

在设备坐标 `(x, y)` 处绘制文本。

**参数**：

| 参数 | 类型 | 说明 |
|------|------|------|
| `x` | `double` | 锚点 X 坐标（设备空间） |
| `y` | `double` | 锚点 Y 坐标（设备空间） |
| `text` | `const char*` | 以 null 结尾的 UTF-8 C 字符串 |
| `font` | `const FontDesc&` | 字体描述 |
| `style` | `const TextStyle&` | 文本样式（含对齐、颜色） |

**坐标语义**：锚点 `(x, y)` + `TextStyle::hAlign/vAlign` 决定文本放置位置。

---

#### fillRect(x, y, w, h, style)

填充一个矩形区域。覆盖场景：

| 场景 | 调用方式 |
|------|---------|
| 图例色块 | `fillRect + Color` |
| 图例背景 | `fillRect + semi-transparent white` |
| 绘图区背景 | `fillRect + light gray` |

---

#### textExtent(text, font, w, h) [可选]

测量文本的宽度和高度。

- **默认实现**：保守估算 `w ≈ strlen(text) * fontSize * 0.6`, `h ≈ fontSize * 1.25`
- **覆盖建议**：若需要精确文本布局（像素级对齐），覆盖此方法

```cpp
void textExtent(const char* text, const FontDesc& font,
                double* w, double* h) override {
    // e.g., QFontMetricsF::boundingRect(text).width()
    // if (w) *w = ...;
    // if (h) *h = ...;
}
```

#### fillPolygon(xs, ys, count, style) [P1 — 默认空实现]

填充任意多边形。P1 Area Plot 使用此方法绘制填充区域。

**参数**：

| 参数 | 类型 | 说明 |
|------|------|------|
| `xs` | `const double*` | 多边形顶点 X 坐标数组（设备空间像素） |
| `ys` | `const double*` | 多边形顶点 Y 坐标数组（设备空间像素） |
| `count` | `int` | 顶点数 (≥ 3) |
| `style` | `const FillStyle&` | 填充样式（颜色 + 透明度） |

**默认实现**：空操作。此方法为 P1 扩展，标记为 `virtual` 而非纯虚——不实现不影响 P0 功能。

**实现建议**：

```cpp
void fillPolygon(const double* xs, const double* ys,
                 int count, const xyplot::FillStyle& style) override {
    // Qt: QPainterPath + drawPolygon
    // Blend2D: BLPath + fillPath
    // OpenGL: glBegin(GL_POLYGON) / triangle fan
}
```

---

#### drawImage(x, y, w, h, rgba, imgW, imgH) [P1 — 默认空实现]

将像素图像渲染到设备空间的指定矩形区域。P1 Heatmap 使用此方法渲染热力矩阵。

**参数**：

| 参数 | 类型 | 说明 |
|------|------|------|
| `x`, `y`, `w`, `h` | `double` | 目标矩形（设备空间像素） |
| `rgba` | `const uint8_t*` | RGBA 像素数据，大小为 `imgW × imgH × 4` |
| `imgW`, `imgH` | `int` | 源图像宽度和高度（像素） |

**默认实现**：空操作。此方法为 P1 扩展，不实现不影响 P0 功能。

**实现建议**：

```cpp
void drawImage(double x, double y, double w, double h,
               const uint8_t* rgba, int imgW, int imgH) override {
    // Qt: QImage(rgba, imgW, imgH, QImage::Format_RGBA8888) → QPainter::drawImage
    // Blend2D: BLImage + BLContext::blitImage
    // OpenGL: glTexImage2D + textured quad
}
```

---

### 3.2 方法总览

| 方法 | 优先级 | 类型 | 覆盖场景 |
|------|--------|------|---------|
| `beginFrame` / `endFrame` | P0 必需 | 纯虚 | 帧生命周期 |
| `setClipRect` / `resetClip` | P0 必需 | 纯虚 | 裁剪 |
| `drawPolyline` | P0 必需 | 纯虚 | 折线图、坐标轴、网格线 |
| `drawMarkers` | P0 必需 | 纯虚 | 散点图、数据点标记 |
| `drawText` | P0 必需 | 纯虚 | 标题、轴标签、刻度标签 |
| `fillRect` | P0 必需 | 纯虚 | 图例色块、背景 |
| `textExtent` | P0 可降级 | 虚（有默认） | 文本度量 |
| `fillPolygon` | **P1 扩展** | 虚（默认空） | Area Plot 填充 |
| `drawImage` | **P1 扩展** | 虚（默认空） | Heatmap 像素渲染 |

---

## 4. 输入事件接口 (iinput_source.h)

### 4.1 InputEvent

```cpp
struct InputEvent {
    enum Type {
        None = 0,
        MouseDown, MouseUp, MouseMove, MouseWheel,
        KeyDown, KeyUp,
        TouchBegin, TouchMove, TouchEnd
    };
    Type type = None;
    double x = 0, y = 0;          // 设备空间坐标
    int button = 0;               // 0=left, 1=middle, 2=right
    int modifiers = 0;            // 修饰键位掩码: Shift=1, Ctrl=2, Alt=4
    double wheelDelta = 0;        // 滚轮增量
    int keyCode = 0;              // 按键码
};
```

### 4.2 IInputSource

```cpp
class IInputSource {
public:
    virtual ~IInputSource() = default;
    virtual bool pollEvent(InputEvent& out) = 0;
    // 返回 true → out 中有新事件
    // 返回 false → 无事件
};
```

**`IInputSource` 完全可选**。若客户不实现此接口，SDK 仅提供 `Plot::render()`，不提供交互。这对于"仅需出图"的场景是最简接入方式。

---

## 5. Plot 门面类 (plot.h)

```cpp
class Plot {
public:
    Plot();
    ~Plot();

    // 不可拷贝，可移动
    Plot(const Plot&) = delete;
    Plot& operator=(const Plot&) = delete;
    Plot(Plot&&) noexcept;
    Plot& operator=(Plot&&) noexcept;

    // ──── 数据绑定 ────
    int addLineSeries(const char* name,
                      const double* xs, const double* ys, int count);
    int addScatterSeries(const char* name,
                         const double* xs, const double* ys, int count);
    void updateSeriesData(int seriesId,
                          const double* xs, const double* ys, int count);

    // ──── 轴配置 ────
    void xAxisSetLabel(const char* label);
    void yAxisSetLabel(const char* label);
    void yAxisAddRight(const char* label);
    void xAxisSetScale(ScaleType type);
    void setAxisRange(double xMin, double xMax,
                      double yMin, double yMax, int yAxisIndex = 0);

    // ──── 样式 ────
    void setTitle(const char* title);
    void setSeriesStyle(int seriesId, const LineStyle& style);

    // ──── 渲染 ────
    void render(IRenderDevice& device);

    // ──── 交互 ────
    InteractionResult handleEvent(const InputEvent& event,
                                   IRenderDevice* device = nullptr);

private:
    class Impl;
    Impl* m_impl;
};
```

### 5.1 数据绑定

#### addLineSeries(name, xs, ys, count)

添加一条折线图 series。

| 参数 | 类型 | 说明 |
|------|------|------|
| `name` | `const char*` | Series 名称（用于图例），可为 nullptr |
| `xs` | `const double*` | X 数据数组 |
| `ys` | `const double*` | Y 数据数组 |
| `count` | `int` | 数据点数量 |

**返回值**：`int` — series ID（从 0 开始），用于后续样式设置或数据更新。

**颜色自动循环**：默认根据添加顺序自动分配颜色（9 色调色板）。

#### addScatterSeries(name, xs, ys, count)

添加一条散点图 series。参数与 `addLineSeries` 相同。返回 series ID。

#### updateSeriesData(seriesId, xs, ys, count)

更新已有 series 的数据。可用于动画或实时数据更新。

| 参数 | 说明 |
|------|------|
| `seriesId` | `addLineSeries` / `addScatterSeries` 返回的 ID |
| `xs`, `ys` | 新数据（大小 ≥ count） |
| `count` | 新数据点数 |

---

### 5.2 轴配置

#### xAxisSetLabel(label) / yAxisSetLabel(label)

设置 X 或 Y（左）轴的标签文本。

| 参数 | 类型 | 说明 |
|------|------|------|
| `label` | `const char*` | 轴标签文本。`nullptr` 或 `""` 清除标签 |

#### setAxisRange(xMin, xMax, yMin, yMax, yAxisIndex)

手动设置轴范围。

| 参数 | 说明 |
|------|------|
| `xMin`, `xMax` | X 轴数据范围 |
| `yMin`, `yMax` | Y 轴数据范围 |
| `yAxisIndex` | Y 轴索引：`0` = 左轴（默认），`≥1` = 右轴 |

**注意**：一旦调用此方法，auto-range 被禁用。若要重新启用 auto-range，暂无公开 API（后续版本提供）。

**支持反转轴**：`xMin > xMax` 或 `yMin > yMax` 可用于反转轴方向。

#### xAxisSetScale(type)

设置 X 轴刻度类型。

| 值 | 说明 |
|------|------|
| `ScaleType::Linear` | 线性刻度（默认） |
| `ScaleType::Log10` | 以 10 为底的对数刻度 |
| `ScaleType::Ln` | 自然对数刻度 |

**注意**：对数刻度要求数据范围 > 0。对于 ≤ 0 的值，坐标变换返回 NaN。

#### yAxisAddRight(label)

添加右 Y 轴并设置标签。允许多次调用以添加多个右轴。

---

### 5.3 样式

#### setTitle(title)

设置图表标题。`nullptr` 或 `""` 表示不显示标题。

#### setSeriesStyle(seriesId, style)

设置指定 series 的线条样式。

```cpp
xyplot::LineStyle style;
style.color = xyplot::Color{255, 0, 0};   // 红色
style.width = 3.0;                          // 线宽 3px
style.dash = xyplot::LineStyle::DashLine;  // 虚线
plot.setSeriesStyle(0, style);
```

---

### 5.4 渲染

#### render(device)

渲染图表。这是 SDK 的主入口。

**调用顺序**（在 `device` 上的调用顺序）：
1. `beginFrame()`
2. `fillRect` — 绘图区背景
3. `setClipRect` — 限制绘制区域
4. `drawPolyline` — 网格线
5. `drawPolyline` / `drawMarkers` — 所有 series 数据
6. `resetClip`
7. `drawPolyline` — 轴线
8. `drawText` — 轴刻度标签、轴标签、标题
9. `fillRect` / `drawText` — 图例
10. `endFrame()`

---

### 5.5 交互

#### handleEvent(event, device)

处理输入事件。`device` 参数可选 — 传入时可用于高亮渲染。

**返回值 `InteractionResult`**：

```cpp
struct InteractionResult {
    enum Action { ViewChanged, DataPicked, CurveSelected, None };
    Action action = None;
    double pickedDataX = 0, pickedDataY = 0;
    int selectedCurveIndex = -1;
};
```

| 字段 | 说明 |
|------|------|
| `action` | 交互动作类型 |
| `pickedDataX`, `pickedDataY` | `DataPicked` 时的数据空间坐标 |
| `selectedCurveIndex` | `CurveSelected` 时的 series ID |

---

### 5.6 生命周期

- **不可拷贝，可移动**：`Plot` 禁止拷贝构造/赋值（因为拥有不共享的内部状态）。支持移动语义。
- **RAII**：构造和析构自动管理内部资源。
- **线程安全**：`Plot` 不提供内部线程安全保证。应在创建线程中调用所有方法。

---

## 6. 主头文件 (xyplot.h)

```cpp
#pragma once
#include "types.h"
#include "irender_device.h"
#include "iinput_source.h"
#include "plot.h"
```

客户只需 `#include <xyplot/xyplot.h>` 即可获得全部公开 API。

---

## 7. 核心算法 API（内部头文件）

以下 API 位于 `src/` 目录下的内部头文件中，供 Agent D (图类型) 和 Agent E (门面) 使用。**客户通常不需要直接使用这些 API**，但若需自定义图类型实现，可参考。

### 7.1 DataTable (datatable.h)

```cpp
class DataTable {
public:
    static DataTable fromMemory(const double* data, int rows, int cols,
                                const char* const* colNames = nullptr);
    static DataTable fromCSV(const char* filename, bool hasHeader = true);

    const double* column(int index) const;
    const double* column(const char* name) const;
    int rowCount() const;
    int colCount() const;
    const std::string& columnName(int index) const;
    int columnIndex(const char* name) const;
};
```

---

### 7.2 Axis System (axis_system.h)

**输入结构体 `AxisConfig`**：

```cpp
struct AxisConfig {
    double    dataMin          = 0.0;   // 数据范围下限
    double    dataMax          = 1.0;   // 数据范围上限
    ScaleType scaleType        = ScaleType::Linear;  // 刻度类型
    int       targetMajorTicks = 5;     // 目标主刻度数
    int       targetMinorTicks = 1;     // 主刻度间的次刻度数（0=无次刻度）
};
```

**输出结构体 `AxisTicks`**：

```cpp
struct AxisTicks {
    std::vector<double>      majorTicks;   // 主刻度值（数据空间）
    std::vector<double>      minorTicks;   // 次刻度值（可为空）
    std::vector<std::string> labels;       // 格式化标签，与 majorTicks 1:1 对应
    double                   niceMin = 0;  // nice 范围下限
    double                   niceMax = 1;  // nice 范围上限
};

struct TickInfo {
    double value = 0.0;       // 刻度值
    bool   isMajor = true;    // true=主刻度, false=次刻度
};
```

**函数 API**：

```cpp
double niceNumber(double x, bool round);
int autoPrecision(double tickInterval);
std::string formatTickLabel(double value, int precision = -1);

AxisTicks computeTicks(const AxisConfig& config);
```

**Nice Number 算法示例**：

| 输入范围 | 目标刻度数 | niceInterval | 输出刻度 |
|---------|-----------|-------------|----------|
| [0.3, 8.7] | 5 | 2.0 | {0, 2, 4, 6, 8, 10} |
| [-5.3, 3.2] | 5 | 2.0 | {-6, -4, -2, 0, 2, 4} |
| [1, 1000] (Log10) | 4 | 1.0 | {1, 10, 100, 1000} |

---

### 7.3 Coordinate Transform (coordinate_transform.h)

```cpp
// 单值变换
double normalize(double dataValue, double dataMin, double dataMax, ScaleType);
double denormalize(double normValue, double deviceMin, double deviceMax);
double transform(double dataValue, double dataMin, double dataMax,
                 double deviceMin, double deviceMax, ScaleType);
double inverseTransform(double deviceValue, double dataMin, double dataMax,
                        double deviceMin, double deviceMax, ScaleType);

// 批量变换
void transformPoints(const double* dataX, const double* dataY, int count,
                     double dataXMin, double dataXMax,
                     double dataYMin, double dataYMax,
                     double deviceXMin, double deviceXMax,
                     double deviceYMin, double deviceYMax,
                     double* outX, double* outY,
                     ScaleType xScale = ScaleType::Linear,
                     ScaleType yScale = ScaleType::Linear);

/// 单数组变换（仅 X 或仅 Y）
void transformArray(const double* data, int count,
                    double dataMin, double dataMax,
                    double deviceMin, double deviceMax,
                    double* out,
                    ScaleType scaleType = ScaleType::Linear);
```

**变换管道**：`DataSpace → NormalizedSpace [0,1] → DeviceSpace (pixels)`

---

### 7.4 Layout Engine (layout_engine.h)

**输入结构体 `LayoutConfig`**：

```cpp
struct LayoutConfig {
    double totalWidth  = 800.0;    // 可用设备宽度
    double totalHeight = 600.0;    // 可用设备高度

    // 内容存在性标记
    bool hasTitle           = false;
    bool hasXAxisLabel      = false;
    bool hasYAxisLabel      = false;
    bool hasYAxisRightLabel = false;
    bool hasLegend          = false;

    // 覆盖尺寸（0 = 根据字体自动计算）
    double titleHeight      = 0.0;
    double xLabelHeight     = 0.0;
    double yLabelWidth      = 0.0;
    double legendWidth      = 0.0;
    double legendHeight     = 0.0;

    // 字体描述
    FontDesc titleFont     {14.0, true,  false};
    FontDesc axisLabelFont {12.0, false, false};
    FontDesc tickLabelFont {10.0, false, false};
    FontDesc legendFont    {11.0, false, false};

    // 文本内容（用于 textExtent 测量）
    std::string title;
    std::string xLabel;
    std::string yLabel;
    std::string yRightLabel;

    double maxTickLabelWidth  = 0.0;   // 最长刻度标签宽度（0=自动估算）
    double maxTickLabelHeight = 0.0;   // 最长刻度标签高度（0=自动估算）
    int    legendItemCount    = 0;     // 图例条目数
};
```

**输出结构体 `LayoutResult`**：

```cpp
struct LayoutResult {
    Rect titleRect;          // 标题区
    Rect plotRect;           // 主绘图区
    Rect xAxisRect;          // X 轴线 + 刻度（绘图区下方）
    Rect yAxisRect;          // Y 轴线 + 刻度（绘图区左侧）
    Rect yAxisRightRect;     // 右Y轴线 + 刻度（绘图区右侧）
    Rect legendRect;         // 图例区
    Rect xLabelRect;         // X 轴标签（X 轴下方）
    Rect yLabelRect;         // Y 轴标签（Y 轴左侧）
    Rect yLabelRightRect;    // 右Y轴标签（右Y轴右侧）

    bool isValid() const;    // plotRect.w > 0 && plotRect.h > 0
};
```

**函数 API**：

```cpp
// 精确布局（使用 device.textExtent() 测量文本）
LayoutResult computeLayout(const LayoutConfig& config, IRenderDevice& device);

// 最小布局（使用保守字体估算，无需 device）
LayoutResult computeLayoutMinimal(const LayoutConfig& config);
```

**布局策略**：渐进减法 — 从总画布减去标题区、轴标签区、图例区，剩余空间 = 绘图区。

---

### 7.5 HitTest (hit_test.h)

```cpp
struct HitTestResult {
    enum HitType { None, PlotArea, Axis, LegendItem, CurveLine, DataPoint };
    HitType hitType = None;
    int seriesIndex = -1;
    int dataPointIndex = -1;
    double dataX = 0.0, dataY = 0.0;
    double distancePx = 0.0;
};

class HitTest {
public:
    static constexpr double kCurveHitRadius = 8.0;      // 曲线命中半径 (px)
    static constexpr double kDataPointHitRadius = 12.0; // 数据点命中半径 (px)

    static HitTestResult test(double px, double py,
                              const HitTestLayout& layout,
                              const std::vector<HitTestSeries>& series,
                              const HitTestAxisRange& axis);
};
```

---

## 8. 图类型注册中心

位于 `src/plot_registry.cpp`，内部 API。

```cpp
namespace xyplot::internal {
    std::unique_ptr<IPlotType> createPlotType(const std::string& name);
    bool registerPlotType(const std::string& name, PlotRegistry::Factory factory);
    std::vector<std::string> listPlotTypes();
}
```

**内置图类型**：

| 图类型 | 注册名 | 优先级 | 说明 |
|--------|--------|--------|------|
| Line Plot | `"Line"` | P0 | 折线图 |
| Scatter Plot | `"Scatter"` | P0 | 散点图 |
| Bar Chart | `"Bar"` | P1 | 柱状图（fillRect 绘制） |
| Step Plot | `"Step"` | P1 | 阶梯图（drawPolyline 阶梯路径） |
| Error Bar | `"ErrorBar"` | P1 | 误差棒（drawPolyline 误差线） |
| Histogram | `"Histogram"` | P1 | 直方图（fillRect + 分箱） |
| Polar Plot | `"Polar"` | P1 | 极坐标图（drawPolyline + 极坐标变换） |
| Area Plot | `"Area"` | P1 B2 | 面积图（fillPolygon 填充 + drawPolyline 边界） |
| Heatmap | `"Heatmap"` | P1 B2 | 热力图（drawImage 像素渲染） |
| Contour | `"Contour"` | P1 B2 | 等值线图（Marching Squares + drawPolyline） |

**自定义图类型**：实现 `IPlotType` 接口并通过 `registerPlotType` 注册。

---

## 9. 错误处理与契约

### 9.1 编译时契约

- **IRenderDevice 接口变更**：必须同步更新 `tests/test_interface_contract.cpp`。CI 门禁使用 `static_assert` 确保 `ContractDevice` 实现所有纯虚方法。
- **类型冻结**：`types.h` 中的类型结构字段只增不删，新增字段必须有默认值。

### 9.2 运行时契约

| 场景 | 行为 |
|------|------|
| 空 series | `render()` 优雅跳过（不崩溃，不调用 draw 方法） |
| 数据范围 min==max | 自动扩展 ±1（轴）或居中（变换） |
| 负值 + Log10 轴 | 坐标变换返回 NaN，渲染时跳过 |
| 无效 seriesId | `updateSeriesData` 和 `setSeriesStyle` 静默忽略 |
| 空/null 字符串 | 所有标签/标题方法接受 `nullptr` 或 `""` |
| 移动后对象 | 源 `Plot` 变为空壳（`m_impl == nullptr`），不可再调用方法 |

### 9.3 性能契约

- `render()` 内部不分配大块内存（复用栈/小堆分配）
- `drawPolyline` 的 `xs`/`ys` 指针在调用期间有效（SDK 不持有）
- 坐标变换为纯计算，无副作用
