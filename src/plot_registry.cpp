// ============================================================
// plot_registry.cpp — 图类型注册中心
// ============================================================
// Owner: Agent D (图类型)
// 职责: 维护图类型名称 → 工厂函数的映射，支持运行时注册和创建
// 使用: Agent E 通过此注册中心获取 IPlotType 实例
// ============================================================
#include "xyplot_internal.h"
#include <unordered_map>

namespace xyplot {
namespace internal {

// ============================================================
// PlotRegistry 单例（仅在 .cpp 内部可见）
// ============================================================
class PlotRegistry {
public:
    using Factory = PlotTypeFactory;

    static PlotRegistry& instance() {
        static PlotRegistry registry;
        return registry;
    }

    bool registerType(const std::string& name, Factory factory) {
        if (!factory) return false;
        return m_factories.emplace(name, std::move(factory)).second;
    }

    std::unique_ptr<IPlotType> create(const std::string& name) const {
        auto it = m_factories.find(name);
        if (it == m_factories.end()) return nullptr;
        return it->second();
    }

    std::vector<std::string> registeredNames() const {
        std::vector<std::string> names;
        names.reserve(m_factories.size());
        for (auto& kv : m_factories) {
            names.push_back(kv.first);
        }
        return names;
    }

private:
    PlotRegistry() { registerBuiltins(); }
    PlotRegistry(const PlotRegistry&) = delete;
    PlotRegistry& operator=(const PlotRegistry&) = delete;

    void registerBuiltins() {
        registerType("Line", createLinePlot);
        registerType("Scatter", createScatterPlot);
    }

    std::unordered_map<std::string, Factory> m_factories;
};

// ============================================================
// 对外的便捷函数
// ============================================================
std::unique_ptr<IPlotType> createPlotType(const std::string& name) {
    return PlotRegistry::instance().create(name);
}

bool registerPlotType(const std::string& name, PlotTypeFactory factory) {
    return PlotRegistry::instance().registerType(name, std::move(factory));
}

std::vector<std::string> listPlotTypes() {
    return PlotRegistry::instance().registeredNames();
}

} // namespace internal
} // namespace xyplot
