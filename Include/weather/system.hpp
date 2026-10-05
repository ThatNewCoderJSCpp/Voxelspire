#ifndef VOXELSPIRE_WEATHER_WEATHER_SYSTEM_HPP
#define VOXELSPIRE_WEATHER_WEATHER_SYSTEM_HPP

#include <cmath>
#include <cstdint>
#include "../core/random.hpp"
#include "../world/biome.hpp"
#include "../core/settings.hpp"
#include "calendar.hpp"

namespace voxelspire {

enum class WeatherKind   : std::uint8_t { Clear = 0, Rain, Storm };
enum class Precipitation : std::uint8_t { None = 0, Rain, Snow, Sleet, FreezingRain, Hail };

inline const char* weather_name(WeatherKind k) noexcept {
    switch (k) {
        case WeatherKind::Clear: return "Clear";
        case WeatherKind::Rain:  return "Rain";
        case WeatherKind::Storm: return "Storm";
    }

    return "Clear";
}

inline const char* precipitation_name(Precipitation p) noexcept {
    switch (p) {
        case Precipitation::None:         return "none";
        case Precipitation::Rain:         return "rain";
        case Precipitation::Snow:         return "snow";
        case Precipitation::Sleet:        return "sleet";
        case Precipitation::FreezingRain: return "freezing rain";
        case Precipitation::Hail:         return "hail";
    }

    return "none";
}

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
    static BiomeClimate effective(const Biome& biome, const BiomeOptions& o) noexcept {
        BiomeClimate c = biome.climate();
        if (o.mean_temperature) c.temperature  = *o.mean_temperature;
        if (o.daily_swing)      c.daily_swing  = *o.daily_swing;
        if (o.season_swing)     c.season_swing = *o.season_swing;
        if (o.rainfall)         c.rainfall     = *o.rainfall;
        if (o.waves)            c.waves        = *o.waves;
        return c;
    }

    static BiomeClimate blend(const BiomeClimate* climates, std::size_t count) noexcept {
        BiomeClimate out{ 0.0, 0.0, 0.0, 0.0, 0.0, Color(0, 0, 0) };
        if (count == 0) return BiomeClimate{};
        double red = 0.0, green = 0.0, blue = 0.0;

        for (std::size_t i = 0; i < count; ++i) {
            out.temperature  += climates[i].temperature;
            out.daily_swing  += climates[i].daily_swing;
            out.season_swing += climates[i].season_swing;
            out.rainfall     += climates[i].rainfall;
            out.waves        += climates[i].waves;
            red   += climates[i].water.red();
            green += climates[i].water.green();
            blue  += climates[i].water.blue();
        }

        const double n = static_cast<double>(count);
        auto channel = [n](double v) { return static_cast<std::uint8_t>(std::lround(v / n)); };
        return { out.temperature / n, out.daily_swing / n, out.season_swing / n, out.rainfall / n, out.waves / n, Color(channel(red), channel(green), channel(blue)) };
    }
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

    void cycle(const WeatherSettings& s, SeededRandom& rng) {
        const WeatherKind next = static_cast<WeatherKind>((static_cast<int>(m_state.kind) + 1) % KIND_COUNT);
        force(next, s, rng);
    }

    void update(double game_days, double real_seconds, const WeatherSettings& s, const CalendarDate& date, SeededRandom& rng) {
        apply_mode(s, rng);

        if (s.mode == WeatherMode::Changing) {
            m_state.remaining -= game_days;
            if (m_state.remaining <= 0.0) start(m_state.kind == WeatherKind::Clear ? pick_wet(s, rng) : WeatherKind::Clear, s, rng, date);
        }

        const double change_days = s.change_hours / HOURS_PER_DAY;
        const double step = change_days > 0.0 ? game_days / change_days : 1.0;
        const double gap = m_state.target - m_state.intensity;
        m_state.intensity = std::fabs(gap) <= step ? m_state.target : m_state.intensity + (gap > 0.0 ? step : -step);

        m_flash = vmax(0.0, m_flash - real_seconds * FLASH_FADE);
        const double hours = game_days * HOURS_PER_DAY;

        if (s.lightning && m_state.kind == WeatherKind::Storm && m_state.intensity > MIN_RAIN && hours > 0.0)
            if (rng.unit() < 1.0 - std::exp(-s.lightning_rate * hours * m_state.intensity)) m_flash = 1.0;
    }

    static double shower(double game_days, double rainfall, const WeatherSettings& s) noexcept {
        const double chance = vclamp((rainfall - 1.0) * s.showers, 0.0, MAX_SHOWER_CHANCE);
        if (chance <= 0.0 || s.shower_hours <= 0.0) return 0.0;
        const double t = game_days * HOURS_PER_DAY / s.shower_hours;
        const double cell = std::floor(t), f = t - cell;
        const double a = hash01(static_cast<std::int64_t>(cell)), b = hash01(static_cast<std::int64_t>(cell) + 1);
        const double k = f * f * (3.0 - 2.0 * f);
        const double v = a + (b - a) * k;
        if (v >= chance) return 0.0;
        return vclamp((chance - v) / chance * SHOWER_RAMP, 0.0, 1.0);
    }

