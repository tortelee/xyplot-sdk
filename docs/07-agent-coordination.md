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

---

## 十二、Phase C：质量深化（Day 2 上午）

**发布时间**: 2026-05-31 Day 1 收尾
**执行时间**: Day 2 09:00 - 12:00
**目标**: 补齐测试缺口、建立性能基线、内存安全验证

### 12.1 质量基线扫描结果

| 维度 | 结果 |
|------|------|
| 编译警告 | ✅ 25 targets, 0 warnings |
| 测试通过率 | ✅ 5/5 套件, 100% |
| 头文件守卫 | ✅ 14/14 `#pragma once` |
| TODO/FIXME | ✅ 零个 |
| NaN/Inf 处理 | ✅ coordinate_transform 完整处理 |
| 除零保护 | ✅ 使用 `DBL_EPSILON` 检查 |
| 数组越界 | ✅ 所有访问有边界检查 |
| 公开/内部 API 分离 | ✅ include/ vs src/ |
| 调试打印 | ✅ 源码中无 printf/cout |
| **DataTable 独立测试** | ❌ 缺失 |
| **性能基线** | ❌ 无数据 |
| **内存安全检查** | ❌ 未运行 ASan/Valgrind |

### 12.2 任务分配

---

**Agent C — 补充 DataTable 测试** (P0, 预计 1 小时)

```
新增文件: tests/test_datatable.cpp

测试用例（≥15 项）:
  1. fromMemory — 正常 3×2 矩阵
  2. fromMemory — 空数据 (0 rows)
  3. fromMemory — 单行单列
  4. column() — 正常索引
  5. column() — 越界索引返回 nullptr
  6. columnName() — 正常索引
  7. columnName() — 越界索引返回空字符串
  8. rowCount() / colCount() — 与输入一致
  9. fromCSV — 正常文件（创建临时 CSV）
  10. fromCSV — 空文件
  11. fromCSV — 只有表头无数据行
  12. fromCSV — 包含空行的文件
  13. fromCSV — 不同分隔符（逗号/空格/Tab）
  14. column() — 返回指针的稳定性（两次调用同一索引）
  15. fromMemory — 大数据 (100万行) 性能和内存

验收: cmake --build build && ctest -R test_datatable --output-on-failure
```

---

**Agent E — 性能基线测试** (P1, 预计 30 分钟)

```
新增文件: tests/test_performance.cpp

测试用例:
  1. 1M 点坐标变换耗时（期望 < 50ms Debug, < 10ms Release）
  2. 1M 点折线渲染路径（transform + drawPolyline 调用序列, 不含实际渲染）
  3. 10M 点内存占用（DataTable 存储开销）
  4. Axis computeTicks 耗时（极端范围: 1e-15 到 1e15）

验收: cmake --build build && ./build/test_performance
```

---

**Agent B — 内存安全检查** (P1, 预计 30 分钟)

```
运行以下检查并记录结果:

  1. AddressSanitizer 构建:
     cmake -B build/asan -DCMAKE_BUILD_TYPE=Debug \
           -DCMAKE_CXX_FLAGS="-fsanitize=address -fno-omit-frame-pointer"
     cmake --build build/asan
     cd build/asan && ctest --output-on-failure

  2. 确认零内存错误 (0 leaks, 0 buffer overflows)

  3. 如果机器不支持 ASan (Windows/MSVC), 使用:
     - MSVC: /fsanitize=address
     - 或跳过，记录"ASan 在 Windows 上待 Linux CI 验证"

验收: 输出 sanitizer 报告，零错误
```

---

**Agent F — 文档同步检查** (P2, 预计 30 分钟)

```
检查项:
  1. API_REFERENCE.md 是否覆盖了 Include/ 下所有公开类型？
     - IRenderDevice (8 方法 + 1 可降级)
     - IInputSource / InputEvent
     - Plot (所有公开方法)
     - Color, LineStyle, MarkerStyle, FillStyle, FontDesc, TextStyle
     - DataPoint, Rect, ScaleType

  2. Agent C 的 src/*.h 是否需要在 API_REFERENCE 中记录？
     - coordinate_transform.h (transformPoints 等)
     - axis_system.h (computeTicks 等)
     - layout_engine.h (computeLayout)
     - datatable.h (DataTable)
     → 如果客户需要直接调用这些函数，更新文档
     → 如果仅内部使用，在 API_REFERENCE 中标注"内部模块"

  3. README.md 中的构建命令是否能一步跑通？
     → 实际执行验证

验收: 列出需要更新的条目，自行修复
```

