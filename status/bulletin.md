# 📋 Project Lead 公告板

**最后更新**: 2026-05-31 — 关机前状态保存
**状态**: 🟢 v1.0.0 已发布，项目暂缓

---

## 当前状态

```
版本:     v1.0.0 (tag 已推送到 GitHub)
提交:     28c6b32
仓库:     github.com/tortelee/xyplot-sdk
状态:     客户满意，暂缓 B3 (3D)
```

---

## 🔄 重启指南（下次打开时）

### 恢复代码

```bash
cd c:/Users/28161/OneDrive/WorkDir/AI2026/LearnProject
git pull   # 拉取最新代码（如果有远程更新）
```

### Agent 不需要"记住"任何东西

Agent 是无状态的 — 每次启动都是新的 session。它们需要的全部信息都在这些文件里：

| Agent 启动时读什么 | 文件 |
|-------------------|------|
| 我是谁，负责什么，怎么写反馈 | `docs/07-agent-coordination.md` §7 |
| 当前全局状态，有没有活跃指令 | `status/bulletin.md`（本文件） |
| 我上次做了什么 | `status/agent-X-status.md`（X = A/B/C/D/E/F） |
| 接口约束 | `docs/interface-freeze.md` |
| Bug 记录 | `docs/08-bug-tracker.md` |

**所有决策、设计、Bug 都在 docs/ 目录里，Agent 读到就能继续工作。**

### 启动 Agent 的标准命令

```
读 docs/07-agent-coordination.md 第7节了解你的职责，
读 status/bulletin.md 了解当前状态，
读 status/agent-X-status.md 了解你上次做了什么，
然后开始工作。
```

---

## 项目文件地图

```
docs/
├── 00-executive-summary.md      → PO 摘要
├── 01-requirements-clarification.md → 需求 + PO 回复
├── 02-technical-assessment.md   → 技术选型
├── 03-solution-proposal.md      → 架构方案
├── 04-roadmap.md                → 路线图
├── 05-po-review-opinion.md      → PO 审阅
├── 06-po-review-response.md     → PO 回复
├── 07-agent-coordination.md     → Agent 协作（任务在这）
├── 08-bug-tracker.md            → Bug 跟踪
├── interface-freeze.md          → 接口冻结清单
├── integration-contract.md      → 客户接入契约
└── API_REFERENCE.md             → API 文档

status/
├── bulletin.md                  → 公告板（Agent 先读这个）
├── agent-[a-f]-status.md        → 各 Agent 状态
└── daily-report-*.md            → 日报
```

---

## 待定

- B3: 3D Surface + 3D Scatter（需架构设计）
