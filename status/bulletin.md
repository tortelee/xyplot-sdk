# 📋 Project Lead 公告板

**最后更新**: 2026-05-31 17:30
**所有人**: Project Lead
**所有人必须读**: 每次合并窗口前（12:00 / 17:00）先读本文件

---

## 当前全局状态：🟡 集成收敛中

```
编译: ✅ 25 targets, 0 warnings, 0 errors
测试: ✅ 5/5 套件, 100% 通过
P0:   ✅ 100% 完成
问题: 🟡 3 个模块间重复实现需去重
```

---

## ⚠️ 活跃指令（按优先级）

### P0 — 必须完成，否则不合并

| # | 负责 | 指令 | 详情文件 |
|---|------|------|---------|
| 1 | **Agent E** | plot.cpp 切换为调用 Agent C 的模块（移除内联 tick/transform/layout） | `agent-e-status.md` |
| 2 | **Agent D** | xyplot_internal.h 移除重复的 transform 桩 + Nice Number → 改用 Agent C | `agent-d-status.md` |

### P1 — 应该完成

| # | 负责 | 指令 | 详情文件 |
|---|------|------|---------|
| 3 | **Agent C** | 审查 Agent B 的临时修复（axis_system.cpp:141） | `agent-c-status.md` |
| 4 | **Agent D** | 暴露内部类型声明（createPlotType 等）到 xyplot_internal.h | `agent-d-status.md` |
| 5 | **Agent E** | 审查 Agent B 的临时修复（plot.cpp 2 处） | `agent-e-status.md` |
| 6 | **Agent A** | 更新 interface-freeze.md 版本号 | `agent-a-status.md` |
| 7 | **Agent F** | 切换完成后验证 minimal 示例 | `agent-f-status.md` |

### P2 — 择机完成

| # | 负责 | 指令 | 详情文件 |
|---|------|------|---------|
| 8 | **Agent B** | 最终 gate-check --full 验证 | `agent-b-status.md` |

---

## 📊 各 Agent 评分卡

| Agent | 评分 | 状态 | 核心问题 |
|-------|------|------|---------|
| A — 接口守护 | ⭐5.0 | 🟢 | — |
| B — 基础设施 | ⭐4.8 | 🟢 | 跨 Owner 临时修复（已记录） |
| C — 核心算法 | ⭐5.0 | 🟢 | — |
| D — 图类型 | ⭐4.6 | 🟡 | 🔴 重复实现需去重 |
| E — 后端集成 | ⭐4.8 | 🟡 | 🔴 内联代码需切换 |
| F — 文档示例 | ⭐5.0 | 🟢 | — |

> 评分详情见 `daily-report-2026-05-31.md`

---

## 📌 重复实现问题（17:00 合并窗口重点）

```
问题：3 个 Agent 各自实现了同一功能

坐标变换:
  Agent C: src/coordinate_transform.cpp  ← 正式模块
  Agent D: src/xyplot_internal.h transform::  ← 需移除
  Agent E: src/plot.cpp 内联              ← 需切换

刻度算法:
  Agent C: src/axis_system.cpp           ← 正式模块
  Agent D: src/multi_axis.cpp 内联        ← 需移除
  Agent E: src/plot.cpp 内联              ← 需切换

目标状态:
  Agent C: 唯一实现者
  Agent D+E: 调用者 (#include Agent C 的头文件)
```

---

## 🔄 17:00 合并窗口流程

```
16:45  各 Agent 重新读取本公告板 + 自己的 status 文件
       ↓
16:50  Agent D 和 Agent E 完成切换，本地 gate-check 通过
       ↓
17:00  Agent B 运行 gate-check --full
       ↓
       如果全部通过 → 合并 → 发布 Day 1 最终版本
       如果有失败 → 对应 Agent 修复 → 18:00 追加合并
```

---

## 📖 文件索引（Agent 速查）

| 我想了解... | 看哪个文件 |
|------------|-----------|
| 我的具体任务和指令 | `status/agent-X-status.md`（X = 你的字母） |
| 全局状态和其他 Agent 进度 | `status/bulletin.md`（本文件） |
| 今日完整评估报告 | `status/daily-report-2026-05-31.md` |
| 项目总体设计 | `docs/07-agent-coordination.md` |
| 接口冻结清单 | `docs/interface-freeze.md` |
| 量化日报（给 PO） | `status/daily-report-2026-05-31.md` §量化指标 |
