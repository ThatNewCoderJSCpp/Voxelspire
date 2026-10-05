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
    static void apply(SkyState& sky, const LocalWeather& w, const WeatherSettings& s, const WeatherViewSettings& view, const OvercastLook& look = OvercastLook{}) noexcept {
        const double cover = w.cloud;
        const double day = vmax(1.0 - sky.night, look.night_floor);
        const Color grey    = dim(look.sky, day);
        const Color horizon = dim(look.horizon, day);
        sky.sky_color       = mix_color(sky.sky_color, grey, cover * look.sky_mix);
        sky.zenith          = mix_color(sky.zenith, grey, cover * look.sky_mix);
        sky.horizon         = mix_color(sky.horizon, horizon, cover * look.sky_mix);
        sky.glow            = mix_color(sky.glow, horizon, cover * look.sky_mix);
        sky.sky_light_color = mix_color(sky.sky_light_color, look.light, cover * look.light_mix);
        sky.light_strength *= 1.0 - w.darkness;
        sky.daylight       *= 1.0 - w.darkness * look.daylight;
        sky.clear           = 1.0 - cover;
        const double snowy  = w.precipitation == Precipitation::Snow ? look.snow_fog : 1.0;
        sky.fog_boost       = s.fog * w.amount * snowy;
        sky.waves           = 1.0 + s.wind * cover * (w.kind == WeatherKind::Storm ? look.storm_waves : 1.0);

        if (view.flashes && w.flash > 0.0) {
            sky.daylight        = vmin(1.0, sky.daylight + w.flash * look.flash_light);
            sky.sky_color       = mix_color(sky.sky_color, look.flash, w.flash * look.flash_sky);
            sky.zenith          = mix_color(sky.zenith, look.flash, w.flash * look.flash_sky);
            sky.horizon         = mix_color(sky.horizon, look.flash, w.flash * look.flash_sky);
            sky.sky_light_color = mix_color(sky.sky_light_color, look.flash, w.flash);
        }
    }

private:
    static Color dim(const Color& c, double k) noexcept { return mix_color(Color(0, 0, 0), c, k); }
};

} // namespace voxelspire

#endif // VOXELSPIRE_WEATHER_SKY_WEATHER_HPP