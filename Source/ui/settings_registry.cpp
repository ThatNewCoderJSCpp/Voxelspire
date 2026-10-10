#include "ui/settings_registry.hpp"

namespace voxelspire {

SettingsTabSpec& SettingsRegistry::add_tab(
    const std::string& id, 
    const std::string& name, 
    const std::string& summary,
    SettingsBuilder builder, 
    const std::string& before 
) {
    SettingsTabSpec* existing = find(id);

    if (existing) {
        existing->name = name;
        existing->summary = summary;
        if (builder) existing->builders.push_back(std::move(builder));
        return *existing;
    }

    SettingsTabSpec spec{ id, name, summary, {}, m_group, m_scope };
    if (builder) spec.builders.push_back(std::move(builder));
    auto at = before.empty() ? m_tabs.end() : position(before);
    return *m_tabs.insert(at, std::move(spec));
}

bool SettingsRegistry::extend(const std::string& id, SettingsBuilder builder) {
    SettingsTabSpec* spec = find(id);
    if (!spec || !builder) return false;
    spec->builders.push_back(std::move(builder));
    return true;
}

std::vector<SettingsTab> SettingsRegistry::build(GameSettings& live, GameSettings& defaults) const {
    std::vector<SettingsTab> out;
    out.reserve(m_tabs.size());

    for (const SettingsTabSpec& spec : m_tabs) {
        out.push_back({ spec.id, spec.name, spec.summary, {}, spec.group, spec.scope });

        for (const SettingsBuilder& build : spec.builders) {
            SettingsPage page(out.back(), live, defaults);
            build(page);
        }
    }

    return out;
}

std::vector<SettingsTabSpec>::iterator SettingsRegistry::position(const std::string& id) {
    return std::find_if(m_tabs.begin(), m_tabs.end(), [&id](const SettingsTabSpec& s) { return s.id == id; });
}

} // namespace voxelspire
