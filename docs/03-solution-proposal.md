# 方案建议书：后端无关的 XY Plot SDK 架构

**项目**：XY Plot 画图 SDK
**文档日期**：2026-05-31
**版本**：v2.0（根据 PO 反馈重写）
**作者**：Project Lead
**前置依赖**：[01 PO 回复](./01-requirements-clarification.md) | [02 技术评估 v2](./02-technical-assessment.md)

---

## 一、架构总览

### 1.1 核心设计理念

```
┌─────────────────────────────────────────────────────────────────┐
│                     CUSTOMER APPLICATION                        │
│                                                                 │
│  ┌──────────┐   ┌──────────────┐   ┌──────────────────────┐   │
│  │ Customer │   │  Our SDK     │   │ Customer's Display   │   │
│  │ Data     │──▶│  (Core)      │──▶│ System               │   │
│  │ Source   │   │              │   │ (Qt / MFC / WPF /    │   │
│  └──────────┘   │  • DataModel │   │  ImGui / OpenGL /    │   │
│                 │  • PlotEngine│   │  Custom ...)         │   │
│  ┌──────────┐   │  • Layout    │   └──────────────────────┘   │
│  │ Customer │   │  • Axes      │                               │
│  │ Input    │──▶│              │   ┌──────────────────────┐   │
│  │ Events   │   │  Interfaces: │   │ Customer's Display   │   │
│  └──────────┘   │  IRenderDevice│◀──│ Adapter (implements  │   │
│                 │  IInputSource │   │ IRenderDevice)       │   │
│                 └──────────────┘   └──────────────────────┘   │
└─────────────────────────────────────────────────────────────────┘

SDK = Pure logic, zero UI framework dependency.
Customer = Owns the window, the event loop, the rendering context.
```

### 1.2 设计原则

| 原则 | 说明 |
|------|------|
| **零 UI 依赖** | SDK 核心不链接任何 UI 框架，纯 C++17 标准库 |
| **接口即契约** | 两个纯虚接口：`IRenderDevice` + `IInputSource`，客户实现 |
| **数据进，像素出** | SDK 接收数据 → 计算坐标 → 调用客户提供的渲染接口 |
| **图类型即策略** | 每种图类型是独立的渲染策略，通过统一接口调用 |
| **渐进式复杂度** | 客户可只实现渲染；交互可选 |

---

## 二、分层架构

```
┌─────────────────────────────────────────────────────────────┐
│                   Customer Application                       │
│          (owns window, event loop, OpenGL context, etc.)    │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌─────────────────── SDK PUBLIC API ───────────────────┐  │
│  │                                                       │  │
│  │  ┌─────────────┐  ┌─────────────┐  ┌──────────────┐  │  │
│  │  │ Plot        │  │ Figure      │  │ DataBinder   │  │  │
│  │  │ (facade)    │  │ (container) │  │ (import API) │  │  │
│  │  └──────┬──────┘  └──────┬──────┘  └──────┬───────┘  │  │
│  │         │                │                │          │  │
│  ├─────────┼────────────────┼────────────────┼──────────┤  │
│  │         ▼                ▼                ▼          │  │
│  │  ┌──────────────────────────────────────────────┐   │  │
│  │  │            PLOT ENGINE (内部)                 │   │  │
│  │  │                                              │   │  │
│  │  │  ┌──────────┐ ┌────────┐ ┌───────────────┐  │   │  │
│  │  │  │ Axis     │ │ Layout │ │ PlotType      │  │   │  │
│  │  │  │ System   │ │ Engine │ │ Registry      │  │   │  │
│  │  │  │          │ │        │ │               │  │   │  │
│  │  │  │ • Linear │ │ • Grid │ │ • LinePlot    │  │   │  │
│  │  │  │ • Log    │ │ • Stack│ │ • ScatterPlot │  │   │  │
│  │  │  │ • Time   │ │ • Over │ │ • MultiAxis   │  │   │  │
│  │  │  │ • Polar  │ │ • ...  │ │ • (extensible)│  │   │  │
│  │  │  └──────────┘ └────────┘ └───────────────┘  │   │  │
│  │  │                                              │   │  │
│  │  │  ┌──────────────────────────────────────┐   │   │  │
│  │  │  │      Coordinate Transform Pipeline   │   │   │  │
│  │  │  │  DataSpace → ViewSpace → DeviceSpace │   │   │  │
│  │  │  └──────────────────────────────────────┘   │   │  │
│  │  └──────────────────┬───────────────────────────┘   │  │
│  │                     │                               │  │
│  │              ┌──────▼──────┐                        │  │
│  │              │ IRenderDevice│  ← 客户实现            │  │
│  │              │ (Abstract)  │                        │  │
│  │              └─────────────┘                        │  │
│  │              ┌─────────────┐                        │  │
│  │              │ IInputSource │  ← 客户实现 (可选)     │  │
│  │              │ (Abstract)  │                        │  │
│  │              └─────────────┘                        │  │
│  └─────────────────────────────────────────────────────┘  │
│                                                             │
├─────────────────────────────────────────────────────────────┤
│                   Optional: Reference Backend                │
│  ┌──────────────────────────────────────────────────────┐  │
│  │ Blend2DRenderDevice : IRenderDevice                  │  │
│  │ (用于测试、Demo、快速集成)                             │  │
│  └──────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

---

## 三、核心接口定义

### 3.1 IRenderDevice — 渲染抽象（★ 客户实现）

```cpp
// ==========================================
// 坐标与裁剪
// ==========================================
class IRenderDevice {
public:
    virtual ~IRenderDevice() = default;

