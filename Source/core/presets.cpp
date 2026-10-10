#include "core/presets.hpp"

namespace voxelspire {

LightingPresets LightingPresets::builtin() {
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

WaterPresets WaterPresets::builtin() {
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

} // namespace voxelspire
