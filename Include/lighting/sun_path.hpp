#ifndef VOXELSPIRE_LIGHTING_SUN_PATH_HPP
#define VOXELSPIRE_LIGHTING_SUN_PATH_HPP

#include <cmath>
#include "../core/settings.hpp"

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
    double   dusk            = 0.0;
    bool     moon            = false;
    double   clear           = 1.0;
    double   fog_boost       = 0.0;
    double   waves           = 1.0;
};

Color mix_color(const Color& a, const Color& b, double t) noexcept;

inline double smooth_step(double edge0, double edge1, double x) noexcept {
    const double t = vclamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
    return t * t * (3.0 - 2.0 * t);
}

class SunPath {
public:
    virtual ~SunPath() = default;
    virtual SkyState evaluate(double time_of_day) const = 0;
    virtual void set_declination(double) noexcept {}
};

class DefaultSunPath final : public SunPath {
public:
    static constexpr double NOON      = 0.5;
    static constexpr double FULL_TURN = 2.0 * PI;

    DefaultSunPath(const DayCycleSettings& settings, const CelestialSettings& bodies) : m_settings(settings), m_bodies(bodies) {}

    SkyState evaluate(double time_of_day) const override;

    void set_declination(double radians) noexcept override { m_declination = radians; }

    double declination() const noexcept { return m_declination; }

    const DayCycleSettings&  settings() const noexcept { return m_settings; }
    const CelestialSettings& bodies()   const noexcept { return m_bodies; }

private:
    DayCycleSettings  m_settings;
    CelestialSettings m_bodies;
    double            m_declination = 0.0;
};

class WorldClock {
public:
    explicit WorldClock(double start_time = 0.3) noexcept { set_time(start_time); }

    void advance(double seconds, const DayCycleSettings& s) noexcept {
        if (!s.enabled || s.real_day_seconds() <= 0.0) return;
        add(seconds / s.real_day_seconds());
    }

    void add(double days) noexcept;

    void set_time(double time_of_day) noexcept { m_time = time_of_day - std::floor(time_of_day); }
    void set(std::int64_t day, double time_of_day) noexcept { m_day = day; set_time(time_of_day); }

    double       time() const noexcept { return m_time; }
    std::int64_t day()  const noexcept { return m_day; }
    double       hours() const noexcept { return m_time * DayCycleSettings::HOURS_PER_DAY; }

private:
    double       m_time = 0.0;
    std::int64_t m_day  = 0;
};

} // namespace voxelspire

#endif // VOXELSPIRE_LIGHTING_SUN_PATH_HPP