    // --- 视口 & 裁剪 ---
    virtual void setViewport(int x, int y, int w, int h) = 0;
    virtual void setClipRect(double x, double y, double w, double h) = 0;
    virtual void resetClip() = 0;

    // --- 线段 & 折线 ---
    virtual void drawPolyline(const double* xs, const double* ys,
                              int count, const LineStyle& style) = 0;
    virtual void drawLine(double x1, double y1, double x2, double y2,
                          const LineStyle& style) = 0;

    // --- 标记点 ---
    virtual void drawMarkers(const double* xs, const double* ys,
                             int count, const MarkerStyle& style) = 0;

    // --- 填充区域 ---
    virtual void fillRect(double x, double y, double w, double h,
                          const FillStyle& style) = 0;
    virtual void fillPolygon(const double* xs, const double* ys,
                             int count, const FillStyle& style) = 0;

    // --- 文本 ---
    virtual void drawText(double x, double y, const char* text,
                          const FontDesc& font, const TextStyle& style) = 0;
    virtual void textExtent(const char* text, const FontDesc& font,
                            double* w, double* h) = 0;

    // --- 图像 (热力图等) ---
    virtual void drawImage(double x, double y, double w, double h,
                           const uint8_t* rgba, int imgW, int imgH) = 0;

    // --- 状态管理 ---
    virtual void beginFrame() = 0;
    virtual void endFrame() = 0;
};
```

**客户实现工作量估算**：~300-500 行 C++ 代码（以 Qt 为例，用 QPainter 实现上述接口）

### 3.2 IInputSource — 交互抽象（★ 客户实现，可选）

```cpp
// 统一事件结构
struct InputEvent {
    enum Type {
        MouseDown, MouseUp, MouseMove, MouseWheel,
        KeyDown, KeyUp,
        TouchBegin, TouchMove, TouchEnd
    };
    Type type;
    double x, y;       // 设备空间坐标
    int button;        // 0=left, 1=middle, 2=right
    int modifiers;     // bitmask: Shift=1, Ctrl=2, Alt=4
    double wheelDelta;
    int keyCode;
};

// 交互结果回调
struct InteractionResult {
    enum Action { ViewChanged, DataPicked, CurveSelected, None };
    Action action;
    // 根据 action 类型，不同字段有效
    double pickedDataX, pickedDataY;  // DataPicked
    int selectedCurveIndex;           // CurveSelected
};

// 客户实现此接口
class IInputSource {
public:
    virtual ~IInputSource() = default;
    virtual bool pollEvent(InputEvent& out) = 0; // 返回 false 表示无事件
};
```

### 3.3 SDK 公开 API（★ 我们实现）

```cpp
// ==========================================
// 主入口：Plot（门面）
// ==========================================
class Plot {
public:
    // --- 数据绑定 ---
    void addLineSeries(const char* name,
                       const double* xs, const double* ys, int count);
    void addScatterSeries(const char* name,
                          const double* xs, const double* ys, int count);
    void updateSeriesData(int seriesId,
                          const double* xs, const double* ys, int count);

