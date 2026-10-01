#ifndef VOXELSPIRE_LIGHTING_PRESETS_HPP
#define VOXELSPIRE_LIGHTING_PRESETS_HPP

#include <string>
#include <vector>
#include "settings.hpp"

namespace voxelspire {

class LightingPresets {
public:
    static constexpr int NOT_FOUND = -1;

    static LightingPresets builtin() {
        LightingPresets p;

        for (const LightingSettings& s : { 
            LightingSettings::off(), 
            LightingSettings::basic(), 
            LightingSettings::classic(),
            LightingSettings::dynamic(), 
            LightingSettings::realistic(), 
            LightingSettings::ultra() 
        }) p.add(s);
        
        return p;
    }

    void add(const LightingSettings& preset) {
        const int at = index_of(preset.name);
        if (at == NOT_FOUND) m_presets.push_back(preset);
        else m_presets[static_cast<std::size_t>(at)] = preset;
    }

    void add_before(const std::string& existing, const LightingSettings& preset) {
        remove(preset.name);
        const int at = index_of(existing);
        if (at == NOT_FOUND) m_presets.push_back(preset);
        else m_presets.insert(m_presets.begin() + at, preset);
    }

    bool remove(const std::string& name) {
        const int at = index_of(name);
        if (at == NOT_FOUND) return false;
        m_presets.erase(m_presets.begin() + at);
        return true;
    }

    int index_of(const std::string& name) const noexcept {
        for (std::size_t i = 0; i < m_presets.size(); ++i) if (m_presets[i].name == name) return static_cast<int>(i);
        return NOT_FOUND;
    }

    const LightingSettings* find(const std::string& name) const noexcept {
        const int at = index_of(name);
        return at == NOT_FOUND ? nullptr : &m_presets[static_cast<std::size_t>(at)];
    }

    const LightingSettings* at(int index) const noexcept {
        return index >= 0 && static_cast<std::size_t>(index) < m_presets.size() ? &m_presets[static_cast<std::size_t>(index)] : nullptr;
    }

    const LightingSettings* next(const std::string& current) const noexcept {
        if (m_presets.empty()) return nullptr;
        const int at = index_of(current);
        return &m_presets[at == NOT_FOUND ? 0 : (static_cast<std::size_t>(at) + 1) % m_presets.size()];
    }

    std::vector<std::string> names() const {
        std::vector<std::string> out;
        out.reserve(m_presets.size());
        for (const LightingSettings& p : m_presets) out.push_back(p.name);
        return out;
    }

    const std::vector<LightingSettings>& all() const noexcept { return m_presets; }
    std::size_t size() const noexcept { return m_presets.size(); }

private:
    std::vector<LightingSettings> m_presets;
};

} // namespace voxelspire

#endif // VOXELSPIRE_LIGHTING_PRESETS_HPP