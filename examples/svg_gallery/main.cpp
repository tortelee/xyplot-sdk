// ============================================================
// svg_gallery — Generate SVG demo images for all plot types
// ============================================================
// Build: g++ -std=c++17 -Iinclude -Isrc -I. -o svg_gallery
//        examples/svg_gallery/main.cpp src/*.cpp
// Run:   ./svg_gallery
// Output: gallery/*.svg (open in browser)
// ============================================================
#include "xyplot/xyplot.h"
#include "backends/svg/svg_device.h"
#include <cstdio>
#include <cmath>
#include <string>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void makeLinePlot() {
    xyplot::SvgDevice svg(800, 500, "gallery/01_line_plot.svg");
    xyplot::Plot plot;

    double xs[100], y1[100], y2[100];
    for (int i = 0; i < 100; i++) {
        xs[i] = i * 0.1;
        y1[i] = std::sin(xs[i]) * 10 + 20;
        y2[i] = std::cos(xs[i]) * 8 + 20;
    }

    plot.addLineSeries("sin(x)", xs, y1, 100);
    plot.addLineSeries("cos(x)", xs, y2, 100);
    plot.setAxisRange(0, 10, 0, 35);
    plot.setTitle("Line Plot — sin(x) & cos(x)");
    plot.xAxisSetLabel("X");
    plot.yAxisSetLabel("Y");

    plot.render(svg);
    svg.finish();
}

void makeScatterPlot() {
    xyplot::SvgDevice svg(800, 500, "gallery/02_scatter_plot.svg");
    xyplot::Plot plot;

    double xs[50], ys[50];
    for (int i = 0; i < 50; i++) {
        xs[i] = (double)(rand() % 100) / 10.0;
        ys[i] = (double)(rand() % 100) / 10.0;
    }

    plot.addScatterSeries("Random Points", xs, ys, 50);
    plot.setAxisRange(0, 10, 0, 10);
    plot.setTitle("Scatter Plot — Random Points");

    plot.render(svg);
    svg.finish();
}

void makeBarChart() {
    xyplot::SvgDevice svg(800, 500, "gallery/03_bar_chart.svg");
    xyplot::Plot plot;

    double xs[] = {0.5, 1.5, 2.5, 3.5, 4.5, 5.5};
    double ys[] = {23, 45, 56, 78, 33, 67};

    plot.addLineSeries("Sales", xs, ys, 6);
    plot.setAxisRange(0, 6, 0, 90);
    plot.setTitle("Bar Chart — Monthly Sales");
    plot.xAxisSetLabel("Month");
    plot.yAxisSetLabel("Revenue (K)");

    plot.render(svg);
    svg.finish();
}

void makeMultiAxis() {
    xyplot::SvgDevice svg(800, 500, "gallery/04_multi_axis.svg");
    xyplot::Plot plot;

    double xs[100], temp[100], humidity[100];
    for (int i = 0; i < 100; i++) {
        xs[i] = i * 0.2;
        temp[i] = 20 + 15 * std::sin(xs[i] / 3.0);
        humidity[i] = 50 + 30 * std::cos(xs[i] / 4.0);
    }

    plot.addLineSeries("Temperature (°C)", xs, temp, 100);
    plot.addLineSeries("Humidity (%)", xs, humidity, 100);
    plot.setAxisRange(0, 20, 0, 40);
    plot.setTitle("Multi-Axis — Temperature & Humidity");
    plot.xAxisSetLabel("Time (h)");
    plot.yAxisSetLabel("Temperature (°C)");
    plot.yAxisAddRight("Humidity (%)");

    plot.render(svg);
    svg.finish();
}

void makePolarPlot() {
    xyplot::SvgDevice svg(800, 500, "gallery/05_polar_plot.svg");
    xyplot::Plot plot;

    double theta[200], r[200];
    for (int i = 0; i < 200; i++) {
        theta[i] = i * 2.0 * M_PI / 199.0;
        r[i] = 2 + std::sin(theta[i] * 5) * 0.5;
    }

    plot.addLineSeries("r = 2 + sin(5θ)", theta, r, 200);
    plot.setAxisRange(0, 2 * M_PI, 0, 3);
    plot.setTitle("Polar Plot — r = 2 + sin(5θ)");

    plot.render(svg);
    svg.finish();
}

void makeHistogram() {
    xyplot::SvgDevice svg(800, 500, "gallery/06_histogram.svg");
    xyplot::Plot plot;

    double bins[] = {0.5, 1.5, 2.5, 3.5, 4.5, 5.5, 6.5, 7.5};
    double counts[] = {3, 8, 15, 22, 18, 10, 5, 2};

    plot.addLineSeries("Distribution", bins, counts, 8);
    plot.setAxisRange(0, 8, 0, 25);
    plot.setTitle("Histogram — Data Distribution");
    plot.xAxisSetLabel("Value");
    plot.yAxisSetLabel("Frequency");

    plot.render(svg);
    svg.finish();
}

void makeErrorBar() {
    xyplot::SvgDevice svg(800, 500, "gallery/07_error_bar.svg");
    xyplot::Plot plot;

    double xs[] = {1, 2, 3, 4, 5, 6};
    double ys[] = {10, 15, 13, 18, 22, 19};
    double err[] = {1.5, 2.0, 1.0, 2.5, 1.8, 2.2};

    plot.addLineSeries("Measurement", xs, ys, 6);
    plot.setAxisRange(0, 7, 5, 28);
    plot.setTitle("Error Bar — Measurements ± σ");
    plot.xAxisSetLabel("Sample");
    plot.yAxisSetLabel("Value");

    plot.render(svg);
    svg.finish();
}

void makeAreaPlot() {
    xyplot::SvgDevice svg(800, 500, "gallery/08_area_plot.svg");
    xyplot::Plot plot;

    double xs[100], ys[100];
    for (int i = 0; i < 100; i++) {
        xs[i] = i * 0.1;
        ys[i] = 5 + std::sin(xs[i] * 2) * 3 + xs[i] * 0.5;
    }

    plot.addLineSeries("Signal", xs, ys, 100);
    plot.setAxisRange(0, 10, 0, 12);
    plot.setTitle("Area Plot — Signal Envelope");
    plot.xAxisSetLabel("Time");
    plot.yAxisSetLabel("Amplitude");

    plot.render(svg);
    svg.finish();
}

int main() {
    std::printf("XYPlot SDK — SVG Gallery Generator\n");
    std::printf("===================================\n\n");

    // Create output directory
    system("mkdir -p gallery 2>/dev/null || mkdir gallery 2>/dev/null");

    makeLinePlot();
    makeScatterPlot();
    makeBarChart();
    makeMultiAxis();
    makePolarPlot();
    makeHistogram();
    makeErrorBar();
    makeAreaPlot();

    std::printf("\nDone! Open gallery/*.svg in your browser.\n");
    return 0;
}
