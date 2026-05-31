# 接口冻结清单 — IRenderDevice & IInputSource

**文档日期**：2026-05-31
**版本**：v1.0-frozen
**冻结范围**：P0 MVP
**变更规则**：冻结后只增不删；新增方法必须有默认实现（非纯虚），确保已有客户代码不 broken

---

## 一、接口最小化检查结果

原始方案中 IRenderDevice 约 20 个方法。经 P0 必要性分析后：

| 分类 | 数量 | 说明 |
|------|------|------|
| **P0 必需** | **8** | 客户必须实现，Line + Scatter + MultiAxis 渲染依赖 |
| **P0 可降级** | **1** | SDK 提供估算回退，客户可选实现以获得更好效果 |
| **延期至 P1+** | **11** | 非 P0 图类型需要，不进入本次冻结 |

方法数从 20 → 8+1，**缩减 55%**。

---

## 二、P0 必需接口（8 个 — 客户必须实现）

```cpp
// ==========================================
// 文件: include/xyplot/irender_device.h
// 冻结版本: v1.0-frozen
// ==========================================

class IRenderDevice {
public:
    virtual ~IRenderDevice() = default;

    // ──── 帧生命周期 (2) ────
    // 1
    virtual void beginFrame() = 0;
    // 2
    virtual void endFrame() = 0;

    // ──── 裁剪 (2) ────
    // 3
    virtual void setClipRect(double x, double y, double w, double h) = 0;
    // 4
    virtual void resetClip() = 0;

    // ──── 线段/折线 (1) ────
    // 5  覆盖: 曲线、坐标轴、网格线、图例线段
    virtual void drawPolyline(const double* xs, const double* ys,
                              int count, const LineStyle& style) = 0;

    // ──── 标记点 (1) ────
    // 6  覆盖: 散点图、数据点高亮
    virtual void drawMarkers(const double* xs, const double* ys,
                             int count, const MarkerStyle& style) = 0;

    // ──── 文本 (1) ────
    // 7  覆盖: 标题、轴标签、刻度标签、图例文本
    virtual void drawText(double x, double y, const char* text,
                          const FontDesc& font, const TextStyle& style) = 0;

    // ──── 填充 (1) ────
    // 8  覆盖: 图例色块
    virtual void fillRect(double x, double y, double w, double h,
                          const FillStyle& style) = 0;
};
```

### P0 必需接口覆盖矩阵

| 渲染需求 | 使用接口 | 
|----------|---------|
| 折线图曲线 | `drawPolyline` |
| 散点图标记 | `drawMarkers` |
| 坐标轴线 | `drawPolyline` |
| 网格线 (虚线) | `drawPolyline` (LineStyle::dash) |
| 刻度标签 | `drawText` |
| 轴标题 | `drawText` |
| 图表标题 | `drawText` |
| 图例文本 | `drawText` |
| 图例色块 | `fillRect` |
| 绘图区裁剪 | `setClipRect` / `resetClip` |

✅ 8 个方法覆盖 P0 全部渲染需求。

---

## 三、P0 可降级接口（1 个 — 客户可选实现）

```cpp
class IRenderDevice {
public:
    // ... P0 必需接口 ...

    // ──── 文本度量 (P0 可降级) ────
    // SDK 内置估算回退 (strlen * fontSize * 0.6)
    // 客户实现此方法可获得精确的文本布局
    virtual void textExtent(const char* text, const FontDesc& font,
                            double* w, double* h) {
        // 默认估算（保守偏大）
        double approxCharWidth = font.size * 0.6;
        *w = strlen(text) * approxCharWidth;
        *h = font.size * 1.25;
    }
};
```

**为什么是可降级？** 
- 布局引擎用 `textExtent` 计算文本占位
- 估算值虽不精确但不会导致渲染错误（仅文本位置可能略有偏移）
- 客户追求像素级精度时可覆盖此方法
- 不实现也不影响功能正确性

---

## 四、延期至 P1+ 的接口（不进入本次冻结）

以下方法在 P0 场景下不需要，标记为"延期"，不在 v1.0-frozen 范围内：

