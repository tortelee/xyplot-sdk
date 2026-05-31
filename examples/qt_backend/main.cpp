// ============================================================
// examples/qt_backend/main.cpp — XYPlot Qt 集成示例
// ============================================================
// Owner: Agent F (文档 & 示例)
//
// 此示例展示如何将 XYPlot SDK 嵌入到 Qt Widgets 应用中:
//   1. 使用 QtRenderDevice (qt_render_device.h) 在 QPainter 上渲染
//   2. 通过 paintEvent 驱动渲染
//   3. 通过 IInputSource 转发 Qt 鼠标事件到 Plot
//
// 依赖: Qt5 或 Qt6 (Widgets 模块)
//
// 编译 (使用 CMake):
//   在 CMakeLists.txt 中启用 examples/qt_backend 部分即可自动构建。
//
// 编译 (手动, Qt6):
//   g++ -std=c++17 -I../../include -I. main.cpp \
//       $(pkg-config --cflags --libs Qt6Widgets) \
//       -L../../build -lxyplot -o xyplot_qt_example
//
// 运行:
//   ./xyplot_qt_example
// ============================================================

#include "qt_render_device.h"

#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QResizeEvent>
#include <QPainter>
#include <QVBoxLayout>
#include <QLabel>
#include <QStatusBar>

#include <xyplot/xyplot.h>

#include <vector>
#include <cmath>
#include <string>

// ═══════════════════════════════════════════════════════════
// QtInputSource — 将 Qt 输入事件适配为 XYPlot InputEvent
// ═══════════════════════════════════════════════════════════
class QtInputSource : public xyplot::IInputSource {
public:
    /// 将 Qt 鼠标事件压入事件队列
    void pushMouseEvent(xyplot::InputEvent::Type type,
                        const QPointF& pos,
                        Qt::MouseButton button,
                        Qt::KeyboardModifiers modifiers) {
        xyplot::InputEvent ev;
        ev.type = type;
        ev.x = pos.x();
        ev.y = pos.y();

        switch (button) {
        case Qt::LeftButton:  ev.button = 0; break;
        case Qt::MiddleButton: ev.button = 1; break;
        case Qt::RightButton:  ev.button = 2; break;
        default:               ev.button = 0; break;
        }

        if (modifiers & Qt::ShiftModifier)   ev.modifiers |= 1;
        if (modifiers & Qt::ControlModifier) ev.modifiers |= 2;
        if (modifiers & Qt::AltModifier)     ev.modifiers |= 4;

        m_queue.push_back(ev);
    }

    /// 压入滚轮事件
    void pushWheelEvent(const QPointF& pos, double delta) {
        xyplot::InputEvent ev;
        ev.type = xyplot::InputEvent::MouseWheel;
        ev.x = pos.x();
        ev.y = pos.y();
        ev.wheelDelta = delta;
        m_queue.push_back(ev);
    }

    // ── IInputSource 接口 ──
    bool pollEvent(xyplot::InputEvent& out) override {
        if (m_queue.empty()) return false;
        out = m_queue.front();
        m_queue.erase(m_queue.begin());
        return true;
    }

private:
    std::vector<xyplot::InputEvent> m_queue;
};

// ═══════════════════════════════════════════════════════════
// PlotWidget — 包含 XYPlot 的 QWidget
// ═══════════════════════════════════════════════════════════
class PlotWidget : public QWidget {
    Q_OBJECT

public:
    explicit PlotWidget(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        // 启用鼠标追踪（用于 hover 效果）
        setMouseTracking(true);
        setFocusPolicy(Qt::StrongFocus);

        // 设置最小尺寸
        setMinimumSize(400, 300);

        // -- 构建演示图表 --
        buildDemoPlot();
    }

protected:
    // ── 绘制 ──
    void paintEvent(QPaintEvent* event) override {
        (void)event;

        QPainter painter(this);
        painter.fillRect(rect(), QColor(248, 248, 248)); // 窗口背景

        // 根据窗口大小更新 Plot 的目标画布
        // (Plot 内部使用固定 800×600，这里展示完整 Qt 集成)

        QtRenderDevice device(&painter);
        m_plot.render(device);
    }

    // ── 大小变化时触发重绘 ──
    void resizeEvent(QResizeEvent* event) override {
        (void)event;
        update();  // 触发 paintEvent
    }

