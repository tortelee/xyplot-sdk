// ============================================================
// contour.cpp — ContourPlot 实现 (B2)
// ============================================================
// Owner: Agent D (图类型)
// 职责: 等值线渲染 — Marching Squares + drawPolyline
// 依赖: IPlotType 接口, IRenderDevice (drawPolyline), 坐标变换
//
// Marching Squares 桩 — Agent C 的 contour_algorithm.h 就绪后可替换
// ============================================================
#include "xyplot_internal.h"
#include <vector>
#include <cmath>
#include <algorithm>

namespace xyplot {
namespace internal {

// ============================================================
// Marching Squares 内联实现（Agent C 提供 contour_algorithm.h 后替换）
// ============================================================
namespace {

struct ContourPath {
    std::vector<double> xs;
    std::vector<double> ys;
};

// 单元格顶点索引：
//   (i,j)   —— cellLeft, cellTop
//   (i+1,j) —— cellRight, cellTop
//   (i,j+1) —— cellLeft, cellBottom
//   (i+1,j+1) — cellRight, cellBottom

// 16 种 Marching Squares 情况的线段端点
// 每个 entry: {edge1_from, edge1_to, edge2_from, edge2_to} (×4 个顶点索引)
// 顶点索引: 0=top, 1=right, 2=bottom, 3=left (边中点)
// 负值表示无线段
const int MS_EDGES[16][4] = {
    {-1,-1, -1,-1},  // 0000
    {0, 3, -1,-1},   // 0001: bottom-left
    {3, 2, -1,-1},   // 0010: left-bottom
    {0, 2, -1,-1},   // 0011: bottom-bottom
    {1, 0, -1,-1},   // 0100: top-right
    {1, 3, 0, 3},    // 0101: right-left (saddle, ambiguous → split)
    {1, 2, 3, 2},    // 0110: right-bottom (saddle)
    {1, 2, -1,-1},   // 0111: right-bottom
    {2, 1, -1,-1},   // 1000: bottom-right
    {2, 3, 0, 3},    // 1001: bottom-left (saddle)
    {2, 1, 3, 1},    // 1010: bottom-right (saddle)
    {0, 1, -1,-1},   // 1011: bottom-right
    {0, 1, -1,-1},   // 1100: top-right (same as 1011 case but different topology)
    {3, 1, -1,-1},   // 1101: left-right
    {3, 0, -1,-1},   // 1110: left-top
    {-1,-1, -1,-1},  // 1111
};

// 顶点在边上的位置 (edge index → {x_offset, y_offset})
// edge 0=top, 1=right, 2=bottom, 3=left
inline double edgeX(int cornerCol, int /*cornerRow*/, int edge, double t) {
    switch (edge) {
    case 0: return static_cast<double>(cornerCol) + t;       // top: varies with col
    case 1: return static_cast<double>(cornerCol + 1);       // right
    case 2: return static_cast<double>(cornerCol) + t;       // bottom
    case 3: return static_cast<double>(cornerCol);           // left
    default: return 0.0;
    }
}

inline double edgeY(int cornerCol, int cornerRow, int edge, double t) {
    (void)cornerCol;
    switch (edge) {
    case 0: return static_cast<double>(cornerRow);           // top
    case 1: return static_cast<double>(cornerRow) + t;       // right
    case 2: return static_cast<double>(cornerRow + 1);       // bottom
    case 3: return static_cast<double>(cornerRow) + t;       // left
    default: return 0.0;
    }
}

// 计算边上的插值位置
inline double interp(double v0, double v1, double level) {
    double diff = v1 - v0;
    if (std::abs(diff) < 1e-15) return 0.5;
    return (level - v0) / diff;
}

void marchingSquares(const double* grid, int rows, int cols,
                     double level, std::vector<ContourPath>& outPaths) {
    for (int i = 0; i < rows - 1; i++) {
        for (int j = 0; j < cols - 1; j++) {
            // 四个角的值
            double v00 = grid[i * cols + j];         // top-left
            double v10 = grid[i * cols + j + 1];     // top-right
            double v01 = grid[(i + 1) * cols + j];   // bottom-left
            double v11 = grid[(i + 1) * cols + j + 1]; // bottom-right

            // 计算 case index
            int caseIdx = 0;
            if (v00 >= level) caseIdx |= 1;  // top-left
            if (v10 >= level) caseIdx |= 2;  // top-right
            if (v11 >= level) caseIdx |= 4;  // bottom-right
            if (v01 >= level) caseIdx |= 8;  // bottom-left

            if (caseIdx == 0 || caseIdx == 15) continue;

            // 获取边线段
            auto edges = MS_EDGES[caseIdx];

            // 计算每条线段端点
            ContourPath seg;
            seg.xs.reserve(2);
            seg.ys.reserve(2);

            for (int segIdx = 0; segIdx < 2; segIdx++) {
                int e1 = edges[segIdx * 2];
                int e2 = edges[segIdx * 2 + 1];
                if (e1 < 0 || e2 < 0) continue;

                double t1 = 0.5, t2 = 0.5;
                switch (e1) {
                case 0: t1 = interp(v00, v10, level); break;
                case 1: t1 = interp(v10, v11, level); break;
                case 2: t1 = interp(v01, v11, level); break;
                case 3: t1 = interp(v00, v01, level); break;
                }
                switch (e2) {
                case 0: t2 = interp(v00, v10, level); break;
                case 1: t2 = interp(v10, v11, level); break;
                case 2: t2 = interp(v01, v11, level); break;
                case 3: t2 = interp(v00, v01, level); break;
                }

                seg.xs.push_back(edgeX(j, i, e1, t1));
                seg.ys.push_back(edgeY(j, i, e1, t1));
                seg.xs.push_back(edgeX(j, i, e2, t2));
                seg.ys.push_back(edgeY(j, i, e2, t2));
            }

            if (!seg.xs.empty()) {
                outPaths.push_back(std::move(seg));
            }
        }
    }
}

} // anonymous namespace

// ============================================================
// ContourPlot 实现
// ============================================================
class ContourPlot : public IPlotType {
public:
    const char* typeName() const override { return "Contour"; }

