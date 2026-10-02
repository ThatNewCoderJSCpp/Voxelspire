#ifndef VOXELSPIRE_LIGHTING_PRESETS_HPP
#define VOXELSPIRE_LIGHTING_PRESETS_HPP

#include "../core/preset_list.hpp"
#include "settings.hpp"

namespace voxelspire {

class LightingPresets : public PresetList<LightingSettings> {
public:
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
};

} // namespace voxelspire

#endif // VOXELSPIRE_LIGHTING_PRESETS_HPP