# Agent 反馈通道协议

这是 6 个 Agent 与 Project Lead 之间的双向通道。

---

## 双向闭环

```
Project Lead                     Agent
     │                              │
     │  下行通道                     │
     ├── status/bulletin.md ────────►│  ← 所有 Agent 读这里（全局指令）
     ├── status/agent-X-status.md ──►│  ← 单个 Agent 读这里（个人指令）
     │                              │
     │  上行通道                     │
     │◄─ status/agent-X-status.md ───┤  ← Agent 写这里（进度汇报）
     │                              │
     │  汇总                        │
     ├── status/daily-report-*.md    │  ← 17:30 Project Lead 编写
     └── 发送 PO                    │
```

## Agent 必读文件（优先级从高到低）

| 优先级 | 文件 | 何时读 |
|--------|------|--------|
| 🔴 最高 | `status/bulletin.md` | **每次合并窗口前必须先读** |
| 🟡 高 | `status/agent-X-status.md` | 启动时 + 每次更新前 |
| 🟢 中 | `status/daily-report-*.md` | 想了解全局时 |
| ⚪ 参考 | `docs/07-agent-coordination.md` | 启动时 |
| ⚪ 参考 | `docs/interface-freeze.md` | Agent A 维护，其他只读 |

---

## 一、状态文件位置

```
status/
├── README.md                  ← 本文档（协议说明）
├── agent-a-status.md          ← Agent A 状态
├── agent-b-status.md          ← Agent B 状态
├── agent-c-status.md          ← Agent C 状态
├── agent-d-status.md          ← Agent D 状态
├── agent-e-status.md          ← Agent E 状态
├── agent-f-status.md          ← Agent F 状态
└── daily-report-YYYY-MM-DD.md ← 每日汇总（Project Lead 在 17:30 填写）
```

---

## 二、状态文件格式（每个 Agent 必须遵守）

每个 Agent 的状态文件采用以下固定格式，确保 Project Lead 可快速扫描：

```markdown
# Agent X 状态

**最后更新**: 2026-05-31 14:30
**当前阶段**: Phase 1 — 核心引擎
**状态**: 🟢 正常 / 🟡 有风险 / 🔴 阻塞 / ⚪ 待启动

---

## 当前任务

- [x] 已完成的任务项 1  (提交: abc1234)
- [ ] 进行中的任务项 2   (预计完成: 15:00)
- [ ] 待开始的任务项 3

## 已修改文件（自上次合并窗口）

| 文件 | 操作 | 行数 |
|------|------|------|
| src/axis_system.cpp | 新增 | +320 |
| tests/test_axis.cpp | 新增 | +150 |

## Gate Check

| 时间 | 结果 |
|------|------|
| 12:00 | ✅ 通过 |
| 17:00 | ⏳ 待运行 |

## 阻塞项

> 无。

或

> 🔴 **阻塞**: 需要 Agent A 在 irender_device.h 中新增 drawDashedLine 方法。
> **影响**: LinePlot 虚线样式无法实现。
> **请求**: 请 Project Lead 协调 Agent A 评估。
> **临时绕过**: 用实线替代，功能降级但不阻塞进度。

## 下一合并窗口计划

- 提交 axis_system.cpp 完整实现
- 提交 test_axis.cpp（10 个测试用例）
```

---

## 三、Agent 更新规则

| 时机 | 必须更新的字段 | 说明 |
|------|-------------|------|
| **任务开始** | 当前任务, 状态 | 标记正在做什么 |
| **任务完成** | 当前任务 (勾选), 已修改文件 | 产出记录 |
| **遇到阻塞** | 阻塞项, 状态 → 🔴 | **立即更新**，不要等到合并窗口 |
| **阻塞解除** | 阻塞项, 状态 → 🟢 | 确认问题已解决 |
| **11:45** | Gate Check, 已修改文件 | 12:00 合并窗口前 |
| **16:45** | Gate Check, 已修改文件 | 17:00 合并窗口前 |
| **每日结束** | 全部字段 | 最终状态快照 |

---

## 四、Project Lead 监控流程

```
Project Lead 的职责：

1. 随时查看 status/ 目录，了解各 Agent 状态
    $ ls status/agent-*-status.md
    $ cat status/agent-c-status.md

2. 重点关注 🔴 阻塞标记
    $ grep -l "🔴" status/agent-*-status.md
    → 立即介入处理

3. 11:50 收集各 Agent 提交意愿
    → 协调合并顺序（如有冲突风险）

4. 16:50 收集各 Agent 提交意愿
    → 协调合并顺序

5. 17:30 汇总所有 Agent 状态 → daily-report-YYYY-MM-DD.md
    → 填写 4 项 PO 量化指标
    → 发送给 PO
```

---

## 五、升级路径（Agent 侧）

```
Agent 遇到问题
    │
    ├── 技术问题（可自行解决）
    │   → 记录在状态文件"当前任务"中
    │   → 不升级
    │
    ├── 需要其他 Agent 配合（接口/数据格式）
    │   → 更新状态文件"阻塞项"，写明需要的配合
    │   → 继续做不受影响的其他任务
    │   → Project Lead 在下次扫描时协调
    │
    ├── 需要修改冻结接口
    │   → 更新状态文件"阻塞项"
    │   → 状态设为 🔴
    │   → Project Lead 评估 → 必要时升级给 PO
    │
    └── 技术阻塞（无法推进）
        → 更新状态文件"阻塞项"
        → 状态设为 🔴
        → 描述: 阻塞原因 + 已尝试的方案 + 建议
        → Project Lead 立即介入
```

---

## 六、17:30 每日汇总模板

Project Lead 在每日 17:30 填写的汇总文件：

```markdown
# 每日汇总 — 2026-05-31

## 量化指标

1. 接口变更数
   新增: 0   删除: 0   破坏性: 0

2. 客户适配代码行数
   最小示例: 150 行   L0 核心适配: 80 行

3. 集成测试通过率
   test_interface_contract: ✅
   单元测试: 15/15 通过 (100%)

4. P0 功能完成率
   ✅ Line Plot        (Agent D)
   ✅ Scatter Plot     (Agent D)
   🔄 Multi-Y-Axis    (Agent D, 80%)
   总体: 60%

## Agent 摘要
| Agent | 状态 | 今日产出 | 阻塞 |
|-------|------|---------|------|
| A | 🟢 | 接口零变更 | 无 |
| B | 🟢 | CMake GLOB 策略 | 无 |
| C | 🟢 | Axis + Transform 完成 | 无 |
| D | 🟡 | MultiAxis 延期到明早 | 无 |
| E | 🟢 | Recording 后端完成 | 无 |
| F | ⚪ | Day 4 启动 | — |

## 风险
- Agent D 的 MultiAxis 比预估多 2 小时。不影响 Day 5 交付。
```

---

## 七、状态目录完整结构

```
status/
├── README.md
├── agent-a-status.md    ← Agent A 每次更新时 overwrite
├── agent-b-status.md    ← Agent B 每次更新时 overwrite
├── agent-c-status.md    ← Agent C 每次更新时 overwrite
├── agent-d-status.md    ← Agent D 每次更新时 overwrite
├── agent-e-status.md    ← Agent E 每次更新时 overwrite
├── agent-f-status.md    ← Agent F 每次更新时 overwrite
├── daily-report-2026-05-31.md
├── daily-report-2026-06-01.md
├── daily-report-2026-06-02.md
├── daily-report-2026-06-03.md
└── daily-report-2026-06-04.md
```
