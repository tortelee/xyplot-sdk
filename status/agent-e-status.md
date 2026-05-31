# Agent E 状态 — 后端 & 集成

**最后更新**: 2026-05-31 §15 Bug 修复 #2 完成
**当前阶段**: §15 Bug 修复
**状态**: 🟢 正常

---

## 全部完成任务总览

### Phase 0-2 + Phase C + Phase B
- [x] P0: Plot 门面渲染管线 ✅
- [x] P0: HitTest + InteractionHandler + 双后端 ✅
- [x] P0: 切换 Agent C 正式模块 ✅
- [x] Phase C: 性能基线 35/35 ✅
- [x] Phase B1: 集成测试 6 项 ✅
- [x] Phase B2: IRenderDevice +2 + 集成测试 5 项 ✅

### §14 Bug 修复
- [x] **BUG-001**: `setCanvasSize()` (plot.h + plot.cpp) ✅
- [x] **BUG-002**: 8 种 `add*Series()` 便捷方法 (plot.h + plot.cpp) ✅
- [x] **BUG-003**: `addLineSeries`/`addScatterSeries` yAxisIndex 参数 ✅
- [x] 回归测试 3 项 ✅

### §15 Bug 修复 #2
- [x] **BUG-005**: X轴标签间距 — `pr.y + pr.h + 4` → `pr.y + pr.h + tickFont.size + 6` (16px gap ≥ 15px 要求) ✅

## §14 变更详情

### API 变更 (plot.h)
| 变更 | 类型 | 破坏性 |
|------|------|--------|
| `setCanvasSize(w, h)` | 新增 | ❌ |
| `addLineSeries` + `yAxisIndex` 默认参数 | 扩展 | ❌ |
| `addScatterSeries` + `yAxisIndex` 默认参数 | 扩展 | ❌ |
| `addBarSeries` ~ `addContourSeries` (8 个) | 新增 | ❌ |

### SeriesType 扩展 (plot_impl.h)
```
Line → Line, Scatter, Bar, Step, Area, Histogram, ErrorBar, Polar, Heatmap, Contour
```

### 回归测试
| 测试 | Bug | 验证 |
|------|-----|------|
| `test_bug001_set_canvas_size` | BUG-001 | 800×500 canvas 内所有内容在 bounds 内 |
| `test_bug002_bar_series_type` | BUG-002 | 8 种 add*Series 全部可用 + render 不崩溃 |
| `test_bug003_y_axis_index` | BUG-003 | yAxisIndex=1 绑定到右轴，标签正常 |

## Gate Check

| 时间 | 结果 |
|------|------|
| §14 最终 | ✅ 163/163 |
| §15 BUG-005 | ✅ 163/163 零回归 |

## 测试增长轨迹

```
Phase 0:    1 套件,   ~5 断言
Phase 1:    2 套件,  ~56 断言
Phase C:    3 套件,  ~91 断言
Phase B1:   3 套件, ~109 断言
Phase B2:   3 套件, ~130 断言
§14 Bug:    3 套件,  163 断言 ✅
```

## 阻塞项

> 无。

## 备注

- 所有 API 变更均为纯新增或带默认参数扩展，零破坏性变更
- `addLineSeries(name, xs, ys, count)` 仍可调用（yAxisIndex 默认 0）
- 渲染管线已集成 Agent D 的类型分发（通过 PlotRegistry + IPlotType）
