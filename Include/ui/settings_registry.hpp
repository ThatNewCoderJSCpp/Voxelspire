#ifndef VOXELSPIRE_UI_SETTINGS_REGISTRY_HPP
#define VOXELSPIRE_UI_SETTINGS_REGISTRY_HPP

#include <algorithm>
#include <functional>
#include <string>
#include <utility>
#include <vector>
#include "settings_model.hpp"

namespace voxelspire {

using SettingsBuilder = std::function<void(SettingsPage&)>;

struct SettingsTabSpec {
    std::string                  id;
    std::string                  name;
    std::string                  summary;
    std::vector<SettingsBuilder> builders;
};

class SettingsRegistry {
public:
    SettingsTabSpec& add_tab(
        const std::string& id, 
        const std::string& name, 
        const std::string& summary,
        SettingsBuilder builder = {}, 
        const std::string& before = std::string()
    ) {
        SettingsTabSpec* existing = find(id);

        if (existing) {
            existing->name = name;
            existing->summary = summary;
            if (builder) existing->builders.push_back(std::move(builder));
            return *existing;
        }

        SettingsTabSpec spec{ id, name, summary, {} };
        if (builder) spec.builders.push_back(std::move(builder));
        auto at = before.empty() ? m_tabs.end() : position(before);
        return *m_tabs.insert(at, std::move(spec));
    }

    bool extend(const std::string& id, SettingsBuilder builder) {
        SettingsTabSpec* spec = find(id);
        if (!spec || !builder) return false;
        spec->builders.push_back(std::move(builder));
        return true;
    }

    bool remove_tab(const std::string& id) {
        auto it = position(id);
        if (it == m_tabs.end()) return false;
        m_tabs.erase(it);
        return true;
    }

    SettingsTabSpec* find(const std::string& id) {
        auto it = position(id);
        return it == m_tabs.end() ? nullptr : &*it;
    }

    const std::vector<SettingsTabSpec>& tabs() const noexcept { return m_tabs; }

    std::vector<SettingsTab> build(GameSettings& live, GameSettings& defaults) const {
        std::vector<SettingsTab> out;
        out.reserve(m_tabs.size());

        for (const SettingsTabSpec& spec : m_tabs) {
            out.push_back({ spec.id, spec.name, spec.summary, {} });

            for (const SettingsBuilder& build : spec.builders) {
                SettingsPage page(out.back(), live, defaults);
                build(page);
            }
        }

        return out;
    }

private:
    std::vector<SettingsTabSpec>::iterator position(const std::string& id) {
        return std::find_if(m_tabs.begin(), m_tabs.end(), [&id](const SettingsTabSpec& s) { return s.id == id; });
    }

    std::vector<SettingsTabSpec> m_tabs;
};

} // namespace voxelspire

#endif // VOXELSPIRE_UI_SETTINGS_REGISTRY_HPP