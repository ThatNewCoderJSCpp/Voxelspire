#ifndef VOXELSPIRE_CORE_LIMITS_WEATHER_HPP
#define VOXELSPIRE_CORE_LIMITS_WEATHER_HPP

#include "bounds.hpp"

namespace voxelspire {

struct SeasonLimits {
    static constexpr Bounds days_per_month    { 1.0, 100.0 };
    static constexpr Bounds months_per_season { 1.0, 12.0 };
    static constexpr Bounds sun_swing         { 0.0, 45.0 };
    static constexpr Bounds temperature       { 0.0, 3.0 };
};

struct TemperatureLimits {
    static constexpr Bounds daily_strength  { 0.0, 3.0 };
    static constexpr Bounds altitude_drop   { 0.0, 5.0 };
    static constexpr Bounds offset          { -40.0, 40.0 };
    static constexpr Bounds weather_cooling { 0.0, 20.0 };
    static constexpr Bounds cloud_damping   { 0.0, 1.0 };
    static constexpr Bounds climate_mix     { 0.0, 1.0 };
    static constexpr Bounds blend_distance  { 0.0, 256.0 };
    static constexpr Bounds local_variation { 0.0, 15.0 };
    static constexpr Bounds local_size      { 2.0, 128.0 };
    static constexpr Bounds drift           { 0.0, 20.0 };
    static constexpr Bounds drift_size      { 32.0, 4096.0 };
    static constexpr Bounds drift_speed     { 0.0, 10.0 };
};

struct WeatherLimits {
    static constexpr Bounds clear_days      { 0.02, 30.0 };
    static constexpr Bounds rain_days       { 0.02, 10.0 };
    static constexpr Bounds chance          { 0.0, 1.0 };
    static constexpr Bounds change_hours    { 0.0, 12.0 };
    static constexpr Bounds threshold       { -30.0, 40.0 };
    static constexpr Bounds dry_below       { 0.0, 1.0 };
    static constexpr Bounds darkening       { 0.0, 1.0 };
    static constexpr Bounds fog             { 0.0, 20.0 };
    static constexpr Bounds lightning_rate  { 0.0, 120.0 };
    static constexpr Bounds wind            { 0.0, 5.0 };
    static constexpr Bounds season_rain     { 0.0, 5.0 };
    static constexpr Bounds showers         { 0.0, 2.0 };
    static constexpr Bounds shower_hours    { 0.25, 24.0 };
    static constexpr Bounds shower_size     { 16.0, 2048.0 };
    static constexpr Bounds rain_fade       { 0.0, 60.0 };
};

struct WeatherViewLimits {
    static constexpr Bounds amount    { 0.0, 3.0 };
    static constexpr Bounds max_drops { 100.0, 50000.0 };
    static constexpr Bounds radius    { 6.0, 64.0 };
};

} // namespace voxelspire

#endif // VOXELSPIRE_CORE_LIMITS_WEATHER_HPP