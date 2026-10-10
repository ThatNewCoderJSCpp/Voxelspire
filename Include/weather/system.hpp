#ifndef VOXELSPIRE_WEATHER_WEATHER_SYSTEM_HPP
#define VOXELSPIRE_WEATHER_WEATHER_SYSTEM_HPP

#include <cmath>
#include <cstdint>
#include "../core/random.hpp"
#include "../world/biome.hpp"
#include "calendar.hpp"
#include "../core/settings.hpp"

namespace voxelspire {

enum class WeatherKind   : std::uint8_t { Clear = 0, Rain, Storm };
enum class Precipitation : std::uint8_t { None = 0, Rain, Snow, Sleet, FreezingRain, Hail };

const char* weather_name(WeatherKind k) noexcept;

const char* precipitation_name(Precipitation p) noexcept;

struct WeatherState {
    WeatherKind kind      = WeatherKind::Clear;
    double      intensity = 0.0;
    double      target    = 0.0;
    double      remaining = 0.0;
    bool        hail      = false;
};

struct LocalWeather {
    WeatherKind   kind          = WeatherKind::Clear;
    Precipitation precipitation = Precipitation::None;
    double        amount        = 0.0;
    double        cloud         = 0.0;
    double        darkness      = 0.0;
    double        temperature   = 0.0;
    double        flash         = 0.0;
};

struct BiomeWeather {
    static BiomeClimate effective(const Biome& biome, const BiomeOptions& o) noexcept;

    static BiomeClimate blend(const BiomeClimate* climates, std::size_t count) noexcept { return blend(climates, nullptr, count); }

    static BiomeClimate blend(const BiomeClimate* climates, const double* weights, std::size_t count) noexcept;
};

class WeatherSystem {
public:
    static constexpr double HOURS_PER_DAY    = 24.0;
    static constexpr double WARMEST_TIME     = 0.6;
    static constexpr double FULL_TURN        = 2.0 * PI;
    static constexpr double BLOCKS_PER_STEP  = 10.0;
    static constexpr double MIN_RAIN         = 0.45;
    static constexpr double STORM_STRENGTH   = 1.0;
    static constexpr double CLOUD_LEAD       = 1.6;
    static constexpr double FLASH_FADE       = 4.0;
    static constexpr double FREEZING         = 0.0;
    static constexpr double ALWAYS_RAIN      = 0.75;

    const WeatherState& state() const noexcept { return m_state; }
    void set_state(const WeatherState& s) noexcept { m_state = s; }

    double flash() const noexcept { return m_flash; }

    void force(WeatherKind kind, const WeatherSettings& s, SeededRandom& rng) {
        start(kind, s, rng);
        m_state.intensity = m_state.target;
    }

    void cycle(const WeatherSettings& s, SeededRandom& rng);

    void update(double game_days, double real_seconds, const WeatherSettings& s, const CalendarDate& date, SeededRandom& rng);

    static double shower(double game_days, double rainfall, const WeatherSettings& s, double x = 0.0, double y = 0.0) noexcept;

    LocalWeather local(const BiomeClimate& c, double altitude, int sea_level, double time_of_day, double game_days, const CalendarDate& date,
                       const WeatherSettings& s, const SeasonSettings& seasons, double x = 0.0, double y = 0.0) const noexcept;

    Precipitation kind_of(double celsius, const WeatherSettings& s) const noexcept;

private:
    static constexpr int           KIND_COUNT        = 3;
    static constexpr double        HALF              = 0.5;
    static constexpr double        MIN_DIVISOR       = 1e-6;
    static constexpr double        RAIN_CAP          = 1.6;
    static constexpr double        MAX_SHOWER_CHANCE = 0.8;
    static constexpr double        SHOWER_SHARPNESS  = 5.0;
    static constexpr double        DRY_FADE          = 0.6;
    static constexpr std::uint64_t LATTICE_X         = 73856093ull;
    static constexpr std::uint64_t LATTICE_Y         = 19349663ull;
    static constexpr std::uint64_t LATTICE_T         = 83492791ull;
    static constexpr std::uint64_t SHOWER_SALT       = 0x5A0E7B11ull;
    static constexpr double        UNIT_SCALE        = 1.0 / 9007199254740992.0;
    static constexpr int           UNIT_SHIFT        = 11;

    static double lattice(std::int64_t x, std::int64_t y, std::int64_t t) noexcept;

    static double value_noise(double x, double y, double t) noexcept;

    static double hash01(std::int64_t n) noexcept;

    void apply_mode(const WeatherSettings& s, SeededRandom& rng);

    static WeatherKind pick_wet(const WeatherSettings& s, SeededRandom& rng) {
        return rng.unit() < s.storm_chance ? WeatherKind::Storm : WeatherKind::Rain;
    }

    static double lasting(double mean, SeededRandom& rng) {
        return -std::log(vmax(1.0 - rng.unit(), MIN_DIVISOR)) * mean;
    }

    void start(WeatherKind kind, const WeatherSettings& s, SeededRandom& rng, const CalendarDate& date = CalendarDate{});

    WeatherState m_state;
    double       m_flash = 0.0;
};

class PrecipitationEasing {
public:
    void apply(LocalWeather& w, double real_seconds, const WeatherSystem& system, const WeatherSettings& s) noexcept;

private:
    static double approach(double from, double to, double step) noexcept { return from + vclamp(to - from, -step, step); }

    double m_amount   = 0.0;
    double m_cloud    = 0.0;
    double m_darkness = 0.0;
};

} // namespace voxelspire

#endif // VOXELSPIRE_WEATHER_WEATHER_SYSTEM_HPP