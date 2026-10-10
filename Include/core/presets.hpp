#ifndef VOXELSPIRE_PRESETS_HPP
#define VOXELSPIRE_PRESETS_HPP

#include "preset_list.hpp"
#include "settings.hpp"

namespace voxelspire {

class LightingPresets : public PresetList<LightingSettings> {
public:
    static LightingPresets builtin();
};

class WaterPresets : public PresetList<WaterSettings> {
public:
    static WaterPresets builtin();
};

} // namespace voxelspire

#endif // VOXELSPIRE_PRESETS_HPP