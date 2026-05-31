# 执行路线图：XY Plot SDK — 1 周冲刺计划

**项目**：XY Plot 画图 SDK
**文档日期**：2026-05-31
**版本**：v2.0（根据 PO 反馈重写 — 1 周时间线 + AI Agent 执行）
**作者**：Project Lead
**前置依赖**：[03 方案建议 v2](./03-solution-proposal.md)

---

## 一、总览

```
Week 1:  5 个工作日，4 个阶段
────────────────────────────────────────────────────────────────

  Day 1        Day 2         Day 3         Day 4         Day 5
  │            │             │             │             │
  ├─ Phase 0 ─┤             │             │             │
  │  架构搭建  │             │             │             │
  │            ├─ Phase 1 ──┤             │             │
  │            │  核心引擎   │             │             │
  │            │             ├─ Phase 2 ──┤             │
  │            │             │  交互+导出  │             │
  │            │             │             ├─ Phase 3 ──┤
  │            │             │             │  打磨交付   │
  │            │             │             │             │
  ▼            ▼             ▼             ▼             ▼
 接口定稿      引擎完成       功能完整       第一个版本发布
 CMake就绪     Line可渲染     Scatter可用   文档+示例就绪
 测试框架      坐标变换OK     多Y轴OK       代码审查通过
```

| 阶段 | 工作日 | 主题 | 交付物 |
|------|--------|------|--------|
| **Phase 0** | Day 1 | 架构搭建 | 接口定义 + CMake + 测试框架 |
| **Phase 1** | Day 2-3 | 核心引擎 | Layout + Axis + Line Plot + Scatter |
| **Phase 2** | Day 3-4 | 交互 + 多轴 | Multi-Axis + Input + Recording 后端 |
| **Phase 3** | Day 5 | 打磨交付 | 文档 + 示例 + 审查 + 发布 |

---

## 二、Phase 0 — Day 1：架构搭建

### 目标：所有接口定稿，项目骨架可编译，CI 就绪

### 任务拆解（可并行）

```
Task 0.1 ─── 类型系统定义 (types.h)
├─ Point, Rect, Color, LineStyle, MarkerStyle, FillStyle, FontDesc
├─ 纯数据结构，无依赖
└─ 产出: include/xyplot/types.h (~100 行)

Task 0.2 ─── IRenderDevice 接口定稿
├─ ~20 个纯虚方法
├─ 充分的文档注释
└─ 产出: include/xyplot/irender_device.h (~80 行)

Task 0.3 ─── IInputSource 接口定稿
├─ InputEvent 结构体
├─ IInputSource::pollEvent()
└─ 产出: include/xyplot/iinput_source.h (~50 行)

Task 0.4 ─── CMake 项目骨架
├─ CMakeLists.txt (顶层 + src/ + tests/ + examples/)
├─ C++17 标准，警告全开
└─ 产出: CMakeLists.txt (~60 行)

Task 0.5 ─── 测试框架接入
├─ Catch2 (header-only) 或 Google Test
├─ 第一个编译测试: #include <xyplot/xyplot.h>
└─ 产出: tests/CMakeLists.txt + tests/test_smoke.cpp

Task 0.6 ─── 主头文件
├─ xyplot.h (include-all)
└─ 产出: include/xyplot/xyplot.h
```

### Day 1 验收标准

| 条目 | 标准 |
|------|------|
| 编译 | `cmake --build .` 在 Windows + Linux 均通过 |
| 接口 | IRenderDevice / IInputSource 冻结（后续只增不减） |
| 测试 | 至少 1 个编译测试通过 |

---

## 三、Phase 1 — Day 2-3：核心引擎

### 目标：Plot + Line + Scatter 可渲染，坐标变换正确

### Day 2 任务

```
Task 1.1 ─── DataTable 实现
├─ 内部存储 (column-major double array)
├─ fromMemory() 静态工厂
├─ column(), columnName(), rowCount(), colCount()
└─ 产出: datatable.h + datatable.cpp (~200 行)

Task 1.2 ─── 坐标变换管道
├─ LinearScale, LogScale
├─ DataSpace → NormalizedSpace → DeviceSpace
├─ 支持轴范围反转 (xMax < xMin)
└─ 产出: coordinate_transform.h/.cpp (~250 行)

Task 1.3 ─── Axis System (核心算法)
├─ Nice Number 算法 (自动刻度)
│   输入: range [0.3, 8.7]
│   输出: ticks = {0, 2, 4, 6, 8, 10}
├─ 刻度标签格式化 (1e6 → "1M", 0.001 → "1m")
├─ 网格线生成
└─ 产出: axis_system.h/.cpp (~300 行)

Task 1.4 ─── Layout Engine
├─ 给定 IRenderDevice + 配置 → 计算各区域矩形
├─ 标题区、轴标签区、图例区、绘图区
├─ 响应式：根据可用空间自动调整
└─ 产出: layout_engine.h/.cpp (~200 行)
```

### Day 3 任务

