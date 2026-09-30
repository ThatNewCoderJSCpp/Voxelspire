#ifndef VOXELSPIRE_LIGHTING_LIGHTING_SETTINGS_HPP
#define VOXELSPIRE_LIGHTING_LIGHTING_SETTINGS_HPP

#include <cstddef>
#include <string>
#include <vector>
#include "format.hpp"

namespace voxelspire {

struct LightingSettings {
    std::string name = "classic";

    bool   baked_light       = true;
    bool   smooth_lighting   = true;
    double ambient_occlusion = 1.0;
    double occlusion_step    = 0.2;
    double face_shading      = 1.0;

    double falloff   = 1.6;
    double min_light = 0.05;
    double max_light = 1.0;
    double ambient   = 0.0;

    double sky_light   = 1.0;
    double block_light = 1.0;
    Color  block_tint  = Color(255, 238, 214);

    bool   sun_lighting = false;
    double sun_strength = 0.55;
    double sun_exposure = 4.0;

    bool        dynamic_lights         = false;
    std::size_t max_dynamic_lights     = 32;
    double      dynamic_light_distance = 64.0;

    bool        block_point_lights          = false;
    std::size_t max_block_point_lights      = 24;
    double      block_point_light_distance  = 40.0;
    double      block_point_light_intensity = 0.8;
    double      block_point_light_radius    = 12.0;
    double      point_light_fade            = 6.0;

    bool         sun_shadows                  = false;
    unsigned int sun_shadow_resolution        = 2048;
    double       sun_shadow_distance          = 64.0;
    double       shadow_strength              = 0.85;
    double       shadow_softness              = 1.5;
    double       shadow_angle_step            = 0.5;
    bool         shadow_crossfade             = true;
    bool         soft_shadows                 = false;
    double       soft_shadow_sun_size         = 1.2;
    double       max_shadow_softness          = 12.0;
    unsigned int shadow_filter_taps           = 4;
    bool         point_shadows                = false;
    unsigned int max_point_shadows            = 4;
    unsigned int point_shadow_resolution      = 512;
    double       point_shadow_fade            = 4.0;
    bool         hide_unshadowed_point_lights = true;

    bool   atmosphere      = false;
    double fog_density     = 0.0025;
    double fog_start       = 32.0;
    double sky_glow        = 0.35;
    double sun_glow_spread = 8.0;
    double sun_glow_focus  = 400.0;

    bool         volumetric_light      = false;
    unsigned int volumetric_steps      = 16;
    double       volumetric_density    = 0.01;
    double       volumetric_anisotropy = 0.5;
    double       volumetric_distance   = 96.0;
    double       volumetric_intensity  = 1.0;
    double       volumetric_near_bias  = 2.0;

    bool         light_shafts          = false;
    unsigned int light_shaft_samples   = 48;
    double       light_shaft_strength  = 0.25;
    double       light_shaft_decay     = 0.96;
    double       light_shaft_length    = 0.85;
    double       light_shaft_focus     = 24.0;

    bool         reflections         = false;
    double       reflectivity        = 1.0;
    double       wave_strength       = 0.08;
    double       wave_scale          = 0.9;
    double       wave_speed          = 1.2;
    double       specular_power      = 180.0;
    double       specular            = 2.5;
    bool         refraction          = false;
    double       refraction_strength = 1.0;
    double       water_absorption    = 0.1;
    double       water_scattering    = 0.08;
    bool         screen_reflections  = false;
    unsigned int reflection_steps    = 32;
    double       reflection_distance = 64.0;

    bool         planar_reflections         = false;
    unsigned int max_reflection_planes      = 1;
    double       reflection_resolution      = 0.5;
    double       reflection_distortion      = 0.02;
    double       reflection_plane_distance  = 48.0;
    double       reflection_render_distance = 128.0;
    bool         water_planar_reflections   = true;

    bool   underwater_fog     = true;
    double underwater_density = 0.12;

    bool   tone_mapping = false;
    double exposure     = 1.0;
    double saturation   = 1.0;

