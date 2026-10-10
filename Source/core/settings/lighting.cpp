#include "core/settings/lighting.hpp"

namespace voxelspire {

LightingSettings LightingSettings::off() {
    LightingSettings s;
    s.name              = "off";
    s.baked_light       = false;
    s.smooth_lighting   = false;
    s.ambient_occlusion = 0.0;
    return s;
}

LightingSettings LightingSettings::basic() {
    LightingSettings s;
    s.name              = "basic";
    s.smooth_lighting   = false;
    s.ambient_occlusion = 0.0;
    return s;
}

LightingSettings LightingSettings::realistic() {
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

LightingSettings LightingSettings::ultra() {
    LightingSettings s = realistic();
    s.name                    = "ultra";
    s.max_light               = 2.5;
    s.sun_strength            = 1.1;
    s.sky_light               = 0.55;
    s.ambient_occlusion       = 1.2;
    s.face_shading            = 0.2;
    s.volumetric_steps        = 48;
    s.volumetric_cell_size    = 6;
    s.volumetric_density      = 0.06;
    s.volumetric_intensity    = 2.0;
    s.light_shaft_samples     = 64;
    s.light_shaft_strength    = 0.3;
    s.screen_reflections      = true;
    s.max_reflection_planes   = 2;
    s.reflection_resolution   = 1.0;
    s.mirror_resolution       = 1.0;
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

} // namespace voxelspire
