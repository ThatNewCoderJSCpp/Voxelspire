#ifndef VOXELSPIRE_LIGHTING_SUN_PATH_HPP
#define VOXELSPIRE_LIGHTING_SUN_PATH_HPP

#include <cmath>
#include "settings.hpp"

namespace voxelspire {

struct SkyState {
    double   time            = 0.5;
    vector3d light_direction { 0.0, 0.0, -1.0 };
    Color    light_color     = Color(255, 255, 255);
    double   light_strength  = 1.0;
    double   daylight        = 1.0;
    Color    sky_color       = Color(135, 190, 255);
    Color    sky_light_color = Color(255, 255, 255);
    Color    zenith          = Color(62, 118, 228);
    Color    horizon         = Color(172, 208, 250);
    Color    glow            = Color(255, 214, 160);
    vector3d sun_position    { 0.0, 0.0, 1.0 };
    vector3d moon_position   { 0.0, 0.0, -1.0 };
    double   night           = 0.0;
    bool     moon            = false;
};

inline Color mix_color(const Color& a, const Color& b, double t) noexcept {
    const double k = vclamp(t, 0.0, 1.0);
    auto ch = [k](std::uint8_t x, std::uint8_t y) { return static_cast<std::uint8_t>(std::lround(x + (y - x) * k)); };
    return Color(ch(a.red(), b.red()), ch(a.green(), b.green()), ch(a.blue(), b.blue()), ch(a.alpha(), b.alpha()));
}

inline double smooth_step(double edge0, double edge1, double x) noexcept {
    const double t = vclamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
    return t * t * (3.0 - 2.0 * t);
}

class SunPath {
public:
    virtual ~SunPath() = default;
    virtual SkyState evaluate(double time_of_day) const = 0;
};

class DefaultSunPath final : public SunPath {
public:
    static constexpr double SUNRISE = 0.25;
    static constexpr double FULL_TURN = 2.0 * PI;

    explicit DefaultSunPath(const DayCycleSettings& settings) : m_settings(settings) {}

    SkyState evaluate(double time_of_day) const override {
        const DayCycleSettings& s = m_settings;
        SkyState out;
        out.time = time_of_day;
        const double angle = FULL_TURN * (time_of_day - SUNRISE);
        vector3d sun{ std::cos(angle), -s.sun_tilt * std::sin(angle), std::sin(angle) };
        sun = sun / sun.magnitude();
        const double height = sun.z;
        const double sun_up = smooth_step(-s.horizon_fade, s.horizon_fade, height);
        const double moon_up = smooth_step(-s.horizon_fade, s.horizon_fade, -height);
        const double dusk = 1.0 - smooth_step(0.0, s.twilight, std::fabs(height));
        out.daylight = s.night_brightness + (1.0 - s.night_brightness) * smooth_step(-s.twilight, s.twilight, height);
        out.sky_light_color = mix_color(s.night_light_tint, s.day_light_tint, smooth_step(-s.twilight, s.twilight, height));
        const double day = smooth_step(-s.twilight, s.twilight, height);
        out.sky_color = mix_color(mix_color(s.night_sky, s.day_sky, day), s.dusk_sky, dusk * s.dusk_sky_mix);
        out.zenith = mix_color(s.night_zenith, s.day_zenith, day);
        out.horizon = mix_color(mix_color(s.night_horizon, s.day_horizon, day), s.dusk_horizon, dusk * s.dusk_sky_mix);
        out.glow = mix_color(s.sun_glow, s.dusk_glow, dusk);
        out.sun_position = sun;
        out.moon_position = sun * -1.0;
        out.night = 1.0 - day;

        if (sun_up >= moon_up * s.moon_strength) {
            out.light_direction = sun * -1.0;
            out.light_color     = mix_color(s.sun_color, s.dusk_sun_color, dusk);
            out.light_strength  = sun_up;
            out.moon            = false;
        } else {
            out.light_direction = sun;
            out.light_color     = s.moon_color;
            out.light_strength  = moon_up * s.moon_strength;
            out.moon            = true;
        }

        return out;
    }

    const DayCycleSettings& settings() const noexcept { return m_settings; }

private:
    DayCycleSettings m_settings;
};

class WorldClock {
public:
    static constexpr double HOURS_PER_DAY = 24.0;

    explicit WorldClock(double start_time = 0.3) noexcept { set_time(start_time); }

    void advance(double seconds, const DayCycleSettings& s) noexcept {
        if (!s.enabled || s.day_length <= 0.0) return;
        add(seconds / s.day_length);
    }

    void add(double days) noexcept {
        double t = m_time + days;
        const double whole = std::floor(t);
        m_day = static_cast<std::int64_t>(m_day + static_cast<std::int64_t>(whole));
        m_time = t - whole;
    }

    void set_time(double time_of_day) noexcept { m_time = time_of_day - std::floor(time_of_day); }

    double       time() const noexcept { return m_time; }
    std::int64_t day()  const noexcept { return m_day; }
    double       hours() const noexcept { return m_time * HOURS_PER_DAY; }

private:
    double       m_time = 0.0;
    std::int64_t m_day  = 0;
};

} // namespace voxelspire

#endif // VOXELSPIRE_LIGHTING_SUN_PATH_HPP