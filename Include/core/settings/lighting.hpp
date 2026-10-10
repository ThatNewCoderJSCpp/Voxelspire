#ifndef VOXELSPIRE_CORE_SETTINGS_LIGHTING_HPP
#define VOXELSPIRE_CORE_SETTINGS_LIGHTING_HPP

#include <cstddef>
#include <string>
#include "../types.hpp"

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
    double       sun_shadow_redraw            = 0.125;
    bool         point_shadows                = false;
    unsigned int max_point_shadows            = 4;
    unsigned int moving_shadow_faces          = 12;
    unsigned int point_shadow_resolution      = 512;
    double       point_shadow_fade            = 4.0;
    bool         hide_unshadowed_point_lights = true;
 
    bool   capsule_shadows          = true;
    double capsule_shadow_sun_size  = 1.0;
    double capsule_shadow_lamp_size = 0.2;
 
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
    unsigned int volumetric_cell_size  = 8;
 
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
    double       mirror_resolution          = 0.5;
    double       reflection_distortion      = 0.02;
    bool         reflect_any_distance       = true;
    double       reflection_plane_distance  = 48.0;
    int          reflection_view_chunks     = 8;
    bool         water_planar_reflections   = true;
 
    bool   underwater_fog     = true;
    double underwater_density = 0.12;
 
    bool   tone_mapping = false;
    double exposure     = 1.0;
    double saturation   = 1.0;
 
    double update_budget_ms = 3.0;
 
    static LightingSettings off();
 
    static LightingSettings basic();
 
    static LightingSettings classic() { return {}; }
 
    static LightingSettings dynamic() {
        LightingSettings s;
        s.name           = "dynamic";
        s.dynamic_lights = true;
        return s;
    }
 
    static LightingSettings realistic();
 
    static LightingSettings ultra();
};

} // namespace voxelspire

#endif // VOXELSPIRE_CORE_SETTINGS_LIGHTING_HPP