---

**Agent A — 接口合规终审** (P2, 预计 15 分钟)

```
检查项:
  1. contract test 是否仍然 100% 反映冻结接口
  2. 所有 P0 必需方法是否都有 static_assert 类型检查
  3. interface-freeze.md 中 types.h 的版本号是否最新
  4. 确认 Day 1 全天零破坏性变更

验收: gate-check.sh 通过 + interface-freeze.md 版本号更新
```

---

### 12.3 执行节奏

```
09:00 — 所有 Agent 读本文档 §12，获取自己的任务
09:15 — Agent C/E/B/F/A 并行启动
11:00 — 合并窗口 #1：提交质量深化成果
11:30 — Project Lead 最终验证
12:00 — 质量深化阶段完成，输出 Day 2 上午日报
```

### 12.4 验收标准

| 指标 | 当前 | 目标 |
|------|------|------|
| 测试套件数 | 5 | **6** (+test_datatable) |
| 测试断言数 | ~120 | **~150** (+30) |
| 性能基线 | 无 | **有** (4 项指标) |
| 内存安全 | 未验证 | **ASan 零错误** |
| 文档同步 | 未知 | **已验证** |
| 接口合规 | ✅ | **终审确认** |

---

## 十三、Phase B：P1 图类型扩展

**发布时间**: Phase C 完成后
**执行时间**: 预计 1-2 天
**目标**: 新增 8 种图类型（P1），扩展 IRenderDevice +2 方法

### 13.1 架构影响分析

```
10 种 P1 图类型 → 按架构影响分 3 批:

B1 (零接口变更):  Bar, Step, ErrorBar, Histogram, Polar
  └─ 仅需新增 IPlotType 子类，复用现有 8+1 IRenderDevice

B2 (扩展接口):    Area, Heatmap, Contour
  └─ Area → 需要 fillPolygon（填充任意多边形）
  └─ Heatmap → 需要 drawImage（像素块渲染）
  └─ Contour → 需要算法 + drawPolyline（轮廓线可用现有接口）

B3 (3D):          3D Surface, 3D Scatter  → 架构复杂，另案处理
```

### 13.2 B1：零接口变更（5 种图类型）

**Agent A — 零工作**。B1 不需要 IRenderDevice 变更。

**Agent C — Polar 坐标变换** 

```
新增文件: src/polar_transform.h + src/polar_transform.cpp

接口:
  namespace xyplot {
    // 极坐标 → 笛卡尔坐标
    void polarToCartesian(double r, double theta, double& x, double& y);
    // 批量变换
    void polarToCartesianBatch(const double* r, const double* theta,
                               int count, double* outX, double* outY);
  }

验收: 单元测试 10+ 项 (0°/90°/180°/270°/负角/大半径/零半径)
```

**Agent D — 5 种新 IPlotType** 

```
每个图类型 1 个 .cpp 文件:

  src/bar_plot.cpp      (~80 行)  fillRect 绘制柱子
  src/step_plot.cpp     (~50 行)  drawPolyline 阶梯路径
  src/error_bar.cpp     (~60 行)  drawPolyline 误差线
  src/histogram.cpp     (~70 行)  fillRect + DataTable 分箱
  src/polar_plot.cpp    (~80 行)  drawPolyline + polar 坐标变换

每个类型:
  - 实现 IPlotType 接口 (typeName + render)
  - 注册到 PlotRegistry (Agent D 负责)

验收: 每个类型 ≥5 个单元测试 (test_plots.cpp 扩展)
```

**Agent E — 集成**

```
- PlotRegistry 注册 5 个新类型
- Plot::addBarSeries() / addStepSeries() 等便捷 API (可选)
- 集成测试扩展

验收: test_integration.cpp 新增 ≥5 个测试
```

### 13.3 B2：扩展 IRenderDevice（+2 方法，3 种图类型）

B2 在 B1 完成后启动。首先扩展 IRenderDevice：

**Agent A — 接口扩展审核**

