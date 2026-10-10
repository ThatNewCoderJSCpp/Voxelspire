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
    std::string                  group;
    SettingsScope                scope = SettingsScope::Personal;
};

class SettingsRegistry {
public:
    void               set_group(std::string name) { m_group = std::move(name); }
    void               end_group() { m_group.clear(); }
    void               set_scope(SettingsScope scope) noexcept { m_scope = scope; }
    const std::string& group() const noexcept { return m_group; }

    SettingsTabSpec& add_tab(
        const std::string& id, 
        const std::string& name, 
        const std::string& summary,
        SettingsBuilder builder = {}, 
        const std::string& before = std::string()
    );

    bool extend(const std::string& id, SettingsBuilder builder);

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

    std::vector<SettingsTab> build(GameSettings& live, GameSettings& defaults) const;

private:
    std::vector<SettingsTabSpec>::iterator position(const std::string& id);

    std::vector<SettingsTabSpec> m_tabs;
    std::string                  m_group;
    SettingsScope                m_scope = SettingsScope::Personal;
};

} // namespace voxelspire

#endif // VOXELSPIRE_UI_SETTINGS_REGISTRY_HPP