```
Task 1.5 ─── IPlotType 接口 + Line Plot
├─ IPlotType 抽象接口
├─ LinePlot 实现:
│   ├─ 接收 DataTable + 列索引
│   ├─ 通过坐标变换 → 调用 device.drawPolyline()
│   └─ 支持 LineStyle (颜色、线宽、虚线)
└─ 产出: iplot_type.h + line_plot.h/.cpp (~250 行)

Task 1.6 ─── Scatter Plot
├─ ScatterPlot 实现:
│   ├─ 接收 DataTable + 列索引
│   ├─ 调用 device.drawMarkers()
│   └─ 支持 MarkerStyle (形状、大小、颜色)
└─ 产出: scatter_plot.h/.cpp (~200 行)

Task 1.7 ─── Plot 门面类 (MVP)
├─ addLineSeries() / addScatterSeries()
├─ setAxisRange() / setAxisLabel()
├─ render(IRenderDevice&) — 串联 Layout + Axis + PlotTypes
└─ 产出: plot.h + plot.cpp (~300 行)

Task 1.8 ─── Recording Render Device
├─ 实现 IRenderDevice，记录所有调用
├─ 用于单元测试验证渲染输出
└─ 产出: backends/recording/recording_device.h (~150 行)
```

### Phase 1 验收标准

| 条目 | 标准 |
|------|------|
| 编译 | 全部源码编译通过 |
| 渲染 | `Plot::render(recordingDevice)` 不崩溃，产生预期调用序列 |
| 坐标 | 单元测试：已知数据 → 已知像素位置 |
| 刻度 | 单元测试：NiceNumber(0.3, 8.7) = {0,2,4,6,8,10} |
| 布局 | 单元测试：给定 800×600 设备，绘图区非零面积 |

---

## 四、Phase 2 — Day 3-4：交互 + 多轴

### 目标：Multi-Y-Axis 可用，交互逻辑完成，第2个后端上线

### 任务

```
Task 2.1 ─── Multi-Y-Axis 支持
├─ 右Y轴独立刻度
├─ 每条 series 绑定到左或右轴
├─ 右轴刻度线绘制在右侧
└─ 产出: multi_axis.h/.cpp (~200 行)

Task 2.2 ─── HitTest 系统
├─ 给定设备坐标 (x,y) → 找到最近的 series/data point
├─ 判断是否点击在绘图区/图例/轴上
├─ 返回 HitTestResult
└─ 产出: hit_test.h/.cpp (~200 行)

Task 2.3 ─── Interaction Handler
├─ Plot::handleEvent(InputEvent) → InteractionResult
├─ 支持: 曲线选择 (点击图例或曲线)
├─ 预留: 缩放、平移、数据点拾取
├─ 状态机: Idle → Hovering → Selected
└─ 产出: interaction_handler.h/.cpp (~250 行)

Task 2.4 ─── 图例渲染
├─ 自动根据 series 名称生成图例
├─ 支持点击选择/取消选择
├─ 图例项含颜色标记 + 名称文本
└─ 产出: legend_renderer.h/.cpp (~200 行)

Task 2.5 ─── Blend2D 参考后端 (可选加速项)
├─ 实现 IRenderDevice 基于 Blend2D
├─ 实际可见的像素输出
├─ 用于 Demo
└─ 产出: backends/blend2d/blend2d_device.h/.cpp (~300 行)
```

### Phase 2 验收标准

| 条目 | 标准 |
|------|------|
| 多Y轴 | 左右Y轴不同刻度，各自系列的线用各自轴 |
| 点击选择 | 点击图例中的曲线名 → handleEvent 返回 CurveSelected |
| 单元测试 | HitTest 覆盖: 点击在曲线上/在空白区/在图例上 |

---

## 五、Phase 3 — Day 5：打磨交付

### 目标：文档、示例、代码审查、发布就绪

### 任务

```
Task 3.1 ─── API 文档
├─ README.md (项目概述 + 快速开始)
├─ API_REFERENCE.md (每个公开类的文档)
├─ 头文件中的 Doxygen 注释
└─ 产出: README.md + docs/API_REFERENCE.md

Task 3.2 ─── 最小示例 (pseudo-device)
├─ 不依赖任何外部库
├─ 展示完整流程: 创建数据 → 配置 Plot → 渲染
├─ 仅用 "recording" 后端打印调用序列
└─ 产出: examples/minimal/main.cpp (~60 行)

Task 3.3 ─── Qt 后端集成示例
├─ 完整的 QWidget 子类示例
├─ QtRenderDevice 完整实现
├─ 交互事件转发
└─ 产出: examples/qt_backend/main.cpp + qt_device.h (~250 行)

Task 3.4 ─── 测试完善
├─ Plot 集成测试 (完整 render 流程)
├─ Axis 边界测试 (负数、零、大数、对数)
├─ 多Y轴集成测试
├─ 坐标变换精度测试
└─ 产出: tests/ 下 15+ 测试用例

Task 3.5 ─── 代码审查 + 清理
├─ 统一命名规范
├─ const 正确性
├─ 内存安全审查 (valgrind / AddressSanitizer)
├─ 头文件包含依赖最小化
└─ 产出: 审查报告
```

### Phase 3 验收标准 (最终交付)

