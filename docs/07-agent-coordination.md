# 6 Agent 并行协作方案

**文档日期**：2026-05-31
**版本**：v1.0
**状态**：Full Go — 执行中
**前置依赖**：[04-roadmap.md](./04-roadmap.md) | [interface-freeze.md](./interface-freeze.md)

---

## 一、协作模型总览

```
                     Project Lead (你)
                           │
              ┌────────────┼────────────┐
              │            │            │
          审核接口      解决冲突      每日汇报
              │            │            │
    ┌─────────┴─────────┐  │  ┌─────────┴─────────┐
    │                   │  │  │                   │
    ▼                   ▼  ▼  ▼                   ▼
  Agent A   Agent B   Agent C   Agent D   Agent E   Agent F
 (接口守护) (基础设施) (核心算法) (图类型)  (后端集成) (文档示例)
```

**核心原则**：6 个 Agent 是 6 个独立的 Claude Code session，各自在自己的文件域内工作。通过文件所有权隔离冲突，通过 gate-check 强制质量。

---

## 二、文件所有权矩阵

这是最关键的协调机制——**每个文件只有一个 Owner，其他人只读**。

### 2.1 头文件 (include/xyplot/) — 冻结区

| 文件 | Owner | 状态 | 其他 Agent 权限 |
|------|-------|------|----------------|
| `types.h` | **Agent A** | 🔒 冻结 | 只读。需变更 → 提 issue → Agent A 评估 → Project Lead 审批 |
| `irender_device.h` | **Agent A** | 🔒 冻结 | 只读。变更触发契约测试失败，必须同步更新 |
| `iinput_source.h` | **Agent A** | 🔒 冻结 | 只读 |
| `plot.h` | **Agent A** | 🔒 冻结 | 只读。新增方法可，但不可改已有签名 |
| `xyplot.h` | **Agent A** | 🔒 冻结 | 只读 |

### 2.2 源文件 (src/) — 工作区

| 文件 | Owner | 职责 |
|------|-------|------|
| `datatable.cpp` | **Agent C** | 数据存储、CSV 解析、列存取 |
| `axis_system.cpp` | **Agent C** | Nice Number 刻度算法、格式化 |
| `coordinate_transform.cpp` | **Agent C** | 线性/对数变换、Data→Device |
| `layout_engine.cpp` | **Agent C** | 标题区/轴标签区/图例区/绘图区计算 |
| `line_plot.cpp` | **Agent D** | LinePlot : IPlotType |
| `scatter_plot.cpp` | **Agent D** | ScatterPlot : IPlotType |
| `multi_axis.cpp` | **Agent D** | 多 Y 轴管理 |
| `legend_renderer.cpp` | **Agent D** | 图例布局与渲染 |
| `plot.cpp` | **Agent E** | Plot 门面类，串联所有组件 |
| `interaction_handler.cpp` | **Agent E** | HitTest + handleEvent |

### 2.3 后端 & 测试

| 文件 | Owner | 职责 |
|------|-------|------|
| `backends/recording/recording_device.h` | **Agent E** | Recording 后端实现 |
| `backends/blend2d/blend2d_device.h` | **Agent E** | Blend2D 参考后端 |
| `tests/test_interface_contract.cpp` | **Agent B** | 🔒 契约测试（修改需全员知晓） |
| `tests/test_axis.cpp` | **Agent C** | 轴系统测试 |
| `tests/test_transform.cpp` | **Agent C** | 变换测试 |
| `tests/test_plots.cpp` | **Agent D** | 图类型测试 |
| `tests/test_integration.cpp` | **Agent E** | 集成测试 |

### 2.4 构建 & 文档

| 文件 | Owner | 职责 |
|------|-------|------|
| `CMakeLists.txt` | **Agent B** | 构建系统，新增 .cpp 时更新 |
| `.github/workflows/ci.yml` | **Agent B** | CI 配置 |
| `scripts/gate-check.sh` | **Agent B** | 本地门禁脚本 |
| `README.md` | **Agent F** | 项目文档 |
| `docs/API_REFERENCE.md` | **Agent F** | API 文档 |
| `examples/minimal/main.cpp` | **Agent F** | 最小示例 |
| `examples/qt_backend/` | **Agent F** | Qt 集成示例 |

