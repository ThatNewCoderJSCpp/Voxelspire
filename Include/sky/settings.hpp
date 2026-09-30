#ifndef VOXELSPIRE_SKY_SETTINGS_HPP
#define VOXELSPIRE_SKY_SETTINGS_HPP

#include <cstdint>
#include "../core/types.hpp"

namespace voxelspire {

struct GlowSettings {
    bool   enabled  = true;
    double size     = 5.0;
    double strength = 0.4;
    int    rings    = 12;
};

struct SunSettings {
    bool   visible          = true;
    double size             = 2.5;
    int    segments         = 48;
    int    rings            = 6;
    Color  color            = Color(255, 238, 190);
    Color  dusk_color       = Color(255, 150, 70);
    double brightness       = 1.0;
    double limb_darkening   = 0.3;
    double edge_softness    = 0.08;
    GlowSettings glow;

    bool   emits_light      = true;
    double light_strength   = 1.0;
    Color  light_color      = Color(255, 244, 222);
    Color  dusk_light_color = Color(255, 160, 90);
};

struct MoonSettings {
    bool   visible        = true;
    double size           = 2.8;
    int    segments       = 40;
    int    rings          = 16;
    Color  color          = Color(228, 232, 242);
    double brightness     = 0.95;
    double edge_softness  = 0.05;
    double day_visibility = 0.45;

    bool   phases         = true;
    double cycle_days     = 8.0;
    double phase_offset   = 0.0;
    double earthshine     = 0.06;
    double dark_opacity   = 0.3;
    double terminator     = 0.08;

    GlowSettings glow { true, 3.5, 0.12, 10 };

    bool   emits_light    = true;
    double light_strength = 0.3;
    Color  light_color    = Color(170, 190, 255);
};

struct StarSettings {
    bool          visible              = true;
    int           count                = 1600;
    double        size                 = 0.3;
    double        size_variation       = 0.6;
    double        brightness           = 1.0;
    double        brightness_variation = 0.75;
    double        color_variation      = 0.35;
    Color         color                = Color(255, 255, 255);
    Color         warm_color           = Color(255, 196, 140);
    Color         cool_color           = Color(160, 190, 255);
    double        twinkle              = 0.35;
    double        twinkle_speed        = 1.5;
    double        day_visibility       = 0.0;
    bool          rotate               = true;
    std::uint32_t seed                 = 1337;
};

struct CelestialSettings {
    double       distance        = 0.85;
    double       horizon_fade    = 0.03;
    bool         hide_underwater = true;
    SunSettings  sun;
    MoonSettings moon;
    StarSettings stars;
};

} // namespace voxelspire

#endif // VOXELSPIRE_SKY_SETTINGS_HPP