    void render(IRenderDevice& device,
                const SeriesRenderData& data,
                const AxisRenderConfig& axis,
                const DevicePlotArea& area) override {
        (void)axis;
        int rows = data.gridRows;
        int cols = data.gridCols;
        if (rows < 2 || cols < 2 || !data.ys) return;

        // 1. 确定等高线级别
        std::vector<double> levels;
        if (data.contourLevels && data.contourLevelCount > 0) {
            levels.assign(data.contourLevels,
                         data.contourLevels + data.contourLevelCount);
        } else {
            // 自动计算 5 个级别
            double minVal = data.ys[0], maxVal = data.ys[0];
            int totalCells = rows * cols;
            for (int i = 1; i < totalCells; i++) {
                if (data.ys[i] < minVal) minVal = data.ys[i];
                if (data.ys[i] > maxVal) maxVal = data.ys[i];
            }
            double step = (maxVal - minVal) / 6.0;
            if (step <= 0.0) step = 1.0;
            for (int i = 1; i <= 5; i++) {
                levels.push_back(minVal + step * static_cast<double>(i));
            }
        }

        // 2. 计算网格 → 设备坐标的缩放因子
        double scaleX = area.width / static_cast<double>(cols - 1);
        double scaleY = area.height / static_cast<double>(rows - 1);

        // 3. 对每个级别执行 Marching Squares
        LineStyle style = data.lineStyle;
        if (style.width <= 0.0) style.width = 1.0;

        for (size_t li = 0; li < levels.size(); li++) {
            std::vector<ContourPath> paths;
            marchingSquares(data.ys, rows, cols, levels[li], paths);

            for (auto& path : paths) {
                // 映射网格坐标 → 设备坐标
                std::vector<double> dx(path.xs.size());
                std::vector<double> dy(path.ys.size());
                for (size_t k = 0; k < path.xs.size(); k++) {
                    dx[k] = area.left + path.xs[k] * scaleX;
                    dy[k] = area.top + path.ys[k] * scaleY;
                }
                device.drawPolyline(dx.data(), dy.data(),
                                   static_cast<int>(dx.size()), style);
            }
        }
    }
};

std::unique_ptr<IPlotType> createContourPlot() {
    return std::make_unique<ContourPlot>();
}

} // namespace internal
} // namespace xyplot