---

## 三、依赖拓扑 & 启动顺序

Agent 之间不是完全独立的。依赖关系决定了启动和集成顺序：

```
依赖拓扑（箭头 = "依赖"）

  Agent A (接口)
   │
   ├──► Agent C (核心算法)
   │       │
   │       └──► Agent D (图类型) ──► Agent E (门面+后端)
   │                                        │
   └──► Agent B (构建+CI) ◄─────────────────┘
   │
   └──► Agent F (文档) ←── 依赖所有人产出，Day 4-5 启动
```

### 启动时序

```
Day 1 上午 (并行)
  Agent A: 确认接口零变更，关闭冻结
  Agent B: CMake 骨架就绪，CI 配置就绪，contract test 通过
  Agent C: 开始 DataTable + Axis（仅依赖 types.h，已冻结）

Day 1 下午 (串行解锁)
  Agent C 完成 DataTable → Agent D 可开始 LinePlot
  Agent C 完成 Axis → Agent D 可开始 ScatterPlot

Day 2 (全并行)
  Agent C: CoordinateTransform + LayoutEngine
  Agent D: LinePlot + ScatterPlot（依赖 transform 接口，不依赖实现）
  Agent E: Recording 后端 + plot.cpp 集成
  Agent B: 维护 CMake，新增 target

Day 3 (全并行)
  Agent D: MultiAxis + Legend
  Agent E: HitTest + InteractionHandler
  Agent C: 补充测试

Day 4 (全并行)
  Agent E: Blend2D 后端
  Agent F: 文档 + 示例
  全员: 测试补充

Day 5 (收敛)
  Agent F: 终版文档
  Agent B: 最终 gate 检查
  Project Lead: 代码审查
```

---

## 四、Git 分支策略

```
main (受保护)
  │
  ├── feature/interface-freeze     ← Agent A (Day 1 上午，已合并)
  ├── feature/build-ci             ← Agent B (Day 1，已合并)
  ├── feature/core-algorithms      ← Agent C (Day 1-3)
  ├── feature/plot-types           ← Agent D (Day 1-4)
  ├── feature/backends-integration ← Agent E (Day 2-4)
  └── feature/docs-examples        ← Agent F (Day 4-5)
```

### 合并规则

```
每天 2 次固定合并窗口：12:00 和 17:00

合并流程：
  1. Agent 提交本地 commit
  2. Agent 本地运行 bash scripts/gate-check.sh
  3. gate 通过 → PR → main
  4. gate 失败 → 修复 → 重新 step 2
  5. 其他 Agent 在下次合并窗口前 rebase 到最新 main

冲突处理：
  - 同一文件冲突 → Owner 胜出，另一方需适配
  - CMakeLists.txt 冲突 → Agent B 协调
  - 接口文件冲突 → Agent A 裁决 + Project Lead 审批
```

---

## 五、Agent 间通信协议

Agent 之间**不直接对话**。所有协调通过以下 3 个渠道：

### 渠道 1：文件系统（主要）

```
Agent C 产出 axis_system.h 中的一个公共函数
  → 写在 include/xyplot/ 或 src/ 的头文件中
  → Agent D #include 并使用
  → 无需"通知"，编译时自然链接
```

### 渠道 2：每日合并窗口（同步点）

```
12:00 合并窗口：
  - 各 Agent 推送已完成的工作
  - 全体 rebase 到最新 main
  - 解决合并冲突（如有）
  - gate-check 必须通过

17:00 合并窗口：
  - 各 Agent 推送当日全部工作
  - 全体 rebase
  - gate-check 必须通过
  - Project Lead 审查当日 diff
  - 输出量化日报
```

### 渠道 3：Project Lead 裁决（阻塞升级）

```
当 Agent 遇到以下情况时，暂停工作，升级给 Project Lead：
  - 需要修改冻结接口
  - 与另一个 Agent 的代码产生设计冲突
  - 发现架构级别的缺口（原计划未覆盖的需求）
  - 性能问题需要跨模块协调
```

---

## 六、并行安全分析

### 6.1 无冲突保证

