#include "lighting/sun_path.hpp"

namespace voxelspire {

Color mix_color(const Color& a, const Color& b, double t) noexcept {
    const double k = vclamp(t, 0.0, 1.0);
    auto ch = [k](std::uint8_t x, std::uint8_t y) { return static_cast<std::uint8_t>(std::lround(x + (y - x) * k)); };
    return Color(ch(a.red(), b.red()), ch(a.green(), b.green()), ch(a.blue(), b.blue()), ch(a.alpha(), b.alpha()));
}

SkyState DefaultSunPath::evaluate(double time_of_day) const {
    const DayCycleSettings& s = m_settings;
    SkyState out;
    out.time = time_of_day;
    const double hour_angle = FULL_TURN * (time_of_day - NOON);
    const double latitude = std::atan(s.sun_tilt);
    const double d = m_declination;
    vector3d sun{
        -std::cos(d) * std::sin(hour_angle),
        std::cos(latitude) * std::sin(d) - std::sin(latitude) * std::cos(d) * std::cos(hour_angle),
        std::sin(latitude) * std::sin(d) + std::cos(latitude) * std::cos(d) * std::cos(hour_angle)
    };
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
    out.dusk = dusk;
    const SunSettings& b = m_bodies.sun;
    const MoonSettings& m = m_bodies.moon;
    const double sun_light = b.emits_light ? sun_up * b.light_strength : 0.0;
    const double moon_light = m.emits_light ? moon_up * m.light_strength : 0.0;

    if (sun_light >= moon_light) {
        out.light_direction = sun * -1.0;
        out.light_color     = mix_color(b.light_color, b.dusk_light_color, dusk);
        out.light_strength  = sun_light;
        out.moon            = false;
    } else {
        out.light_direction = sun;
        out.light_color     = m.light_color;
        out.light_strength  = moon_light;
        out.moon            = true;
    }

    return out;
}

void WorldClock::add(double days) noexcept {
    double t = m_time + days;
    const double whole = std::floor(t);
    m_day = static_cast<std::int64_t>(m_day + static_cast<std::int64_t>(whole));
    m_time = t - whole;
}

} // namespace voxelspire
