# XYPlot SDK — 后端无关的 C++ 嵌入式绘图库

[![License](https://img.shields.io/badge/license-BSD-blue.svg)](LICENSE)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![Build](https://img.shields.io/badge/build-CMake-green.svg)](https://cmake.org/)

**XYPlot** 是一个零强制外部依赖的 C++17 嵌入式绘图 SDK。通过两个抽象接口 — `IRenderDevice`（渲染设备）和 `IInputSource`（输入源）— 与客户任意显示/交互系统对接。核心引擎纯计算，不含任何 UI 框架绑定。

---

## 🎯 定位

> 一个 **零 UI 依赖的 C++ 嵌入式绘图 SDK**。客户只需实现 ~8 个渲染方法（~200 行代码），即可将图表嵌入到自己的仿真软件、数据平台或 GUI 应用中。

**适用场景**：仿真软件、数据采集系统、科学计算平台 — 任何需要"在自己的窗口里画图"但不想绑定特定 UI 框架的场景。

---

## ✨ 核心特性

| 特性 | 说明 |
|------|------|
| **后端无关** | 通过 `IRenderDevice` 抽象接口对接任意渲染后端 (Qt, MFC, OpenGL, Skia, 自定义引擎…) |
| **零强制依赖** | 核心库仅依赖 C++17 标准库 — 不强制引入 Qt, VTK, OpenGL 等 |
| **接口极简** | P0 MVP 仅 **8 个纯虚方法** + 1 个可选方法 |
| **交互自由** | `IInputSource` 统一鼠标/键盘/触摸事件，客户完全控制输入模型 |
| **跨平台** | Windows (MSVC 2022+), Linux (GCC 13+/Clang 16+)，CMake 构建 |
| **轻量** | 公开头文件 < 10 个，核心源码 < 5000 行 |

---

## 🚀 快速开始

### 依赖

- **CMake** ≥ 3.20
- **C++17** 编译器 (MSVC 2022 / GCC 13+ / Clang 16+)
- **无需其他依赖**

### 构建 & 安装

```bash
# 1. 配置
cmake -B build -DCMAKE_BUILD_TYPE=Release

# 2. 构建
cmake --build build --config Release

# 3. 运行测试
cd build && ctest --output-on-failure

# 4. 安装 (可选)
cmake --install build --prefix /path/to/install
```

### 第一个绘图程序（~30 行）

```cpp
#include <xyplot/xyplot.h>
#include <cstdio>

// 1. 实现最小 IRenderDevice（控制台打印）
struct ConsoleDevice : public xyplot::IRenderDevice {
    void beginFrame() override { printf("=== beginFrame ===\n"); }
    void endFrame() override   { printf("=== endFrame ===\n"); }
    void setClipRect(double x, double y, double w, double h) override {
        printf("  clip: [%.0f, %.0f, %.0f, %.0f]\n", x, y, w, h);
    }
    void resetClip() override {}
    void drawPolyline(const double* xs, const double* ys, int count,
                      const xyplot::LineStyle& style) override {
        printf("  polyline: %d points, width=%.1f\n", count, style.width);
    }
    void drawMarkers(const double* xs, const double* ys, int count,
                     const xyplot::MarkerStyle& style) override {
        printf("  markers: %d points, size=%.1f\n", count, style.size);
    }
    void drawText(double x, double y, const char* text,
                  const xyplot::FontDesc& font,
                  const xyplot::TextStyle& style) override {
        printf("  text: \"%s\" at (%.0f, %.0f)\n", text, x, y);
    }
    void fillRect(double x, double y, double w, double h,
                  const xyplot::FillStyle& style) override {
        printf("  fillRect: [%.0f, %.0f, %.0f, %.0f]\n", x, y, w, h);
    }
};

// 2. 创建 Plot，添加数据，渲染
int main() {
    xyplot::Plot plot;

    // 数据
    double xs[] = {1.0, 2.0, 3.0, 4.0, 5.0};
    double ys[] = {2.0, 4.0, 1.0, 8.0, 7.0};
    plot.addLineSeries("My Data", xs, ys, 5);
    plot.addScatterSeries("Points", xs, ys, 5);

    // 样式
    plot.setTitle("Hello XYPlot!");
    plot.xAxisSetLabel("X Axis");
    plot.yAxisSetLabel("Y Axis");

    // 渲染
    ConsoleDevice device;
    plot.render(device);

    return 0;
}
```

编译 & 运行（假设 `console_example.cpp` 在当前目录）：

```bash
g++ -std=c++17 -I/path/to/xyplot/include console_example.cpp -L/path/to/xyplot/build -lxyplot -o console_example
./console_example
```

---

## 📁 项目结构

```
xyplot/
├── include/xyplot/          # 公开头文件（客户只需 #include <xyplot/xyplot.h>）
│   ├── types.h              #   基础类型: Color, Rect, LineStyle, MarkerStyle...
│   ├── irender_device.h     #   渲染设备抽象接口 (8+1 方法)
│   ├── iinput_source.h      #   输入事件抽象接口
│   ├── plot.h               #   Plot 门面类
│   └── xyplot.h             #   主头文件 (include-all)
├── src/                     # 核心实现
│   ├── plot.cpp             #   Plot 门面: render(), handleEvent()
│   ├── datatable.cpp        #   数据存储 & CSV 解析
│   ├── axis_system.cpp      #   Nice Number 自动刻度算法
│   ├── coordinate_transform.cpp  # 线性/对数坐标变换
│   ├── layout_engine.cpp    #   标题/轴/图例/绘图区自动布局
│   ├── line_plot.cpp        #   折线图
│   ├── scatter_plot.cpp     #   散点图
│   ├── multi_axis.cpp       #   多 Y 轴管理
│   ├── legend_renderer.cpp  #   图例渲染
│   ├── plot_registry.cpp    #   图类型注册中心
│   └── hit_test.cpp         #   HitTest 系统
├── tests/                   # 单元测试
│   ├── test_interface_contract.cpp  #  🔒 接口契约测试（CI 门禁）
│   ├── test_axis.cpp                #  轴系统测试
│   ├── test_transform.cpp           #  坐标变换测试
│   └── test_plots.cpp               #  图类型集成测试
├── examples/                # 示例代码
│   ├── minimal/             #   最小示例（控制台设备）
│   ├── qt_backend/          #   Qt 集成示例
│   └── blend2d_demo/        #   Blend2D 渲染 Demo
├── docs/                    # 项目文档
│   ├── API_REFERENCE.md     #   API 参考文档
│   ├── interface-freeze.md  #   接口冻结清单
│   └── 04-roadmap.md        #   执行路线图
└── CMakeLists.txt           # 构建系统
```

---

## 🏗️ 架构设计

```
  客户的仿真软件
  ┌──────────────────────────────────────────┐
  │                                          │
  │  客户数据 ──►  Plot SDK (我们的库)         │
  │                 │                        │
  │                 ├─► IRenderDevice  ◄─────┼── 客户实现 (Qt/MFC/OpenGL/...)
  │                 │   (8 纯虚方法)           │
  │                 │                        │
  │  客户输入 ──►    ├─► IInputSource   ◄─────┼── 客户实现 (鼠标/键盘/触摸)
  │                 │   (可选)                │
  │                 │                        │
  │                 ▼                        │
  │            像素输出到客户的窗口             │
  └──────────────────────────────────────────┘
```

**核心思想**：SDK 只负责"计算要画什么"（坐标变换、刻度计算、布局），不负责"怎么画"（实际像素绘制）。"怎么画"由客户通过 `IRenderDevice` 接口实现。

---

## 📖 文档

| 文档 | 内容 |
|------|------|
| [API_REFERENCE.md](docs/API_REFERENCE.md) | 完整 API 参考 — 所有公开类型、接口、方法 |
| [interface-freeze.md](docs/interface-freeze.md) | 接口冻结清单 — P0 方法覆盖矩阵 |
| [04-roadmap.md](docs/04-roadmap.md) | 执行路线图 — 1 周冲刺计划 |
| [00-executive-summary.md](docs/00-executive-summary.md) | 项目执行摘要 |

---

## ✅ 支持的图类型

| 图类型 | P0 (MVP) | P1 (V1.0) | P2 (V2.0) |
|--------|----------|-----------|-----------|
| Line Plot (折线图) | ✅ | — | — |
| Scatter Plot (散点图) | ✅ | — | — |
| Multi Y-Axis (多Y轴) | ✅ | — | — |
| Bar Chart (柱状图) | — | 📅 | — |
| Area Plot (面积图) | — | 📅 | — |
| Error Bar (误差棒) | — | 📅 | — |
| Heatmap (热力图) | — | — | 📅 |
| 3D Surface | — | — | 📅 |

---

## 📄 许可证

BSD 许可证 — 商用友好。

---

## 🤝 贡献

欢迎通过 Issue / PR 贡献。详见项目文档中的 Agent 协作方案。