```cpp
// 新增到 irender_device.h（冻结策略: virtual + 默认空实现）

class IRenderDevice {
public:
    // ... P0 8+1 不变 ...

    // ──── P1 扩展 (B2) ────
    
    // 填充任意多边形（Area Plot 使用）
    virtual void fillPolygon(const double* xs, const double* ys,
                             int count, const FillStyle& style) {}

    // 渲染像素图像（Heatmap 使用）
    // rgba: RGBA 格式像素，imgW×imgH
    virtual void drawImage(double x, double y, double w, double h,
                           const uint8_t* rgba, int imgW, int imgH) {}
};
```

**Agent D — 3 种新 IPlotType**

```
  src/area_plot.cpp     (~80 行)  fillPolygon 填充 + drawPolyline 边界
  src/heatmap.cpp       (~100 行) drawImage 渲染 + 色条
  src/contour.cpp       (~150 行) 等值线算法 + drawPolyline
```

**Agent C — Contour 算法**

```
新增文件: src/contour_algorithm.h + src/contour_algorithm.cpp

功能: Marching Squares 算法，输入 grid + 等值线值 → 输出轮廓路径

验收: 单元测试 ≥8 项
```

### 13.4 执行节奏

```
B1 (预计 1 天):
  09:00  Agent C 启动 polar_transform
         Agent D 启动 5 种 IPlotType
  12:00  合并窗口 — B1 类型提交
  14:00  Agent E 集成 B1 类型 → 集成测试
  17:00  B1 完成

B2 (预计 1 天):
  09:00  Agent A 审核 + 新增 fillPolygon/drawImage 到 IRenderDevice
         Agent C 启动 contour_algorithm
         Agent D 启动 Area/Heatmap/Contour
  12:00  合并窗口 — B2 类型提交
  17:00  B2 完成，Phase B 收尾
```

### 13.5 验收标准

| 指标 | Phase C 基线 | B1 目标 | B2 目标 |
|------|-------------|---------|---------|
| 图类型 | 2 (Line+Scatter) | **7** (+5) | **10** (+3) |
| IRenderDevice 方法 | 8+1 | 8+1 (不变) | **8+3** (+fillPolygon+drawImage) |
| 测试套件 | 7 | ≥**8** | ≥**10** |
| 总断言数 | ~190 | ≥**230** | ≥**280** |

---

## 十四、Bug 修复（客户反馈）

**来源**：客户测试 SVG demo 发现 3 个 Bug
**Bug 跟踪**：[`docs/08-bug-tracker.md`](./08-bug-tracker.md)
**执行时间**：立即
**目标**：修复 BUG-001/002/003，添加回归测试，更新 gallery

### 14.1 Bug 概述

| Bug | 现象 | 根因 | 责任模块 |
|-----|------|------|---------|
| BUG-001 | X 轴不显示 | `Plot` canvas 800×600 但 SVG 只有 800×500，底部被裁剪 | Agent E |
| BUG-002 | Bar 显示为曲线 | Gallery 用 `addLineSeries` 画 bar；Plot API 缺 `addBarSeries()` 等 | Agent E + Agent D |
| BUG-003 | Multi-axis 不显示 | `addLineSeries` 不支持 yAxisIndex，数据无法绑到右轴 | Agent E |

### 14.2 任务分配

---

**Agent E — Plot API 扩展 + 回归测试** (P0, 预计 1.5h)