    double update_budget_ms = 3.0;

    static LightingSettings off() {
        LightingSettings s;
        s.name              = "off";
        s.baked_light       = false;
        s.smooth_lighting   = false;
        s.ambient_occlusion = 0.0;
        return s;
    }

    static LightingSettings basic() {
        LightingSettings s;
        s.name              = "basic";
        s.smooth_lighting   = false;
        s.ambient_occlusion = 0.0;
        return s;
    }

    static LightingSettings classic() { return {}; }

    static LightingSettings dynamic() {
        LightingSettings s;
        s.name           = "dynamic";
        s.dynamic_lights = true;
        return s;
    }

    static LightingSettings realistic() {
        LightingSettings s;
        s.name                 = "realistic";
        s.dynamic_lights       = true;
        s.block_point_lights   = true;
        s.max_light            = 2.2;
        s.sun_strength         = 1.0;
        s.sky_light            = 0.58;
        s.block_light          = 0.5;
        s.ambient_occlusion    = 1.1;
        s.face_shading         = 0.25;
        s.sun_lighting         = true;
        s.sun_shadows          = true;
        s.soft_shadows         = true;
        s.point_shadows        = true;
        s.atmosphere           = true;
        s.volumetric_light     = true;
        s.volumetric_steps     = 24;
        s.volumetric_density   = 0.04;
        s.volumetric_intensity = 1.0;
        s.light_shafts         = true;
        s.reflections          = true;
        s.planar_reflections   = true;
        s.refraction           = true;
        s.tone_mapping         = true;
        s.exposure             = 0.9;
        s.saturation           = 1.15;
        return s;
    }

    static LightingSettings ultra() {
        LightingSettings s = realistic();
        s.name                    = "ultra";
        s.max_light               = 2.5;
        s.sun_strength            = 1.1;
        s.sky_light               = 0.55;
        s.ambient_occlusion       = 1.2;
        s.face_shading            = 0.2;
        s.volumetric_steps        = 48;
        s.volumetric_density      = 0.06;
        s.volumetric_intensity    = 2.0;
        s.light_shaft_samples     = 64;
        s.light_shaft_strength    = 0.3;
        s.screen_reflections      = true;
        s.max_reflection_planes   = 2;
        s.reflection_resolution   = 1.0;
        s.saturation              = 1.2;
        s.sun_shadow_resolution   = 4096;
        s.sun_shadow_distance     = 96.0;
        s.shadow_strength         = 0.92;
        s.shadow_filter_taps      = 6;
        s.soft_shadow_sun_size    = 1.5;
        s.max_point_shadows       = 8;
        s.point_shadow_resolution = 768;
        s.max_block_point_lights  = 32;
        return s;
    }

    static std::vector<LightingSettings> presets() { return { off(), basic(), classic(), dynamic(), realistic(), ultra() }; }
};

struct DayCycleSettings {
    bool   enabled          = true;
    double ticks_per_day    = 72000.0;
    double hours_per_day    = 24.0;
    double start_time       = 0.3;
    double sun_tilt         = 0.35;
    double night_brightness = 0.2;
    double horizon_fade     = 0.12;
    double twilight         = 0.2;
    double dusk_sky_mix     = 0.6;
    Color  day_sky          = Color(135, 190, 255);
    Color  night_sky        = Color(8, 11, 28);
    Color  dusk_sky         = Color(250, 140, 80);
    Color  day_light_tint   = Color(255, 255, 255);
    Color  night_light_tint = Color(130, 150, 255);
    Color  day_zenith       = Color(62, 118, 228);
    Color  day_horizon      = Color(172, 208, 250);
    Color  night_zenith     = Color(3, 5, 16);
    Color  night_horizon    = Color(14, 20, 44);
    Color  dusk_horizon     = Color(255, 150, 90);
    Color  sun_glow         = Color(255, 214, 160);
    Color  dusk_glow        = Color(255, 120, 60);
};

} // namespace voxelspire

#endif // VOXELSPIRE_LIGHTING_LIGHTING_SETTINGS_HPP