| 条目 | 标准 |
|------|------|
| 编译 | Windows (MSVC) + Linux (GCC) 零警告 |
| 测试 | `ctest` 全部通过，覆盖率 >70% |
| 文档 | README + API 参考 + 2 个可编译运行的示例 |
| 内存 | AddressSanitizer 零报错 |
| 接口 | IRenderDevice 18-22 个方法，完整文档注释 |
| SDK 体积 | include/ < 10 个文件，src/ < 15 个文件，总计 < 5000 行 |

---

## 六、AI Agent 执行策略

### 6.1 Agent 分工（Day 1 即可启动的并行任务）

```
Agent Pool (4-6 个 Agent 并行)

Agent A ─── 接口设计师
  ├─ types.h (数据结构)
  ├─ irender_device.h (渲染接口)
  ├─ iinput_source.h (交互接口)
  └─ iplot_type.h (图类型接口)

Agent B ─── 基础设施工程师
  ├─ CMakeLists.txt (构建系统)
  ├─ 测试框架接入
  └─ CI 配置

Agent C ─── 核心算法工程师
  ├─ Axis System (刻度算法)
  ├─ Coordinate Transform (变换管道)
  └─ Layout Engine (布局计算)

Agent D ─── 图类型开发者
  ├─ Line Plot
  ├─ Scatter Plot
  └─ Multi-Y-Axis

Agent E ─── 参考后端 + 集成
  ├─ Recording Device
  ├─ Blend2D Device
  └─ Plot 门面类 + 集成测试

Agent F ─── 文档 + 示例 (Day 4-5 加入)
  ├─ README.md
  ├─ API_REFERENCE.md
  ├─ examples/
  └─ 代码审查
```

### 6.2 每日节奏

```
Daily:
  09:00 — 15min Standup (检查进度，识别阻塞)
  09:15-12:00 — Agent 并行工作
  12:00 — 编译检查 (所有 Agent 产出合并到 main)
  13:00-17:00 — Agent 并行工作
  17:00 — 集成测试 (确保各模块可联编)
  17:30 — 日总结 (更新 Roadmap 进度)

Day 1 重点: 接口冻结 (所有 .h 文件定稿)
Day 2 重点: 引擎跑通 (Line Plot 首次像素级渲染)
Day 3 重点: 功能完整 (所有 P0 图类型可用)
Day 4 重点: 稳定 + 测试 (覆盖率攻关)
Day 5 重点: 文档 + 审查 (交付就绪)
```

---

## 七、风险 & 缓解（1 周特化版）

| 风险 | 概率 | 缓解 |
|------|------|------|
| 接口设计争议大，Day 1 未冻结 | 中 | Project Lead 有最终决策权；接口遵循"最小必要"原则 |
| 坐标变换精度问题 | 低 | Day 2 即写变换测试，发现立即修 |
| Nice Number 算法边界 case 多 | 中 | 参考成熟实现 (matplotlib ticker)，先覆盖主路径 |
| Agent 间集成冲突 | 高 | 每日 2 次强制合并 + 编译检查 |
| 时间不够 | 高 | Day 3 晚上做 cut：砍 Blend2D 后端，保留 Recording |
| Blend2D 编译不过 | 中 | 降级为纯头文件 stub，可后续补 |

---

## 八、交付物清单

| # | 交付物 | 文件 | 负责 Agent |
|---|--------|------|-----------|
| 1 | 公开头文件 | `include/xyplot/*.h` | Agent A |
| 2 | 源文件 | `src/*.cpp` | Agent B, C, D |
| 3 | 构建系统 | `CMakeLists.txt` | Agent B |
| 4 | 单元测试 | `tests/*.cpp` | Agent E |
| 5 | Recording 后端 | `backends/recording/` | Agent E |
| 6 | Blend2D 后端 (可选) | `backends/blend2d/` | Agent E |
| 7 | 最小示例 | `examples/minimal/` | Agent F |
| 8 | Qt 集成示例 | `examples/qt_backend/` | Agent F |
| 9 | API 文档 | `README.md` + `docs/API_REFERENCE.md` | Agent F |
| 10 | 审查报告 | `docs/review-report.md` | Project Lead |

---

## 九、里程碑进度条（面向 PO 汇报用）

```
Phase 0 (Day 1):    ████████░░░░░░░░░░░░  接口冻结 + CMake + 测试框架
Phase 1 (Day 2-3):  ░░░░░░░░████████░░░░  Line + Scatter + Axis + Layout
Phase 2 (Day 3-4):  ░░░░░░░░░░░░░░████░░  MultiAxis + HitTest + Recording
Phase 3 (Day 5):    ░░░░░░░░░░░░░░░░░░██  文档 + 示例 + 审查 + 发布

每个 Phase 完成时向 PO 汇报进度。
```

---

> **PO 请注意**：
> 1. 以上为 1 周内交付的 MVP（P0 范围：Line + Scatter + Multi-Y-Axis）
> 2. P1 (Bar, Area, Histogram, ErrorBar, Step, Polar) 和 P2 (3D, Smith, Bode...) 将通过新增 IPlotType 子类渐进交付
> 3. 每天 17:30 可通过日总结了解进度