    // --- 轴配置 ---
    void xAxisSetLabel(const char* label);
    void yAxisSetLabel(const char* label);
    void yAxisAddRight(const char* label);  // 多Y轴
    void xAxisSetScale(ScaleType type);     // Linear / Log / Time
    void setAxisRange(double xMin, double xMax,
                      double yMin, double yMax, int yAxisIndex = 0);

    // --- 渲染（核心调用）---
    void render(IRenderDevice& device);

    // --- 交互（可选）---
    InteractionResult handleEvent(const InputEvent& event,
                                   IRenderDevice* device = nullptr);
    // device 非空时，若有视图变化会自动触发重绘

    // --- 样式 ---
    void setTitle(const char* title);
    void setSeriesStyle(int seriesId, const LineStyle& style);

    // --- 导出（通过 IRenderDevice 的矢量后端）---
    // 客户提供 SVG/PDF 类型的 RenderDevice 即可实现导出
};

// ==========================================
// 数据表
// ==========================================
class DataTable {
public:
    static DataTable fromCSV(const char* filepath);
    static DataTable fromMemory(const double* data, int rows, int cols);

    int rowCount() const;
    int colCount() const;
    const double* column(int index) const;
    const char* columnName(int index) const;
};
```

---

## 四、渲染流程详解

### 4.1 一次 render() 调用的内部流程

```
Plot::render(device)
│
├─ 1. Layout Pass（纯计算，不调 device）
│   ├─ 计算标题区域 → titleRect
│   ├─ 计算轴标签区域 → axisLabelRects
│   ├─ 计算图例区域 → legendRect
│   └─ 计算绘图区 → plotArea (剩下的空间)
│
├─ 2. Axis Render Pass
│   ├─ 计算刻度值 (nice numbers algorithm)
│   ├─ 计算刻度位置
│   ├─ device.setClipRect(wholeArea)
│   ├─ device.drawLine(axisLine)
│   ├─ device.drawText(tickLabels)
│   └─ device.drawPolyline(gridLines)  // 虚线网格
│
├─ 3. Plot Data Pass
│   ├─ device.setClipRect(plotArea)
│   ├─ 对每个 series:
│   │   ├─ 坐标变换: DataSpace → DeviceSpace
│   │   ├─ device.drawPolyline(transformedPoints)   // Line
│   │   └─ device.drawMarkers(transformedPoints)    // Scatter
│   └─ device.resetClip()
│
├─ 4. Annotation Pass
│   ├─ device.drawText(titlePos, title)
│   ├─ device.drawText(axisLabels)
│   └─ renderLegend(device, seriesList)
│
└─ 5. 返回
```

### 4.2 坐标变换链

```cpp
struct TransformPipeline {
    // 数据空间 → 归一化空间 [0,1]
    double toNormalizedX(double dataX) const {
        return (dataX - xDataMin) / (xDataMax - xDataMin);
    }
    // 对数坐标:
    // double toNormalizedX(double dataX) const {
    //     return (log10(dataX) - log10(xDataMin)) /
    //            (log10(xDataMax) - log10(xDataMin));
    // }

    // 归一化空间 → 设备空间 (pixels)
    double toDeviceX(double normX) const {
        return plotLeft + normX * plotWidth;
    }
};
```

---

## 五、文件结构（SDK 交付物）

```
xyplot-sdk/
├── include/
│   └── xyplot/
│       ├── xyplot.h              ← 主头文件（客户只需 #include 这个）
│       ├── plot.h                ← Plot 门面类
│       ├── datatable.h           ← 数据表
│       ├── types.h               ← Point, Rect, Color, Style 等基础类型
│       ├── irender_device.h      ← ★ IRenderDevice 抽象接口
│       ├── iinput_source.h       ← ★ IInputSource 抽象接口
│       └── plot_types/
│           ├── iplot_type.h      ← 图类型插件接口
│           ├── line_plot.h       ← 折线图
│           ├── scatter_plot.h    ← 散点图
│           └── multi_axis.h      ← 多Y轴支持
│
├── src/
│   ├── plot.cpp
│   ├── datatable.cpp
│   ├── axis_system.cpp
│   ├── layout_engine.cpp
│   ├── coordinate_transform.cpp
│   ├── legend_renderer.cpp
│   └── plot_types/
│       ├── line_plot.cpp
│       ├── scatter_plot.cpp
│       └── multi_axis.cpp
│
├── backends/                     ← 参考后端（可选，非核心 SDK）
│   ├── recording/                ← Recording 后端（测试用）
│   │   └── recording_device.h
│   └── blend2d/                  ← Blend2D 后端（Demo 用）
│       └── blend2d_device.h
│
├── examples/
│   ├── minimal/                  ← 最小示例（伪设备）
│   ├── blend2d_demo/             ← Blend2D 渲染 Demo
│   └── qt_backend_example/       ← Qt 后端集成示例
│
├── tests/
│   ├── test_plot.cpp
│   ├── test_axis.cpp
│   ├── test_transform.cpp
│   └── test_render_integration.cpp
│
└── CMakeLists.txt
```

---

## 六、客户集成示例（以 Qt 为例）

```cpp
// ==========================================
// 客户代码：在自己的 Qt 应用中集成 XYPlot SDK
// ==========================================