| 方法 | 需要它的图类型 | 计划引入版本 |
|------|--------------|-------------|
| `drawLine(x1,y1,x2,y2)` | 无 — `drawPolyline` 可替代 | 永不（语法糖） |
| `fillPolygon(xs,ys,count)` | Area Plot, 误差棒填充 | P1 (V1.0) |
| `drawImage(x,y,w,h,rgba)` | Heatmap, Contour, 3D texture | P1 (V1.0) |
| `drawTriangle*` (3D 面片) | 3D Surface, 3D Mesh | P2 (V2.0) |
| `setTransform(matrix)` | 3D 旋转/缩放 | P2 (V2.0) |
| `drawGradient*` | 高级填充 | P2 (V2.0) |
| `setAntiAlias(bool)` | 全局渲染质量 | P2 (V2.0) |
| `drawPath` | 自定义形状 | P2 (V2.0) |
| `drawGlyph` (3D) | 3D 矢量场 | P2 (V2.0) |
| `setLighting` | 3D 光照 | P2 (V2.0) |
| `drawColorBar` | 热力图色条 | P1 (V1.0) |

**当需要引入上述方法时**：新增为带默认空实现的虚方法（非纯虚），确保已有客户代码编译不受影响。

---

## 五、IInputSource — 冻结接口

```cpp
// ==========================================
// 文件: include/xyplot/iinput_source.h
// 冻结版本: v1.0-frozen
// ==========================================

struct InputEvent {
    enum Type {
        None = 0,
        MouseDown, MouseUp, MouseMove, MouseWheel,
        KeyDown, KeyUp,
        TouchBegin, TouchMove, TouchEnd
    };
    Type type = None;
    double x = 0, y = 0;      // 设备空间坐标
    int button = 0;           // 0=left, 1=middle, 2=right
    int modifiers = 0;        // bitmask: Shift=1, Ctrl=2, Alt=4
    double wheelDelta = 0;
    int keyCode = 0;
};

class IInputSource {
public:
    virtual ~IInputSource() = default;

    // 返回 true 表示有事件可读取（事件写入 out）
    // 返回 false 表示无事件
    // 客户可选择轮询模式或 push 模式（SDK 也提供 push 重载）
    virtual bool pollEvent(InputEvent& out) = 0;
};
```

**IInputSource 为完全可选**。客户不实现 IInputSource 时，SDK 仅提供渲染能力（Plot::render），不提供交互。这对"仅需出图"的场景是最简接入方式。

---

## 六、支持类型（冻结）

```cpp
// ==========================================
// 文件: include/xyplot/types.h
// 冻结版本: v1.0-frozen
// ==========================================

// 颜色
struct Color {
    uint8_t r = 0, g = 0, b = 0, a = 255;
    static Color fromHex(uint32_t hex);  // e.g., 0xFF0000 → red
};

// 线段样式
struct LineStyle {
    double width = 1.0;
    Color color{};
    enum DashStyle { SolidLine, DashLine, DotLine, DashDotLine };
    DashStyle dash = SolidLine;
};

// 标记样式
struct MarkerStyle {
    enum Shape { Circle, Square, Diamond, Triangle, Cross, Plus };
    Shape shape = Circle;
    double size = 6.0;
    Color fillColor = {0,0,0};
    Color edgeColor = {0,0,0};
    double edgeWidth = 1.0;
};

// 填充样式
struct FillStyle {
    Color color = {200,200,200};
};

// 字体描述
struct FontDesc {
    double size = 12.0;    // 字号（设备空间像素）
    bool bold = false;
    bool italic = false;
};

// 文本样式
struct TextStyle {
    Color color = {0,0,0};
    enum Align { Left, Center, Right };
    Align hAlign = Left;
    enum VAlign { Top, Middle, Bottom };
    VAlign vAlign = Bottom;
};

// 坐标轴刻度类型
enum class ScaleType { Linear, Log10, Ln };

// 点（数据空间）
struct DataPoint { double x = 0, y = 0; };

// 矩形（设备空间）
struct Rect { double x = 0, y = 0, w = 0, h = 0; };
```

---

## 七、变更控制规则

| 规则 | 内容 |
|------|------|
| **冻结期内** (Day 1-5) | 只允许新增带默认实现的虚方法；禁止删除/修改已有方法签名 |
| **冻结期内** | types.h 字段只增不删，新增字段必须有默认值 |
| **冻结期后** (V1.0+) | 破坏性变更需 PO 审批 + 跨版本过渡期（旧方法标记 deprecated 保留 1 个版本） |
| **任何人** | 修改 `irender_device.h` 必须同步更新 `test_interface_contract.cpp` |

---

## 八、冻结签字

| 角色 | 签字 | 日期 |
|------|------|------|
| Project Lead | ___________ | 2026-05-31 |
| PO | ___________ | 2026-05-31 |

> PO 签署即表示：此 8+1 接口定义满足 P0 MVP 需求，开发团队可按此接口并行工作。
