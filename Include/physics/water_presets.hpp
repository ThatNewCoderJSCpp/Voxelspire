#ifndef VOXELSPIRE_PHYSICS_WATER_PRESETS_HPP
#define VOXELSPIRE_PHYSICS_WATER_PRESETS_HPP

#include "../core/preset_list.hpp"
#include "water_settings.hpp"

namespace voxelspire {

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

#endif // VOXELSPIRE_PHYSICS_WATER_PRESETS_HPP