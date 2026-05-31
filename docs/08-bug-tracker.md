# Bug 跟踪

**创建日期**：2026-05-31
**维护人**：Project Lead
**状态说明**：🔴 未修复 / 🟡 修复中 / 🟢 已修复 / ⚪ 非Bug

---

## 总览

| # | Bug | 严重度 | 状态 | 发现 | 修复 | 负责 |
|---|-----|--------|------|------|------|------|
| BUG-001 | X 轴不显示 | 🔴 P0 | 🟢 已修复 | 客户 | 2026-05-31 | Agent E |
| BUG-002 | Bar chart 显示为曲线 | 🔴 P0 | 🟢 已修复 | 客户 | 2026-05-31 | Agent E + Agent D |
| BUG-003 | Multi-axis 不显示 | 🔴 P0 | 🟢 已修复 | 客户 | 2026-05-31 | Agent E |
| BUG-004 | gallery SVG 不设 canvasSize | 🟡 P1 | 🟢 已修复 | 内部 | 2026-05-31 | Agent F |
| BUG-005 | X轴标签与轴线重叠 | 🔴 P0 | 🟢 已修复 | 客户 | 2026-05-31 | Agent E |
| BUG-006 | Polar 图例与曲线颜色不一致 | 🔴 P0 | 🟢 已修复 | 客户 | 2026-05-31 | Agent D |
| BUG-007 | Scatter 连成曲线而非散点 | 🔴 P0 | 🟢 已修复 | 客户 | 2026-05-31 | Agent D |

---

## BUG-005：X轴刻度标签与轴线重叠

**发现**：客户查看 SVG，X 轴数字 "横插在轴线上"。

**根因**：轴刻度标签 Y 坐标与轴线 Y 坐标间距仅 4px（轴线 y=445，标签 y=449），视觉上标签压在轴线上。`plot.cpp` 中刻度标签的 Y 位置计算未留足间距——应该在轴线下方至少 12-15px。

**修复方案**：`plot.cpp` 中 X 轴刻度标签绘制时，Y 坐标从 `xAxisTop + tickLength + gap` 改为 `xAxisTop + tickLength + fontSize + gap`。参考值：tickLength=5px, fontSize=10px, gap=3px → 标签基线应在轴线下方 ~18px。

**验证**：SVG 中 X 轴刻度标签的 Y 坐标 − X 轴线 Y 坐标 ≥ 15px。

---

## BUG-006：Polar 图颜色不匹配

**发现**：客户看到 legend 示意蓝色，但实际曲线是黑色。

**根因**：Polar plot 的 SVG 中有两组渲染：polyline（`stroke="rgb(31,119,180)"` 蓝色）和 circle markers（`fill="rgb(0,0,0)"` 黑色）。曲线颜色是正确的蓝色，但叠加在曲线上的数据点标记是黑色。Legend 的色块是蓝色，与曲线一致，但与客户看到的标记点颜色不一致。问题出在 `polar_plot.cpp` 渲染时 marker 使用了默认 MarkerStyle（fillColor={0,0,0}）而非从 series 继承颜色。

**修复方案**：`polar_plot.cpp` 中渲染 markers 时，从 `data.lineStyle.color` 继承 fillColor 和 edgeColor（与 `scatter_plot.cpp` 相同的逻辑）。

**验证**：SVG 中 circle 元素的 fill 和 stroke 颜色与 legend 色块颜色一致。

---

## BUG-007：Scatter plot 显示为连线

**发现**：客户查看 `02_scatter_plot.svg`，散点图显示为连线而非独立散点。

**根因**：`src/scatter_plot.cpp:53-58` 在 `drawMarkers()` 之后额外调用了 `drawPolyline()`，将所有散点用线连起来。这导致 Scatter plot 变成了 "散点 + 连线"，客户看到的是连线而非独立散点。

**修复方案**：移除 `scatter_plot.cpp` 中连接散点的 `drawPolyline()` 调用。若需要连线，应由用户显式调用 `addLineSeries` 叠加。散点图默认只画标记点。

**验证**：`02_scatter_plot.svg` 中 polyline 元素仅来自网格线和轴线（不来自数据），circle 元素数量 = 数据点数。

**发现**：客户查看 SVG demo，X 轴线不可见。

**根因**：`Plot` 内部 canvas 尺寸默认为 800×600，但 SVG 设备尺寸为 800×500。布局引擎按 600px 高度计算出的 X 轴位置（y≈545）超出了 500px 的 SVG 视口，导致 X 轴线、刻度标签、X 轴标题全部被裁剪。

**影响范围**：所有使用非默认尺寸设备的场景。

