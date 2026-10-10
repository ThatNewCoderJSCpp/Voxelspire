#include "weather/system.hpp"

namespace voxelspire {

const char* weather_name(WeatherKind k) noexcept {
    switch (k) {
        case WeatherKind::Clear: return "Clear";
        case WeatherKind::Rain:  return "Rain";
        case WeatherKind::Storm: return "Storm";
    }

    return "Clear";
}

const char* precipitation_name(Precipitation p) noexcept {
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

BiomeClimate BiomeWeather::effective(const Biome& biome, const BiomeOptions& o) noexcept {
    BiomeClimate c = biome.climate();
    if (o.mean_temperature) c.temperature  = *o.mean_temperature;
    if (o.daily_swing)      c.daily_swing  = *o.daily_swing;
    if (o.season_swing)     c.season_swing = *o.season_swing;
    if (o.rainfall)         c.rainfall     = *o.rainfall;
    if (o.waves)            c.waves        = *o.waves;
    return c;
}

BiomeClimate BiomeWeather::blend(const BiomeClimate* climates, const double* weights, std::size_t count) noexcept {
    BiomeClimate out{ 0.0, 0.0, 0.0, 0.0, 0.0, Color(0, 0, 0) };
    double red = 0.0, green = 0.0, blue = 0.0, total = 0.0;

    for (std::size_t i = 0; i < count; ++i) {
        const double w = weights ? weights[i] : 1.0;
        const BiomeClimate& c = climates[i];
        out.temperature  += c.temperature * w;
        out.daily_swing  += c.daily_swing * w;
        out.season_swing += c.season_swing * w;
        out.rainfall     += c.rainfall * w;
        out.waves        += c.waves * w;
        red   += c.water.red() * w;
        green += c.water.green() * w;
        blue  += c.water.blue() * w;
        total += w;
    }

    if (total <= 0.0) return BiomeClimate{};
    auto channel = [total](double v) { return static_cast<std::uint8_t>(std::lround(v / total)); };
    return { out.temperature / total, out.daily_swing / total, out.season_swing / total, out.rainfall / total, out.waves / total, Color(channel(red), channel(green), channel(blue)) };
}

void WeatherSystem::cycle(const WeatherSettings& s, SeededRandom& rng) {
    const WeatherKind next = static_cast<WeatherKind>((static_cast<int>(m_state.kind) + 1) % KIND_COUNT);
    force(next, s, rng);
}

void WeatherSystem::update(double game_days, double real_seconds, const WeatherSettings& s, const CalendarDate& date, SeededRandom& rng) {
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

double WeatherSystem::shower(double game_days, double rainfall, const WeatherSettings& s, double x, double y) noexcept {
    const double chance = vclamp((rainfall - 1.0) * s.showers, 0.0, MAX_SHOWER_CHANCE);
    if (chance <= 0.0 || s.shower_hours <= 0.0) return 0.0;
    const double size = vmax(s.shower_size, 1.0);
    const double v = value_noise(x / size, y / size, game_days * HOURS_PER_DAY / s.shower_hours);
    if (v >= chance) return 0.0;
    return vclamp((chance - v) * SHOWER_SHARPNESS, 0.0, 1.0);
}

LocalWeather WeatherSystem::local(const BiomeClimate& c, double altitude, int sea_level, double time_of_day, double game_days, const CalendarDate& date,
                   const WeatherSettings& s, const SeasonSettings& seasons, double x, double y) const noexcept {
    LocalWeather out;
    const TemperatureSettings& t = s.temperature;
    double intensity = m_state.intensity;
    out.kind = m_state.kind;
    const double rain_shower = s.mode == WeatherMode::AlwaysClear ? 0.0 : shower(game_days, c.rainfall, s, x, y);

    if (rain_shower > intensity) {
        intensity = rain_shower;
        if (out.kind == WeatherKind::Clear) out.kind = WeatherKind::Rain;
    }

    out.cloud = vclamp(intensity * CLOUD_LEAD, 0.0, 1.0);
    const double dry_start = s.dry_below * DRY_FADE;
    const double wetness = vclamp((c.rainfall - dry_start) / vmax(s.dry_below - dry_start, MIN_DIVISOR), 0.0, 1.0);
    const bool wet = out.kind != WeatherKind::Clear && wetness > 0.0;
    out.amount = wet ? intensity * vmin(c.rainfall, RAIN_CAP) * wetness : 0.0;
    if (wetness < 1.0) out.cloud *= vclamp(c.rainfall / vmax(s.dry_below, MIN_DIVISOR), 0.0, 1.0);
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

Precipitation WeatherSystem::kind_of(double celsius, const WeatherSettings& s) const noexcept {
    if (s.style == RainStyle::Simple) return celsius <= s.snow_below ? Precipitation::Snow : Precipitation::Rain;
    if (m_state.kind == WeatherKind::Storm && m_state.hail && celsius >= s.hail_above) return Precipitation::Hail;
    if (celsius <= s.snow_below) return Precipitation::Snow;
    if (celsius <= FREEZING) return Precipitation::FreezingRain;
    if (celsius < s.sleet_below) return Precipitation::Sleet;
    return Precipitation::Rain;
}

double WeatherSystem::lattice(std::int64_t x, std::int64_t y, std::int64_t t) noexcept {
    const std::uint64_t h = static_cast<std::uint64_t>(x) * LATTICE_X ^ static_cast<std::uint64_t>(y) * LATTICE_Y ^ static_cast<std::uint64_t>(t) * LATTICE_T;
    return hash01(static_cast<std::int64_t>(h));
}

double WeatherSystem::value_noise(double x, double y, double t) noexcept {
    const double fx = std::floor(x), fy = std::floor(y), ft = std::floor(t);
    const auto ix = static_cast<std::int64_t>(fx), iy = static_cast<std::int64_t>(fy), it = static_cast<std::int64_t>(ft);
    auto smooth = [](double v) { return v * v * (3.0 - 2.0 * v); };
    const double kx = smooth(x - fx), ky = smooth(y - fy), kt = smooth(t - ft);
    auto lerp = [](double a, double b, double k) { return a + (b - a) * k; };
    auto plane = [&](std::int64_t c) {
        return lerp(lerp(lattice(ix, iy, c), lattice(ix + 1, iy, c), kx), lerp(lattice(ix, iy + 1, c), lattice(ix + 1, iy + 1, c), kx), ky);
    };
    return lerp(plane(it), plane(it + 1), kt);
}

double WeatherSystem::hash01(std::int64_t n) noexcept {
    std::uint64_t h = static_cast<std::uint64_t>(n) ^ SHOWER_SALT;
    h ^= h >> 33; h *= 0xFF51AFD7ED558CCDull; h ^= h >> 33; h *= 0xC4CEB9FE1A85EC53ull; h ^= h >> 33;
    return static_cast<double>(h >> UNIT_SHIFT) * UNIT_SCALE;
}

void WeatherSystem::apply_mode(const WeatherSettings& s, SeededRandom& rng) {
    switch (s.mode) {
        case WeatherMode::Changing: return;
        case WeatherMode::AlwaysClear: if (m_state.kind != WeatherKind::Clear) start(WeatherKind::Clear, s, rng); return;
        case WeatherMode::AlwaysRain:  if (m_state.kind != WeatherKind::Rain) start(WeatherKind::Rain, s, rng); m_state.target = ALWAYS_RAIN; return;
        case WeatherMode::AlwaysStorm: if (m_state.kind != WeatherKind::Storm) start(WeatherKind::Storm, s, rng); return;
    }
}

void WeatherSystem::start(WeatherKind kind, const WeatherSettings& s, SeededRandom& rng, const CalendarDate& date) {
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

void PrecipitationEasing::apply(LocalWeather& w, double real_seconds, const WeatherSystem& system, const WeatherSettings& s) noexcept {
    const double step = s.rain_fade > 0.0 ? vmax(real_seconds, 0.0) / s.rain_fade : 1.0;
    m_amount   = approach(m_amount, w.amount, step);
    m_cloud    = approach(m_cloud, w.cloud, step);
    m_darkness = approach(m_darkness, w.darkness, step);
    w.amount   = m_amount;
    w.cloud    = m_cloud;
    w.darkness = m_darkness;

    if (m_amount <= 0.0) {
        w.precipitation = Precipitation::None;
        return;
    }

    if (w.precipitation == Precipitation::None) w.precipitation = system.kind_of(w.temperature, s);
    if (w.kind == WeatherKind::Clear) w.kind = WeatherKind::Rain;
}

} // namespace voxelspire
