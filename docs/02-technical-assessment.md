# 技术评估：后端无关的 SDK 架构 — 选型分析

**项目**：XY Plot 画图 SDK
**文档日期**：2026-05-31
**版本**：v2.0（根据 PO 反馈重写）
**作者**：Project Lead
**前置依赖**：[01 PO 回复 Q4](./01-requirements-clarification.md#三po-回复记录--影响分析) — "交互库不可控，客户系统使用什么显示，我们不知道"

---

## 一、核心约束与评估维度转移

### 1.1 Q4 带来的架构约束

```
PO 回复: "交互库不可控，因为我们是产品卖出去的。
          客户系统使用什么显示，我们不知道的。"

解读:
  ✗ 不能依赖 Qt
  ✗ 不能依赖 VTK
  ✗ 不能依赖任何特定 UI 框架
  ✓ SDK 核心 = 纯计算 + 抽象接口
  ✓ 提供参考后端（用于测试和 Demo）
  ✓ 客户适配自己的显示系统
```

### 1.2 新评估维度

| 维度 | 权重 | 说明 |
|------|------|------|
| **抽象接口设计** | 30% | 渲染接口的抽象层次是否合理、完整、可扩展 |
| **参考后端可行性** | 25% | 用于测试/Demo 的参考实现用什么库 |
| **集成友好度** | 25% | API 设计对客户是否友好、依赖最小化 |
| **性能路径** | 15% | 大数据 + 实时场景下的数据流设计 |
| **许可证合规** | 5% | 商用友好 |

---

## 二、关键设计决策 #1：渲染抽象的层级

### 2.1 三种抽象层级对比

```
高层抽象 (Chart-level)
┌─────────────────────────────────┐
│ IRenderer::drawLineChart(       │  ← 客户只需实现几个方法
│   points, style, axes)          │
│ IRenderer::drawScatter(...)     │    但新图类型需要改接口
│ IRenderer::drawBarChart(...)    │    → 不够灵活，否决 ❌
└─────────────────────────────────┘

中层抽象 (Primitive-level) ★推荐
┌─────────────────────────────────┐
│ IRenderDevice::drawPolyline(...)│  ← 客户实现 ~20 个图元方法
│ IRenderDevice::drawMarker(...)  │
│ IRenderDevice::drawText(...)    │    图类型可自由组合图元
│ IRenderDevice::fillRect(...)    │    → 灵活且可管理 ✓
│ IRenderDevice::setClipRect(...) │
│ ...                             │
└─────────────────────────────────┘

低层抽象 (Pixel-level)
┌─────────────────────────────────┐
│ IRenderDevice::setPixel(x,y,c)  │  ← 调用次数太多，性能差
│ IRenderDevice::getScanline(...) │    文本渲染极难实现
│ ...                             │    → 太底层，否决 ❌
└─────────────────────────────────┘
```

### 2.2 结论：采用中层图元抽象

```
✓ 图元数量可控（~20 个方法）
✓ 客户对接工作量合理（约 300-500 行代码）
✓ 新图类型不需要改接口
✓ 性能：一次 drawPolyline(1M点) 而非 1M 次 setPixel()
```

---

## 三、关键设计决策 #2：几何模型

### 3.1 坐标系设计

```
数据空间           变换             设备空间
(Data Space)  ──────────────►  (Device Space)
                               
(1.5, 3.2e6)    AxesTransform    (437, 218)
(对数坐标)      + ViewTransform   (像素坐标)
```

SDK 内部职责：**数据空间 → 设备空间**的坐标变换。
客户只需告诉我们设备空间的像素范围。

### 3.2 渲染命令模型

```cpp
// SDK 产生渲染命令，调用客户提供的 IRenderDevice
class PlotEngine {
    void render(IRenderDevice& device, const ViewState& view) {
        // 1. 计算变换
        auto transform = computeTransform(dataBounds, view);

        // 2. 调用图元
        device.setClipRect(view.plotArea);
        device.drawPolyline(transform.map(lineData), lineStyle);
        device.drawMarkers(transform.map(scatterData), markerStyle);
        device.drawText(transform.map(labelPos), "Label", fontDesc);
    }
};
```

---

## 四、关键设计决策 #3：参考后端选择

参考后端用于：
1. SDK 自身的单元测试（验证渲染输出）
2. Demo 应用（快速看到效果）
3. 客户集成的参考代码

### 4.1 参考后端候选

| 库 | 类型 | 许可证 | 体积 | 优缺点 |
|----|------|--------|------|--------|
| **AGG** | 2D 软件渲染 | BSD | ~300KB | 经典抗锯齿，不再维护但稳定 |
| **Blend2D** | 2D 高性能 | Zlib | ~2MB | 现代设计，SIMD 优化，活跃维护 |
| **NanoVG** | 2D (OpenGL) | Zlib | ~50KB | 极轻，但依赖 OpenGL 上下文 |
| **Cairo** | 2D 矢量 | LGPL/MPL | ~2MB | 成熟，PDF/SVG/PNG 多后端 |
| **Skia** | 2D 矢量 | BSD | ~20MB | 最强大，Chrome/Android 在用，但重 |
| **Null/Recording** | 仅记录调用 | 自研 | ~5KB | 仅用于测试，无实际显示 |

### 4.2 推荐：Blend2D + Recording 双模式

| 模式 | 用途 | 后端 |
|------|------|------|
| **Recording** | 单元测试、CI | 自研（记录所有调用及参数，验证正确性） |
| **Blend2D** | Demo、客户参考 | 高性能 2D 软件渲染，zlib 许可证商用友好 |

### 4.3 为什么不是 AGG？

虽然 AGG 是经典的 2D 渲染库，但：
- 2006 年后未更新，C++98 风格
- 与现代 C++17/20 集成有摩擦
- Blend2D 在 API 设计、性能、活跃度上均优于 AGG

### 4.4 为什么不是 Skia？

Skia 功能最全，但：
- 构建系统极其复杂（GN + Ninja）
- 二进制体积大（~20MB+）
- 对于 SDK 的参考后端来说过重
- 客户集成时会面对同样的构建复杂度

---

## 五、关键设计决策 #4：交互抽象

### 5.1 输入事件模型

```cpp
// 客户将原始事件转换为我们的统一事件结构
struct InputEvent {
    enum Type { MouseDown, MouseUp, MouseMove,
                MouseWheel, KeyDown, KeyUp,
                TouchBegin, TouchMove, TouchEnd };

    Type type;
    double x, y;          // 设备空间坐标
    int button;           // Mouse button
    int modifiers;        // Shift/Ctrl/Alt
    double wheelDelta;
    int keyCode;
};

// 客户实现此接口上报事件
class IInputSource {
public:
    virtual ~IInputSource() = default;
    // 由客户主动调用，或 SDK 提供回调注册
    virtual void pollEvents(std::vector<InputEvent>& out) = 0;
};
```

### 5.2 交互处理分离

```
客户UI线程                    SDK内部                 客户RenderDevice
    │                          │                         │
    ├─ 捕获原生鼠标事件         │                         │
    ├─ 转为 InputEvent ──────►│                         │
    │                          ├─ HitTest               │
    │                          ├─ 更新 ViewState        │
    │                          ├─ 确定交互目标          │
    │                          ├─ requestRedraw() ─────►│
    │                          │                         ├─ 重绘
    │◄──── 交互结果回调 ───────┤                         │
    │                          │                         │
```

---

## 六、许可证合规路径

基于 Q11（需卖出去），依赖库许可证审查：

| 组件 | 许可证 | 商用合规 | 注意 |
|------|--------|---------|------|
| SDK 核心 | 自研 | ✅ | 闭源商用无问题 |
| Blend2D (参考后端) | Zlib | ✅ | 非常宽松，可静态链接 |
| 抽象接口头文件 | BSD/MIT | ✅ | 客户可自由使用 |
| **禁止** | GPL | ❌ | 不可引入任何 GPL 依赖 |

> 参考后端仅用于 Demo 和测试。客户生产环境使用自己的渲染后端，不依赖 Blend2D。SDK 核心零外部依赖。

---

## 七、对标验证：类似架构的成熟案例

| 产品 | 架构模式 | 说明 |
|------|---------|------|
| **matplotlib** | 后端抽象 | Python，但概念一致：Agg/Cairo/PDF/SVG 等多后端 |
| **plotly** | 声明式 JSON → 渲染器 | 数据模型与渲染分离 |
| **TeeChart** | 原生控件 + 多种后端 | 商业图表库，支持 VCL/FMX/.NET 多框架 |
| **LightningChart** | 自研 GPU 渲染 | 商业科学图表库，独立渲染引擎 |
| **GR Framework** | 抽象 GKS 标准 | 德国学术科学可视化框架，后端无关 |

---

## 八、总结：技术决策清单

| # | 决策项 | 选择 | 理由 |
|---|--------|------|------|
| D1 | 抽象层级 | 中层图元抽象 | 灵活性与实现成本最佳平衡 |
| D2 | 几何模型 | 坐标变换 + 命令式渲染 | 标准科学可视化范式 |
| D3 | 参考后端 | Blend2D + Recording | 轻量、商用友好、高性能 |
| D4 | 交互模型 | 统一事件 + 轮询/回调 | 最小化对客户事件系统的侵入 |
| D5 | 许可证策略 | BSD + Zlib | 商用友好，零合规风险 |
| D6 | SDK 依赖 | 零外部强制依赖 | 核心引擎 header-only + 单 .cpp 编译单元 |