    // ── 鼠标事件 → IInputSource ──
    void mousePressEvent(QMouseEvent* event) override {
        m_input.pushMouseEvent(
            xyplot::InputEvent::MouseDown,
            event->position(),
            event->button(),
            event->modifiers());

        processInput();
    }

    void mouseReleaseEvent(QMouseEvent* event) override {
        m_input.pushMouseEvent(
            xyplot::InputEvent::MouseUp,
            event->position(),
            event->button(),
            event->modifiers());

        processInput();
    }

    void mouseMoveEvent(QMouseEvent* event) override {
        m_input.pushMouseEvent(
            xyplot::InputEvent::MouseMove,
            event->position(),
            event->button(),
            event->modifiers());

        processInput();
    }

    void wheelEvent(QWheelEvent* event) override {
        m_input.pushWheelEvent(
            event->position(),
            event->angleDelta().y() / 120.0);  // 转换为"步"数

        processInput();
    }

private:
    xyplot::Plot m_plot;
    QtInputSource m_input;

    /// 构建演示用的 Plot 数据
    void buildDemoPlot() {
        // -- 正弦波 (折线图) --
        constexpr int N = 200;
        std::vector<double> xs(N), sinYs(N), cosYs(N);
        for (int i = 0; i < N; ++i) {
            xs[i] = static_cast<double>(i) * 4.0 * 3.14159265 / N;
            sinYs[i] = std::sin(xs[i]);
            cosYs[i] = std::cos(xs[i]);
        }
        m_plot.addLineSeries("sin(x)", xs.data(), sinYs.data(), N);

        int cosId = m_plot.addLineSeries("cos(x)", xs.data(), cosYs.data(), N);
        xyplot::LineStyle cosStyle;
        cosStyle.color = xyplot::Color{255, 127, 14};
        cosStyle.width = 2.0;
        cosStyle.dash = xyplot::LineStyle::DashLine;
        m_plot.setSeriesStyle(cosId, cosStyle);

        // -- 散点: 采样点 --
        std::vector<double> sx(20), sy(20);
        for (int i = 0; i < 20; ++i) {
            sx[i] = static_cast<double>(i) * 0.65;
            sy[i] = std::sin(sx[i]) + ((i % 3) - 1) * 0.2;
        }
        m_plot.addScatterSeries("Sampled", sx.data(), sy.data(), 20);

        // -- 样式 --
        m_plot.setTitle("XYPlot Qt Integration Demo");
        m_plot.xAxisSetLabel("x (radians)");
        m_plot.yAxisSetLabel("Amplitude");
    }

    /// 处理输入事件并更新
    void processInput() {
        xyplot::InputEvent ev;
        if (m_input.pollEvent(ev)) {
            // 创建临时 QPainter 用于可选的高亮渲染
            // (handleEvent 的 device 参数用于渲染交互反馈)
            QtRenderDevice* dev = nullptr;

            auto result = m_plot.handleEvent(ev, dev);

            // 根据交互结果显示状态
            if (result.action == xyplot::InteractionResult::CurveSelected) {
                // 可在状态栏显示选中的曲线信息
                setToolTip(QString("Curve %1 selected").arg(result.selectedCurveIndex));
            } else if (result.action == xyplot::InteractionResult::DataPicked) {
                setToolTip(QString("Picked: (%1, %2)")
                    .arg(result.pickedDataX, 0, 'f', 3)
                    .arg(result.pickedDataY, 0, 'f', 3));
            }

            update();  // 触发重绘
        }
    }
};

// ═══════════════════════════════════════════════════════════
// MainWindow
// ═══════════════════════════════════════════════════════════
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow() {
        setWindowTitle("XYPlot SDK — Qt Integration Demo");

        m_plotWidget = new PlotWidget(this);
        setCentralWidget(m_plotWidget);

        // 状态栏提示
        statusBar()->showMessage(
            "Mouse: click legend items to select curves | "
            "Move mouse over data points | Scroll to zoom (future)");

        resize(900, 650);
    }

private:
    PlotWidget* m_plotWidget = nullptr;
};

// ═══════════════════════════════════════════════════════════
// main
// ═══════════════════════════════════════════════════════════
int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("XYPlot Qt Demo");

    MainWindow window;
    window.show();

    return app.exec();
}

#include "main.moc"