| Agent 对 | 共享文件 | 冲突风险 | 缓解 |
|----------|---------|---------|------|
| A ↔ Any | 头文件 | 低（A 冻结后只读） | A 是唯一写入者 |
| C ↔ D | 无 | **零** | C 写 `src/core/`，D 写 `src/plots/` |
| C ↔ E | 无 | **零** | 不同目录 |
| D ↔ E | `plot.cpp` | 中 | D 定义 IPlotType，E 注册到 Plot；通过接口隔离 |
| B ↔ Any | `CMakeLists.txt` | 中 | B 是唯一写入者；其他人告诉 B 新增了哪些 .cpp |
| F ↔ Any | 文档 | 低 | F Day 4 才启动，代码已稳定 |

### 6.2 plot.cpp 的协调（唯一高风险文件）

`plot.cpp` 是 Plot 门面类，Agent D 和 Agent E 都需要触碰它：

```
plot.cpp 拆分策略（Day 1 即执行）：

  plot.cpp → 拆为 3 个文件：
    ├── src/plot.cpp              ← Agent E (Plot::render 主流程)
    ├── src/plot_registry.cpp     ← Agent D (图类型注册)
    └── src/plot_interaction.cpp  ← Agent E (handleEvent)

  这样 D 和 E 不再触碰同一文件。
```

---

## 七、各 Agent 的 Session 启动指令

每个 Agent 作为一个独立的 Claude Code session。以下是推荐的启动 prompt 模板：

### Agent A — 接口守护

```
你是 XYPlot SDK 项目的接口守护 Agent。
你的职责：
1. 维护 include/xyplot/ 下的所有头文件
2. 接口已冻结（见 docs/interface-freeze.md），今日目标是零变更
3. 如果其他 Agent 请求接口变更，评估必要性并升级给 Project Lead
4. 审查其他 Agent 的 PR 中是否误改了头文件

工作目录：c:\Users\28161\OneDrive\WorkDir\AI2026\LearnProject
你拥有的文件：include/xyplot/*.h
只读：src/**, tests/**, docs/**

★★★ 反馈与同步（必须执行）★★★
1. 合并窗口前（12:00 / 17:00），先读公告板：
     → status/bulletin.md（Project Lead 的全局指令在这里）
2. 每次完成任务、遇到阻塞、或合并窗口前，更新你的状态文件：
     → status/agent-a-status.md（Project Lead 也会在这里写指令）
3. 遇到阻塞立即设为 🔴，并写入"阻塞项"。
```

### Agent B — 基础设施

```
你是 XYPlot SDK 项目的基础设施 Agent。
你的职责：
1. 维护 CMakeLists.txt — 当其他 Agent 新增 .cpp 时更新构建
2. 维护 CI 配置（.github/workflows/ci.yml）
3. 维护 gate-check 脚本（scripts/gate-check.sh）
4. 确保 test_interface_contract 持续通过
5. 管理依赖（如有新增第三方库）

工作目录：c:\Users\28161\OneDrive\WorkDir\AI2026\LearnProject
你拥有的文件：CMakeLists.txt, .github/**, scripts/**, tests/test_interface_contract.cpp
只读：include/xyplot/**, src/**

★★★ 反馈与同步（必须执行）★★★
1. 合并窗口前（12:00 / 17:00），先读公告板：
     → status/bulletin.md（Project Lead 的全局指令在这里）
2. 每次完成任务、遇到阻塞、或合并窗口前，更新你的状态文件：
     → status/agent-b-status.md（Project Lead 也会在这里写指令）
3. 遇到阻塞立即设为 🔴，并写入"阻塞项"。
```

### Agent C — 核心算法

```
你是 XYPlot SDK 项目的核心算法 Agent。
你的职责：
1. 实现 DataTable（src/datatable.cpp）
2. 实现 Axis System，包含 Nice Number 自动刻度算法
3. 实现 CoordinateTransform（LinearScale, LogScale）
4. 实现 LayoutEngine（标题/轴标签/图例/绘图区自动布局）
5. 编写对应的单元测试

工作目录：c:\Users\28161\OneDrive\WorkDir\AI2026\LearnProject
你拥有的文件：src/datatable.cpp, src/axis_system.cpp, src/coordinate_transform.cpp, src/layout_engine.cpp
依赖接口：include/xyplot/types.h, include/xyplot/irender_device.h（已冻结）
输出 API：供 Agent D 调用的纯计算函数（不需要 IRenderDevice）

★★★ 反馈与同步（必须执行）★★★
1. 合并窗口前（12:00 / 17:00），先读公告板：
     → status/bulletin.md（Project Lead 的全局指令在这里）
2. 每次完成任务、遇到阻塞、或合并窗口前，更新你的状态文件：
     → status/agent-c-status.md（Project Lead 也会在这里写指令）
3. 遇到阻塞立即设为 🔴，并写入"阻塞项"。
```

