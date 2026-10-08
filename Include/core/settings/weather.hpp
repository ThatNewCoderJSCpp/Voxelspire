#ifndef VOXELSPIRE_CORE_SETTINGS_WEATHER_HPP
#define VOXELSPIRE_CORE_SETTINGS_WEATHER_HPP

#include <array>
#include <cstddef>
#include <cstdint>

namespace voxelspire {

enum class SeasonMode      : std::uint8_t { Cycle = 0, Fixed, Off };
enum class Season          : std::uint8_t { Spring = 0, Summer, Autumn, Winter };
enum class WeatherMode     : std::uint8_t { Changing = 0, AlwaysClear, AlwaysRain, AlwaysStorm };
enum class RainStyle       : std::uint8_t { Realistic = 0, Simple };
enum class TemperatureUnit : std::uint8_t { Celsius = 0, Fahrenheit };

struct Seasons {
    static constexpr int COUNT = 4;
    static constexpr std::array<const char*, COUNT> NAMES{ "Spring", "Summer", "Autumn", "Winter" };
    static const char* name(Season s) noexcept { return NAMES[static_cast<std::size_t>(s)]; }
};

struct SeasonSettings {
    SeasonMode mode              = SeasonMode::Cycle;
    Season     fixed_season      = Season::Summer;
    Season     start_season      = Season::Spring;
    int        days_per_month    = 8;
    int        months_per_season = 3;
    double     sun_swing         = 23.4;
    double     temperature       = 1.0;
 
    int    days_per_season() const noexcept { return days_per_month * months_per_season; }
    int    days_per_year()   const noexcept { return days_per_season() * Seasons::COUNT; }
    int    months_per_year() const noexcept { return months_per_season * Seasons::COUNT; }
};

struct TemperatureSettings {
    bool   daily_change    = true;
    double daily_strength  = 1.0;
    double altitude_drop   = 1.5;
    double offset          = 0.0;
    double weather_cooling = 4.0;
    double cloud_damping   = 0.6;
    double climate_mix     = 0.5;
    double blend_distance  = 40.0;
    double local_variation = 2.0;
    double local_size      = 12.0;
    double drift           = 3.0;
    double drift_size      = 320.0;
    double drift_speed     = 0.8;
};

struct WeatherSettings {
    static constexpr std::size_t SEASON_COUNT = Seasons::COUNT;
 
    WeatherMode mode            = WeatherMode::Changing;
    RainStyle   style           = RainStyle::Realistic;
    double      clear_days      = 1.2;
    double      rain_days       = 0.35;
    double      storm_chance    = 0.25;
    double      hail_chance     = 0.35;
    double      change_hours    = 1.0;
    double      snow_below      = -1.0;
    double      sleet_below     = 2.5;
    double      hail_above      = 12.0;
    double      dry_below       = 0.15;
    double      darkening       = 0.55;
    double      storm_darkening = 0.8;
    double      fog             = 3.0;
    bool        lightning       = true;
    double      lightning_rate  = 6.0;
    double      wind            = 1.0;
    double      showers         = 0.35;
    double      shower_hours    = 3.0;
    double      shower_size     = 160.0;
    double      rain_fade       = 8.0;
    std::array<double, SEASON_COUNT> season_rain{ 1.3, 0.9, 1.1, 1.0 };
    TemperatureSettings temperature;
};

struct WeatherViewSettings {
    bool            precipitation = true;
    double          amount        = 1.0;
    int             max_drops     = 6000;
    double          radius        = 22.0;
    bool            flashes       = true;
    TemperatureUnit unit          = TemperatureUnit::Fahrenheit;
};

} // namespace voxelspire

#endif // VOXELSPIRE_CORE_SETTINGS_WEATHER_HPP