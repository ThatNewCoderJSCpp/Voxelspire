#ifndef VOXELSPIRE_CORE_LIMITS_SKY_HPP
#define VOXELSPIRE_CORE_LIMITS_SKY_HPP

#include "bounds.hpp"

namespace voxelspire {

struct DayCycleLimits {
    static constexpr Bounds real_day_minutes { 0.25, 1440.0 };
    static constexpr Bounds sun_tilt         { 0.0, 1.0 };
    static constexpr Bounds night_brightness { 0.0, 1.0 };
    static constexpr Bounds horizon_fade     { 0.01, 0.5 };
    static constexpr Bounds twilight         { 0.01, 0.5 };
    static constexpr Bounds dusk_sky_mix     { 0.0, 1.0 };
};

struct GlowLimits {
    static constexpr Bounds size     { 1.0, 30.0 };
    static constexpr Bounds strength { 0.0, 1.0 };
    static constexpr Bounds rings    { 1.0, 64.0 };
};

struct SunLimits {
    static constexpr Bounds size           { 0.1, 20.0 };
    static constexpr Bounds segments       { 3.0, 256.0 };
    static constexpr Bounds rings          { 1.0, 64.0 };
    static constexpr Bounds brightness     { 0.0, 1.0 };
    static constexpr Bounds limb_darkening { 0.0, 1.0 };
    static constexpr Bounds edge_softness  { 0.0, 1.0 };
    static constexpr Bounds light_strength { 0.0, 3.0 };
};

struct MoonLimits {
    static constexpr Bounds size           { 0.1, 20.0 };
    static constexpr Bounds segments       { 3.0, 256.0 };
    static constexpr Bounds rings          { 1.0, 64.0 };
    static constexpr Bounds brightness     { 0.0, 1.0 };
    static constexpr Bounds edge_softness  { 0.0, 1.0 };
    static constexpr Bounds day_visibility { 0.0, 1.0 };
    static constexpr Bounds cycle_days     { 1.0, 100.0 };
    static constexpr Bounds phase_offset   { 0.0, 1.0 };
    static constexpr Bounds earthshine     { 0.0, 0.5 };
    static constexpr Bounds dark_opacity   { 0.0, 1.0 };
    static constexpr Bounds terminator     { 0.0, 1.0 };
    static constexpr Bounds light_strength { 0.0, 2.0 };
};

struct StarLimits {
    static constexpr Bounds count                { 0.0, 20000.0 };
    static constexpr Bounds size                 { 0.01, 2.0 };
    static constexpr Bounds size_variation       { 0.0, 1.0 };
    static constexpr Bounds brightness           { 0.0, 2.0 };
    static constexpr Bounds brightness_variation { 0.0, 1.0 };
    static constexpr Bounds color_variation      { 0.0, 1.0 };
    static constexpr Bounds twinkle              { 0.0, 1.0 };
    static constexpr Bounds twinkle_speed        { 0.0, 10.0 };
    static constexpr Bounds day_visibility       { 0.0, 1.0 };
    static constexpr Bounds seed                 { 0.0, 99999.0 };
};

struct CelestialLimits {
    static constexpr Bounds distance     { 0.05, 0.95 };
    static constexpr Bounds horizon_fade { 0.0, 0.3 };
};

} // namespace voxelspire

#endif // VOXELSPIRE_CORE_LIMITS_SKY_HPP