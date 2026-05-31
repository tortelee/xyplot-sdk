# 📋 Project Lead 公告板

**最后更新**: Bug 修复启动
**状态**: 🔴 3 个客户 Bug 待修复

---

## 活跃指令：Bug 修复

**Bug 跟踪**: [`docs/08-bug-tracker.md`](../docs/08-bug-tracker.md)
**修复指令**: [`docs/07-agent-coordination.md`](../docs/07-agent-coordination.md) §14

| Bug | 现象 | 负责 |
|-----|------|------|
| BUG-001 | X 轴不显示（canvas 尺寸不匹配） | Agent E |
| BUG-002 | Bar chart 显示为曲线（缺 API） | Agent E + Agent D |
| BUG-003 | Multi-axis 不显示（缺右轴绑定） | Agent E |

| Agent | 任务 |
|-------|------|
| **Agent E** | Plot API 扩展: setCanvasSize + addBarSeries 等 + yAxisIndex + 回归测试 |
| **Agent D** | render() 中按 SeriesType 分发到 IPlotType |
| **Agent F** | Gallery 改用新 API + setCanvasSize |
| **Agent B** | 最终 gate-check --full |
| **Agent A** | API 变更审核 |

**预计 1.5h 全部修复。**
