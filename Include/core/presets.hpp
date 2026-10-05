#ifndef VOXELSPIRE_PRESETS_HPP
#define VOXELSPIRE_PRESETS_HPP

#include "preset_list.hpp"
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

class WaterPresets : public PresetList<WaterSettings> {
public:
    static WaterPresets builtin() {
        WaterPresets p;

        for (const WaterSettings& s : {
            WaterSettings::still(),
            WaterSettings::minecraft(),
            WaterSettings::flowing(),
            WaterSettings::realistic(),
            WaterSettings::ultra()
        }) p.add(s);

        return p;
    }
};

} // namespace voxelspire

#endif // VOXELSPIRE_PRESETS_HPP