#include <xyplot/xyplot.h>
#include <QPainter>
#include <QWidget>

// Step 1: 实现 IRenderDevice（~200 行）
class QtRenderDevice : public xyplot::IRenderDevice {
    QPainter* m_painter;
public:
    QtRenderDevice(QPainter* p) : m_painter(p) {}

    void setClipRect(double x, double y, double w, double h) override {
        m_painter->setClipRect(QRectF(x, y, w, h));
    }
    void drawPolyline(const double* xs, const double* ys,
                      int count, const LineStyle& s) override {
        QPolygonF poly;
        for (int i = 0; i < count; i++)
            poly << QPointF(xs[i], ys[i]);
        QPen pen(toQColor(s.color), s.width);
        m_painter->setPen(pen);
        m_painter->drawPolyline(poly);
    }
    void drawText(double x, double y, const char* text,
                  const FontDesc& f, const TextStyle& s) override {
        m_painter->setFont(toQFont(f));
        m_painter->drawText(QPointF(x, y), QString::fromUtf8(text));
    }
    // ... 其余 ~15 个方法
};

// Step 2: 在自己的 Widget 中使用
class MySimulationView : public QWidget {
    xyplot::Plot m_plot;
    QtRenderDevice m_device{nullptr};

    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);
        m_device = QtRenderDevice(&painter);

        // 更新数据（仿真每步计算后）
        m_plot.updateSeriesData(0, xs, ys, N);

        // 渲染！
        m_plot.render(m_device);
    }

    void mousePressEvent(QMouseEvent* e) override {
        InputEvent ev = toInputEvent(e);  // 客户写转换函数
        auto result = m_plot.handleEvent(ev, &m_device);
        if (result.action == InteractionResult::CurveSelected) {
            // 高亮选中的曲线...
        }
    }
};
```

---

## 七、技术指标

| 指标 | 目标值 | 说明 |
|------|--------|------|
| SDK 核心代码量 | < 5000 行 | 不含注释和测试 |
| 外部强制依赖 | 0 | 核心仅依赖 C++17 标准库 |
| IRenderDevice 方法数 | ~20 | 客户实现工作量 ~300 行 |
| 头文件数 | < 10 | include/ 下 |
| 编译单元 | < 15 个 .cpp | src/ 下 |
| ABI 稳定性 | C API 包装 | 便于不同编译器混用 |
| 坐标变换精度 | double (64-bit) | 满足科学计算需求 |
| 1M 点渲染延迟 | < 100ms (变换+调用) | 不含客户渲染耗时 |

---

## 八、扩展路径

```
Phase 1 (本周 MVP):
  IRenderDevice + IInputSource + Line + Scatter + Multi-Y-Axis

Phase 2 (未来):
  + Bar, Area, Histogram, ErrorBar, Step, Polar → 新增 IPlotType 子类
  + 3D Surface, 3D Scatter → 扩展 IRenderDevice (drawTriangle, ...)
  + Python 绑定 → pybind11 包装 Plot 类

Phase 3 (远期):
  + Smith, Bode, Nyquist, Eye → 专用 IPlotType 插件
  + 流式数据管道 → DataTable::append() + 增量渲染
  + GPU 加速后端 → 客户用 OpenGL 实现 IRenderDevice
```