### Agent D — 图类型

```
你是 XYPlot SDK 项目的图类型 Agent。
你的职责：
1. 实现 IPlotType 接口
2. 实现 LinePlot（折线图渲染逻辑）
3. 实现 ScatterPlot（散点图渲染逻辑）
4. 实现 MultiAxis（多 Y 轴支持）
5. 实现 LegendRenderer
6. 编写对应的单元测试

工作目录：c:\Users\28161\OneDrive\WorkDir\AI2026\LearnProject
你拥有的文件：src/line_plot.cpp, src/scatter_plot.cpp, src/multi_axis.cpp, src/legend_renderer.cpp, src/plot_registry.cpp
依赖：Agent C 的坐标变换函数（纯计算），Agent A 的 IRenderDevice 接口
注意：你的代码调用 IRenderDevice，但不实现它

★★★ 反馈与同步（必须执行）★★★
1. 合并窗口前（12:00 / 17:00），先读公告板：
     → status/bulletin.md（Project Lead 的全局指令在这里）
2. 每次完成任务、遇到阻塞、或合并窗口前，更新你的状态文件：
     → status/agent-d-status.md（Project Lead 也会在这里写指令）
3. 遇到阻塞立即设为 🔴，并写入"阻塞项"。
```

### Agent E — 后端 & 集成

```
你是 XYPlot SDK 项目的后端集成 Agent。
你的职责：
1. 实现 Plot 门面类的 render() 和 handleEvent()
2. 实现 Recording Render Device
3. 实现 Blend2D 参考后端（可选加速项）
4. 实现 HitTest 系统
5. 实现 InteractionHandler
6. 编写集成测试

工作目录：c:\Users\28161\OneDrive\WorkDir\AI2026\LearnProject
你拥有的文件：src/plot.cpp, src/plot_interaction.cpp, src/hit_test.cpp,
              backends/recording/**, backends/blend2d/**, tests/test_integration.cpp
依赖：Agent D 的图类型实现，Agent C 的算法
你是最终的胶水层，把 C+D 的产出串联起来

★★★ 反馈与同步（必须执行）★★★
1. 合并窗口前（12:00 / 17:00），先读公告板：
     → status/bulletin.md（Project Lead 的全局指令在这里）
2. 每次完成任务、遇到阻塞、或合并窗口前，更新你的状态文件：
     → status/agent-e-status.md（Project Lead 也会在这里写指令）
3. 遇到阻塞立即设为 🔴，并写入"阻塞项"。
```

### Agent F — 文档 & 示例

```
你是 XYPlot SDK 项目的文档示例 Agent。
你的职责：
1. 编写 README.md（项目概述 + 快速开始 + 构建说明）
2. 编写 docs/API_REFERENCE.md
3. 编写 examples/minimal/main.cpp（Recording 后端示例）
4. 编写 examples/qt_backend/（Qt 集成示例，含完整 QtRenderDevice）
5. 编写 examples/blend2d_demo/（Blend2D 渲染 Demo，如有）
6. Day 5 执行代码审查辅助

工作目录：c:\Users\28161\OneDrive\WorkDir\AI2026\LearnProject
你拥有的文件：README.md, docs/API_REFERENCE.md, examples/**
你依赖所有人的产出，Day 4 启动即可

★★★ 反馈与同步（必须执行）★★★
1. 合并窗口前（12:00 / 17:00），先读公告板：
     → status/bulletin.md（Project Lead 的全局指令在这里）
2. 每次完成任务、遇到阻塞、或合并窗口前，更新你的状态文件：
     → status/agent-f-status.md（Project Lead 也会在这里写指令）
3. 遇到阻塞立即设为 🔴，并写入"阻塞项"。
```

---

## 八、每日节奏