    LocalWeather local(const BiomeClimate& c, double altitude, int sea_level, double time_of_day, double game_days, const CalendarDate& date,
                       const WeatherSettings& s, const SeasonSettings& seasons) const noexcept {
        LocalWeather out;
        const TemperatureSettings& t = s.temperature;
        double intensity = m_state.intensity;
        out.kind = m_state.kind;
        const double rain_shower = s.mode == WeatherMode::AlwaysClear ? 0.0 : shower(game_days, c.rainfall, s);

        if (rain_shower > intensity) {
            intensity = rain_shower;
            if (out.kind == WeatherKind::Clear) out.kind = WeatherKind::Rain;
        }

        out.cloud = vclamp(intensity * CLOUD_LEAD, 0.0, 1.0);
        const bool wet = out.kind != WeatherKind::Clear && c.rainfall >= s.dry_below;
        out.amount = wet ? intensity * vmin(c.rainfall, RAIN_CAP) : 0.0;
        if (!wet) out.cloud *= vclamp(c.rainfall / vmax(s.dry_below, MIN_DIVISOR), 0.0, 1.0);
        const double storm = m_state.kind == WeatherKind::Storm ? s.storm_darkening : s.darkening;
        out.darkness = out.cloud * storm;

        const double season = date.warmth * c.season_swing * HALF * seasons.temperature;
        const double daily = t.daily_change ? HALF * c.daily_swing * t.daily_strength * std::cos(FULL_TURN * (time_of_day - WARMEST_TIME)) * (1.0 - t.cloud_damping * out.cloud) : 0.0;
        const double height = vmax(0.0, altitude - sea_level) * t.altitude_drop / BLOCKS_PER_STEP;
        out.temperature = c.temperature + season + daily - height - t.weather_cooling * vmin(out.amount, 1.0) + t.offset;
        out.precipitation = out.amount > 0.0 ? kind_of(out.temperature, s) : Precipitation::None;
        out.flash = m_flash;
        return out;
    }

    Precipitation kind_of(double celsius, const WeatherSettings& s) const noexcept {
        if (s.style == RainStyle::Simple) return celsius <= s.snow_below ? Precipitation::Snow : Precipitation::Rain;
        if (m_state.kind == WeatherKind::Storm && m_state.hail && celsius >= s.hail_above) return Precipitation::Hail;
        if (celsius <= s.snow_below) return Precipitation::Snow;
        if (celsius <= FREEZING) return Precipitation::FreezingRain;
        if (celsius < s.sleet_below) return Precipitation::Sleet;
        return Precipitation::Rain;
    }

private:
    static constexpr int           KIND_COUNT        = 3;
    static constexpr double        HALF              = 0.5;
    static constexpr double        MIN_DIVISOR       = 1e-6;
    static constexpr double        RAIN_CAP          = 1.6;
    static constexpr double        MAX_SHOWER_CHANCE = 0.8;
    static constexpr double        SHOWER_RAMP       = 3.0;
    static constexpr std::uint64_t SHOWER_SALT       = 0x5A0E7B11ull;
    static constexpr double        UNIT_SCALE        = 1.0 / 9007199254740992.0;
    static constexpr int           UNIT_SHIFT        = 11;

    static double hash01(std::int64_t n) noexcept {
        std::uint64_t h = static_cast<std::uint64_t>(n) ^ SHOWER_SALT;
        h ^= h >> 33; h *= 0xFF51AFD7ED558CCDull; h ^= h >> 33; h *= 0xC4CEB9FE1A85EC53ull; h ^= h >> 33;
        return static_cast<double>(h >> UNIT_SHIFT) * UNIT_SCALE;
    }

    void apply_mode(const WeatherSettings& s, SeededRandom& rng) {
        switch (s.mode) {
            case WeatherMode::Changing: return;
            case WeatherMode::AlwaysClear: if (m_state.kind != WeatherKind::Clear) start(WeatherKind::Clear, s, rng); return;
            case WeatherMode::AlwaysRain:  if (m_state.kind != WeatherKind::Rain) start(WeatherKind::Rain, s, rng); m_state.target = ALWAYS_RAIN; return;
            case WeatherMode::AlwaysStorm: if (m_state.kind != WeatherKind::Storm) start(WeatherKind::Storm, s, rng); return;
        }
    }

    static WeatherKind pick_wet(const WeatherSettings& s, SeededRandom& rng) {
        return rng.unit() < s.storm_chance ? WeatherKind::Storm : WeatherKind::Rain;
    }

    static double lasting(double mean, SeededRandom& rng) {
        return -std::log(vmax(1.0 - rng.unit(), MIN_DIVISOR)) * mean;
    }

    void start(WeatherKind kind, const WeatherSettings& s, SeededRandom& rng, const CalendarDate& date = CalendarDate{}) {
        m_state.kind = kind;
        m_state.hail = kind == WeatherKind::Storm && rng.unit() < s.hail_chance;

        if (kind == WeatherKind::Clear) {
            const double rain = date.seasons ? s.season_rain[static_cast<std::size_t>(date.season)] : 1.0;
            m_state.target    = 0.0;
            m_state.remaining = lasting(s.clear_days / vmax(rain, MIN_DIVISOR), rng);
            return;
        }

        m_state.target    = kind == WeatherKind::Storm ? STORM_STRENGTH : rng.range(MIN_RAIN, 1.0);
        m_state.remaining = lasting(s.rain_days, rng);
    }

    WeatherState m_state;
    double       m_flash = 0.0;
};

} // namespace voxelspire

#endif // VOXELSPIRE_WEATHER_WEATHER_SYSTEM_HPP