```
任务 1: BUG-001 — setCanvasSize()
  新增 Plot::setCanvasSize(double width, double height)
  确保 render() 中的 layoutCfg.totalWidth/totalHeight 使用此值
  (当前已在用 m_impl->canvasWidth，只需暴露 setter)

任务 2: BUG-002 — add*Series() 便捷方法
  扩展 src/plot_impl.h: SeriesInfo::SeriesType 枚举:
    → Line, Scatter, Bar, Step, Area, Histogram, ErrorBar, Polar, Heatmap, Contour
  扩展 include/xyplot/plot.h:
    int addLineSeries(name, xs, ys, count, yAxisIndex = 0)  // 加 yAxisIndex 参数
    int addScatterSeries(name, xs, ys, count, yAxisIndex = 0)
    int addBarSeries(name, xs, ys, count)
    int addStepSeries(name, xs, ys, count)
    int addAreaSeries(name, xs, ys, count)
    int addHistogramSeries(name, xs, ys, count)
    int addErrorBarSeries(name, xs, ys, count)
    int addPolarSeries(name, xs, ys, count)
    int addHeatmapSeries(name, xs, ys, count)
    int addContourSeries(name, xs, ys, count)
  实现: 所有方法委托给一个内部的 addSeriesImpl() 辅助函数

任务 3: BUG-003 — yAxisIndex 支持
  addLineSeries/addScatterSeries 支持 yAxisIndex 参数
  render() 中根据 series.yAxisIndex 选择正确的 Y 轴范围 + 刻度

任务 4: 回归测试
  在 tests/test_integration.cpp 中新增测试:
    - BUG-001: setCanvasSize 后布局在 viewport 内
    - BUG-002: addBarSeries 创建的 series type = Bar
    - BUG-003: yAxisIndex=1 的 series 绑定到右轴
  tests/test_plots.cpp 中新增:
    - BarPlotType::render 产生 FillRect 调用 (≥3)
    - StepPlotType::render 产生 staircase polyline (≥3 段)
```

**Agent D — render() 类型分发** (P0, 预计 20min)

```
在 src/plot.cpp 的 render() 方法中，将当前对所有 series 统一调用
drawPolyline 的逻辑改为按 SeriesType 分发:

  for (auto& s : m_impl->series) {
      switch (s.type) {
          case SeriesInfo::Line:      → LinePlot 或直接 drawPolyline
          case SeriesInfo::Scatter:   → ScatterPlot 或直接 drawMarkers
          case SeriesInfo::Bar:       → 通过 PlotRegistry 创建 "Bar" → render()
          case SeriesInfo::Step:      → 通过 PlotRegistry 创建 "Step" → render()
          case SeriesInfo::Area:      → 通过 PlotRegistry 创建 "Area" → render()
          ...
      }
  }

  或更简洁: 用 PlotRegistry 查找 IPlotType，统一调用 type->render()
  注意: 需要将 SeriesInfo 转换为 SeriesRenderData + AxisRenderConfig

验收: Bar chart SVG 中出现 <rect> 元素
```

**Agent F — Gallery 修复** (P0, 预计 20min)

```
修改 examples/svg_gallery/main.cpp:
  1. 每个 make*() 函数中，render() 前加:
     plot.setCanvasSize(800, 500);
  2. makeBarChart(): addLineSeries → addBarSeries
  3. makeMultiAxis(): 湿度线改为 addLineSeries("Humidity", ..., 1)  // yAxisIndex=1
  4. makeHistogram(): addLineSeries → addHistogramSeries
  5. makeErrorBar(): addLineSeries → addErrorBarSeries
  6. makeAreaPlot(): addLineSeries → addAreaSeries
  7. makePolarPlot(): addLineSeries → addPolarSeries

验收: 重新生成 gallery，所有 8 张图效果正确
```

**Agent B — 最终验证** (P0, 预计 10min)

```
Agent C/D/E/F 全部完成后:
  bash scripts/gate-check.sh --full
  确认: 全量编译 + 全部测试通过
  确认: gallery/*.svg 中:
    - 01_line_plot.svg 的 X 轴线 y < 500
    - 03_bar_chart.svg 包含 <rect> 元素
    - 04_multi_axis.svg 包含右侧 Y 轴刻度
```

**Agent A — API 变更审核** (P1, 预计 10min)

```
审查 plot.h 的新增方法是否符合冻结策略:
  - setCanvasSize: 纯新增 ✅
  - addBarSeries 等: 纯新增 ✅
  - addLineSeries 新增默认参数: 非破坏性 ✅
  - 更新 interface-freeze.md 记录此轮变更
```

### 14.3 执行节奏

```
立即  — Agent E + Agent D 并行修复
30min — Agent F 更新 gallery 
45min — Agent E 添加回归测试
60min — Agent B gate-check --full
75min — 提交 + 推送
```

### 14.4 验收标准

| Bug | 验收 |
|-----|------|
| BUG-001 | SVG 中 X 轴线 polyline Y 坐标 < SVG height |
| BUG-002 | `03_bar_chart.svg` 包含 `<rect>` 元素 |
| BUG-003 | `04_multi_axis.svg` 包含右轴刻度标签 |
| 回归 | gate-check --full 全部通过 |
| Gallery | 全部 8 张 SVG 可用浏览器正常查看 |
| 破坏性变更 | 0 | 0 | 0 |

