#ifndef VOXELSPIRE_WEATHER_SKY_WEATHER_HPP
#define VOXELSPIRE_WEATHER_SKY_WEATHER_HPP

#include "../lighting/sun_path.hpp"
#include "system.hpp"

namespace voxelspire {

struct OvercastLook {
    Color  sky         = Color(150, 156, 166);
    Color  horizon     = Color(176, 180, 188);
    Color  light       = Color(205, 210, 220);
    Color  flash       = Color(235, 240, 255);
    double sky_mix     = 0.85;
    double light_mix   = 0.6;
    double night_floor = 0.15;
    double daylight    = 0.5;
    double flash_light = 0.9;
    double flash_sky   = 0.6;
    double snow_fog    = 1.6;
    double storm_waves = 2.0;
};

class SkyWeather {
public:
    static void apply(SkyState& sky, const LocalWeather& w, const WeatherSettings& s, const WeatherViewSettings& view, const OvercastLook& look = OvercastLook{}) noexcept;

private:
    static Color dim(const Color& c, double k) noexcept { return mix_color(Color(0, 0, 0), c, k); }
};

} // namespace voxelspire

#endif // VOXELSPIRE_WEATHER_SKY_WEATHER_HPP