**修复方案**：
1. `Plot` 新增 `setCanvasSize(width, height)` 公开方法
2. Gallery 在 `plot.render(svg)` 前调用 `plot.setCanvasSize(800, 500)`
3. 或：`IRenderDevice` 新增 `int width()/height()` 虚方法，`Plot::render()` 自动读取

**验证方式**：SVG 中 X 轴 polyline 的 Y 坐标应在 SVG height 范围内；gallery 全部 8 张图可正常看到 X 轴。

---

## BUG-002：Bar chart 显示为曲线

**发现**：客户查看 `03_bar_chart.svg`，柱状图显示为折线。

**根因**：Gallery 使用 `plot.addLineSeries("Sales", ...)` 添加柱状图数据。`addLineSeries` 创建的是 Line 类型，渲染为折线。`src/bar_plot.cpp` 中的 `BarPlot` 实现正确但无公开 API 可用。

**影响范围**：所有 B1 图类型（Bar/Step/ErrorBar/Histogram/Polar）均无公开 API，客户无法使用。

**修复方案**：
1. `Plot` 新增便捷方法：`addBarSeries()`, `addStepSeries()`, `addAreaSeries()`, `addHistogramSeries()`, `addErrorBarSeries()`, `addPolarSeries()`, `addHeatmapSeries()`, `addContourSeries()` 
2. `SeriesInfo::SeriesType` 扩展枚举：`Bar, Step, Area, Histogram, ErrorBar, Polar, Heatmap, Contour`
3. `Plot::render()` 中根据 `SeriesType` 分发到对应的 `IPlotType`（通过 `PlotRegistry`）
4. Gallery 改用 `addBarSeries()` 等方法

**验证方式**：`03_bar_chart.svg` 包含 `<rect>` 元素（fillRect 调用的 SVG 输出），而非只有 `<polyline>`。

---

## BUG-003：Multi-axis 不显示

**发现**：客户查看 `04_multi_axis.svg`，只有一条 Y 轴，右侧 Y 轴未显示。

**根因**：`Plot::addLineSeries()` 不支持指定 Y 轴索引。`Plot::yAxisAddRight()` 仅添加了右侧轴标签，但所有数据系列都绑定在左轴（yAxisIndex=0）。右侧 Y 轴没有数据 → `hasYAxisRightLabel` 为 true 但布局未分配右轴空间（因为 `maxTickLabelWidth` 为 0 且无数据）。另外，《右轴绑定》功能缺失。

实际代码流程：
1. `yAxisAddRight("Humidity (%)")` → `m_impl->yRightLabel` 被设置 ✅
2. `addLineSeries("Humidity", xs, humidity, 100)` → `yAxisIndex` 始终为 0 ❌
3. layout 中 `hasYAxisRightLabel = !m_impl->yRightLabel.empty()` → true ✅  
4. 但 `yRightTickW = yTickW` (line 143) → 应该有空间
5. 问题是 plot.cpp render 中右轴数据被左轴范围裁剪

**修复方案**：
1. `addLineSeries()` 和 `addScatterSeries()` 新增 `yAxisIndex` 参数（默认 0）
2. Gallery 中 `plot.addLineSeries("Humidity (%)", xs, humidity, 100, 1)` （yAxisIndex=1 表右轴）
3. `Plot::render()` 中根据 `yAxisIndex` 选择对应的 Y 轴范围和刻度

**验证方式**：`04_multi_axis.svg` 可见右侧 Y 轴刻度标签（如 20, 40, 60, 80）。

---

## BUG-004：Gallery 未设置 canvasSize（关联 BUG-001）

**发现**：Gallery 创建 `SvgDevice svg(800, 500, ...)` 但未告诉 `Plot` 实际的像素尺寸。

**修复方案**：Gallery 每个 `make*()` 函数中，`render()` 前调用 `plot.setCanvasSize(800, 500)`。

---

## 修复任务分配

| # | 负责 | 任务 | 预计 |
|---|------|------|------|
| 1 | **Agent E** | BUG-001: Plot 新增 `setCanvasSize()` + 在 render 中应用 | 15min |
| 2 | **Agent A** | BUG-002: 审查新增的 `add*Series()` API 是否符合接口冻结策略 | 15min |
| 3 | **Agent E** | BUG-002: Plot 新增 `addBarSeries()` 等 8 个方法 + SeriesType 扩展 | 30min |
| 4 | **Agent D** | BUG-002: Plot::render() 中按 SeriesType 分发到 IPlotType | 20min |
| 5 | **Agent E** | BUG-003: `addLineSeries/addScatterSeries` 新增 yAxisIndex 参数 | 20min |
| 6 | **Agent F** | BUG-004: Gallery 改用新 API + `setCanvasSize` | 20min |
| 7 | **Agent E** | 为 3 个 Bug 添加回归测试（test_integration.cpp） | 30min |
| 8 | **Agent B** | 最终 gate-check --full | 10min |
