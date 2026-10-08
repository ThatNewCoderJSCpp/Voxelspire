#ifndef VOXELSPIRE_CORE_SETTINGS_SKY_HPP
#define VOXELSPIRE_CORE_SETTINGS_SKY_HPP

#include <cstdint>
#include "../types.hpp"

namespace voxelspire {

struct DayCycleSettings {
    static constexpr double REAL_SECONDS_PER_MINUTE = 60.0;
    static constexpr double HOURS_PER_DAY           = 24.0;
    static constexpr double MINUTES_PER_HOUR        = 60.0;
    static constexpr double SECONDS_PER_MINUTE      = 60.0;
 
    bool   enabled          = true;
    double real_day_minutes = 20.0;
    double start_time       = 0.3;
    double sun_tilt         = 0.35;
    double night_brightness = 0.2;
    double horizon_fade     = 0.12;
    double twilight         = 0.2;
    double dusk_sky_mix     = 0.6;
    Color  day_sky          = Color(135, 190, 255);
    Color  night_sky        = Color(8, 11, 28);
    Color  dusk_sky         = Color(250, 140, 80);
    Color  day_light_tint   = Color(255, 255, 255);
    Color  night_light_tint = Color(130, 150, 255);
    Color  day_zenith       = Color(62, 118, 228);
    Color  day_horizon      = Color(172, 208, 250);
    Color  night_zenith     = Color(3, 5, 16);
    Color  night_horizon    = Color(14, 20, 44);
    Color  dusk_horizon     = Color(255, 150, 90);
    Color  sun_glow         = Color(255, 214, 160);
    Color  dusk_glow        = Color(255, 120, 60);
 
    double real_day_seconds()  const noexcept { return real_day_minutes * REAL_SECONDS_PER_MINUTE; }
    double real_hour_seconds() const noexcept { return real_day_seconds() / HOURS_PER_DAY; }
};

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

#endif // VOXELSPIRE_CORE_SETTINGS_SKY_HPP