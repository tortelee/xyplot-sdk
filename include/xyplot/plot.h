// ============================================================
// xyplot/plot.h — Plot 门面类 (冻结 v1.0)
// ============================================================
#pragma once
#include "types.h"
#include "irender_device.h"
#include "iinput_source.h"

namespace xyplot {

struct InteractionResult {
    enum Action { ViewChanged, DataPicked, CurveSelected, None };
    Action action = None;
    double pickedDataX = 0, pickedDataY = 0;
    int selectedCurveIndex = -1;
};

class Plot {
public:
    Plot();
    ~Plot();

    // 不可拷贝，可移动
    Plot(const Plot&) = delete;
    Plot& operator=(const Plot&) = delete;
    Plot(Plot&&) noexcept;
    Plot& operator=(Plot&&) noexcept;

    // ──── 数据绑定 ────
    int addLineSeries(const char* name,
                      const double* xs, const double* ys, int count);
    int addScatterSeries(const char* name,
                         const double* xs, const double* ys, int count);
    void updateSeriesData(int seriesId,
                          const double* xs, const double* ys, int count);

    // ──── 轴配置 ────
    void xAxisSetLabel(const char* label);
    void yAxisSetLabel(const char* label);
    void yAxisAddRight(const char* label);
    void xAxisSetScale(ScaleType type);
    void setAxisRange(double xMin, double xMax,
                      double yMin, double yMax, int yAxisIndex = 0);

    // ──── 样式 ────
    void setTitle(const char* title);
    void setSeriesStyle(int seriesId, const LineStyle& style);

    // ──── 渲染 ────
    void render(IRenderDevice& device);

    // ──── 交互 ────
    InteractionResult handleEvent(const InputEvent& event,
                                   IRenderDevice* device = nullptr);

private:
    class Impl;
    Impl* m_impl;
};

} // namespace xyplot