```
09:00  Project Lead 发布今日任务清单（基于 04-roadmap.md 当日目标）
       ↓
09:15  各 Agent 启动，开始工作
       ↓
       ... 独立并行工作 ...
       ↓
11:45  各 Agent 本地运行 gate-check.sh
       ↓
12:00  合并窗口 #1：PR → main，全体 rebase
       Project Lead 快速审查 diff
       ↓
13:00  下午工作开始
       ↓
       ... 独立并行工作 ...
       ↓
16:45  各 Agent 本地运行 gate-check.sh
       ↓
17:00  合并窗口 #2：PR → main，全体 rebase
       最终 gate-check 必须通过
       ↓
17:30  量化日报（4 项指标）→ PO
```

---

## 九、异常处理

| 异常场景 | 处理方式 |
|----------|---------|
| Agent X 的 PR 导致 gate 失败 | 该 PR 不得合并；Agent X 在下次合并窗口前修复 |
| 两个 Agent 修改同一文件 | Owner 胜出；非 Owner 需适配 Owner 的版本 |
| Agent 产出落后于计划 | Project Lead 在 12:00 合并窗口评估；必要时调整分工 |
| 发现需要修改冻结接口 | Agent A 评估 → Project Lead 审批 → 更新 interface-freeze.md → 更新 contract test → 通知全员 |
| Agent session 崩溃/丢失上下文 | 从 main 分支重新开始；未提交的代码丢失（每日 2 次合并降低此风险） |
| Blend2D 编译/集成超预期困难 | Day 3 17:00 决策点：降级为可选，优先保障 Recording 链路 |

---

## 十、Agent → Project Lead 反馈通道

本文档定义了 Project Lead → Agent 的下行通道。上行通道（Agent → Project Lead）通过 **status/ 目录**实现。

详见 [`status/README.md`](../status/README.md) — Agent 反馈协议。

### 快速概览

```
Project Lead                     Agent
     │                              │
     │  下行: 07-agent-coordination │
     ├─────────────────────────────►│  读取本文档，了解自己的任务
     │                              │
     │  上行: status/agent-X-status.md
     │◄─────────────────────────────┤  写入状态文件
     │                              │
     │  扫描                        │
     ├── status/agent-a-status.md   │
     ├── status/agent-b-status.md   │
     ├── ...                        │
     │                              │
     │  grep "🔴" status/*.md       │  标记阻塞
     │◄── 立即介入                  │
     │                              │
     │  17:30 汇总                  │
     ├── status/daily-report-*.md   │
     └── 发送 PO                    │
```

### Agent 必须更新的时机

| 时机 | 动作 |
|------|------|
| 任务开始/完成 | 更新"当前任务" |
| 遇到阻塞 | **立即**更新"阻塞项" + 设为 🔴 |
| 11:45 | 更新 Gate Check + 已修改文件 |
| 16:45 | 更新 Gate Check + 已修改文件 |

### Project Lead 监控命令

```bash
# 快速扫描所有 Agent 状态
grep -l "🔴" status/agent-*-status.md

# 查看某个 Agent 详情
cat status/agent-c-status.md

# 检查 gate 状态
grep "Gate Check" status/agent-*-status.md
```

---

## 十一、Project Lead 检查清单（每日）

```
□ 09:00 — 发布今日任务清单
□ 09:05 — 扫描 status/ 确认所有 Agent 状态正常
□ 11:50 — 收集各 Agent 合并意愿，协调顺序
□ 12:00 — 合并窗口审查
□ 12:00 — gate-check 确认通过
□ 13:00 — 扫描 status/ 确认下午无阻塞
□ 16:50 — 收集各 Agent 合并意愿，协调顺序
□ 17:00 — 合并窗口审查
□ 17:00 — gate-check 确认通过
□ 17:30 — 汇总 status/ → daily-report → 量化日报发送 PO
□ 随时  — grep "🔴" status/ 并立即介入
```

---

> **核心思想**：6 个 Agent 不是 6 个人在开会——是 6 个进程在各自的文件域内独立工作。文件所有权矩阵消除了 90% 的协调开销。合并窗口 + gate-check 处理剩余 10%。**上行通过 status/*.md，下行通过 docs/07，双向闭环。**