---

## 十五、Bug 修复 #2（客户第二轮反馈）

**Bug 跟踪**：[`docs/08-bug-tracker.md`](./08-bug-tracker.md)
**执行时间**：立即

### ⚠️ 本阶段所有 Agent 注意

**完成任务 ≠ 写完代码。做完后必须做以下 3 件事，否则 Project Lead 不知道你完成了：**

1. 本地运行 `bash scripts/gate-check.sh`（或 cmake --build build && ctest）
2. **更新 `status/agent-X-status.md`**：勾选已完成的任务，填写 Gate Check 结果
3. 如果任务失败或被阻塞，状态改为 🔴 并写"阻塞项"

---

### 15.1 Bug 概述

| Bug | 现象 | 根因 | 文件 |
|-----|------|------|------|
| BUG-005 | X轴数字压在轴线上 | 刻度标签 Y 与轴线 Y 仅差 4px | plot.cpp |
| BUG-006 | Polar 图例蓝色，曲线黑色 | marker 用了默认色 {0,0,0} 而非继承 lineStyle | polar_plot.cpp |
| BUG-007 | Scatter 连成曲线 | scatter_plot.cpp 额外调了 drawPolyline | scatter_plot.cpp |

### 15.2 任务分配

---

**Agent E — BUG-005: X轴标签间距** (P0, 10min)

```
文件: src/plot.cpp

找到 X 轴刻度标签绘制代码，将标签 Y 坐标从:
  y = xAxisLineY + tickLength + 2   // 仅 4-5px 间距
改为:
  y = xAxisLineY + tickLength + tickFontSize + 4  // ~18px 间距

参考计算: 轴线 y=445, tickLength=5, fontSize=10, gap=4
         → 标签 y = 445 + 5 + 10 + 4 = 464

验证: 重新生成 gallery，X 轴标签 Y − 轴线 Y ≥ 15px

⚠️ 完成后必须: 更新 status/agent-e-status.md，勾选任务 + 标注 Gate Check 结果
```

**Agent D — BUG-006: Polar 颜色继承** (P0, 10min)

```
文件: src/polar_plot.cpp

问题: 渲染 markers 时使用默认 MarkerStyle (fillColor={0,0,0})
修复: 与 scatter_plot.cpp 一样，从 data.lineStyle.color 继承:
  MarkerStyle style = data.markerStyle;
  if (style.fillColor.r == 0 && style.fillColor.g == 0 && style.fillColor.b == 0) {
      style.fillColor = data.lineStyle.color;
      style.fillColor.a = 200;
  }
  if (style.edgeColor.r == 0 && style.edgeColor.g == 0 && style.edgeColor.b == 0) {
      style.edgeColor = data.lineStyle.color;
  }

验证: SVG 中 circle fill/stroke = legend 色块颜色

⚠️ 完成后必须: 更新 status/agent-d-status.md，勾选任务 + 标注 Gate Check 结果
```

**Agent D — BUG-007: Scatter 去连线** (P0, 5min)

```
文件: src/scatter_plot.cpp:53-58

删除 drawPolyline 调用（将散点连成线的代码块）。
保留 drawMarkers 调用。

验证: 02_scatter_plot.svg 中无数据 polyline（仅网格/轴线允许）

⚠️ 完成后必须: 更新 status/agent-d-status.md，勾选任务 + 标注 Gate Check 结果
```

**Agent B — 最终验证** (P0, 10min)

```
Agent E/D 完成后:
  cmake --build build && ctest --output-on-failure
  ./build/svg_gallery
  grep 验证: BUG-005 Y gap ≥ 15, BUG-006 circle fill ≠ black, BUG-007 scatter polyline=0

⚠️ 完成后必须: 更新 status/agent-b-status.md，写入验证结果
```

### 15.3 验收

| Bug | 验收标准 |
|-----|---------|
| BUG-005 | X 轴标签 Y − 轴线 Y ≥ 15px |
| BUG-006 | Polar SVG 中 circle fill != "rgb(0,0,0)" |
| BUG-007 | Scatter SVG 中唯一的 polyline 是网格/轴线 |
