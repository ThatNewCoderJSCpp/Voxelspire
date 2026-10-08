#ifndef VOXELSPIRE_CORE_LIMITS_LIGHTING_HPP
#define VOXELSPIRE_CORE_LIMITS_LIGHTING_HPP

#include "bounds.hpp"

namespace voxelspire {

struct LightingLimits {
    static constexpr Bounds ambient_occlusion           { 0.0, 2.0 };
    static constexpr Bounds occlusion_step              { 0.0, 0.5 };
    static constexpr Bounds face_shading                { 0.0, 1.0 };
    static constexpr Bounds falloff                     { 1.0, 4.0 };
    static constexpr Bounds min_light                   { 0.0, 0.5 };
    static constexpr Bounds max_light                   { 0.5, 4.0 };
    static constexpr Bounds ambient                     { 0.0, 1.0 };
    static constexpr Bounds sky_light                   { 0.0, 2.0 };
    static constexpr Bounds block_light                 { 0.0, 2.0 };
    static constexpr Bounds sun_strength                { 0.0, 3.0 };
    static constexpr Bounds sun_exposure                { 0.0, 8.0 };
    static constexpr Bounds max_dynamic_lights          { 0.0, 128.0 };
    static constexpr Bounds dynamic_light_distance      { 8.0, 256.0 };
    static constexpr Bounds max_block_point_lights      { 0.0, 64.0 };
    static constexpr Bounds block_point_light_distance  { 8.0, 128.0 };
    static constexpr Bounds block_point_light_intensity { 0.0, 3.0 };
    static constexpr Bounds block_point_light_radius    { 1.0, 32.0 };
    static constexpr Bounds point_light_fade            { 0.0, 16.0 };
    static constexpr Bounds sun_shadow_distance         { 16.0, 256.0 };
    static constexpr Bounds shadow_strength             { 0.0, 1.0 };
    static constexpr Bounds shadow_softness             { 0.0, 6.0 };
    static constexpr Bounds shadow_angle_step           { 0.0, 2.0 };
    static constexpr Bounds soft_shadow_sun_size        { 0.1, 5.0 };
    static constexpr Bounds max_shadow_softness         { 1.0, 32.0 };
    static constexpr Bounds shadow_filter_taps          { 1.0, 8.0 };
    static constexpr Bounds sun_shadow_redraw           { 0.0, 0.5 };
    static constexpr Bounds max_point_shadows           { 0.0, 8.0 };
    static constexpr Bounds moving_shadow_faces         { 0.0, 48.0 };
    static constexpr Bounds point_shadow_fade           { 0.0, 16.0 };
    static constexpr Bounds capsule_shadow_sun_size     { 0.0, 5.0 };
    static constexpr Bounds capsule_shadow_lamp_size    { 0.0, 1.0 };
    static constexpr Bounds fog_density                 { 0.0, 0.05 };
    static constexpr Bounds fog_start                   { 0.0, 256.0 };
    static constexpr Bounds sky_glow                    { 0.0, 2.0 };
    static constexpr Bounds sun_glow_spread             { 1.0, 32.0 };
    static constexpr Bounds sun_glow_focus              { 16.0, 4000.0 };
    static constexpr Bounds volumetric_steps            { 4.0, 128.0 };
    static constexpr Bounds volumetric_density          { 0.0, 0.2 };
    static constexpr Bounds volumetric_anisotropy       { -0.9, 0.95 };
    static constexpr Bounds volumetric_distance         { 16.0, 256.0 };
    static constexpr Bounds volumetric_intensity        { 0.0, 5.0 };
    static constexpr Bounds volumetric_near_bias        { 1.0, 4.0 };
    static constexpr Bounds volumetric_cell_size        { 2.0, 32.0 };
    static constexpr Bounds light_shaft_samples         { 8.0, 128.0 };
    static constexpr Bounds light_shaft_strength        { 0.0, 2.0 };
    static constexpr Bounds light_shaft_decay           { 0.8, 1.0 };
    static constexpr Bounds light_shaft_length          { 0.1, 1.0 };
    static constexpr Bounds light_shaft_focus           { 1.0, 100.0 };
    static constexpr Bounds reflectivity                { 0.0, 2.0 };
    static constexpr Bounds wave_strength               { 0.0, 0.5 };
    static constexpr Bounds wave_scale                  { 0.1, 4.0 };
    static constexpr Bounds wave_speed                  { 0.0, 5.0 };
    static constexpr Bounds specular_power              { 8.0, 1000.0 };
    static constexpr Bounds specular                    { 0.0, 10.0 };
    static constexpr Bounds refraction_strength         { 0.0, 2.0 };
    static constexpr Bounds water_absorption            { 0.0, 1.0 };
    static constexpr Bounds water_scattering            { 0.0, 1.0 };
    static constexpr Bounds reflection_steps            { 4.0, 128.0 };
    static constexpr Bounds reflection_distance         { 8.0, 256.0 };
    static constexpr Bounds max_reflection_planes       { 0.0, 2.0 };
    static constexpr Bounds reflection_resolution       { 0.25, 1.0 };
    static constexpr Bounds mirror_resolution           { 0.25, 1.0 };
    static constexpr Bounds reflection_distortion       { 0.0, 0.1 };
    static constexpr Bounds reflection_plane_distance   { 8.0, 1024.0 };
    static constexpr Bounds reflection_view_chunks      { 1.0, 32.0 };
    static constexpr Bounds underwater_density          { 0.0, 1.0 };
    static constexpr Bounds exposure                    { 0.1, 3.0 };
    static constexpr Bounds saturation                  { 0.0, 2.0 };
    static constexpr Bounds update_budget_ms            { 0.1, 20.0 };
};

struct DynamicLightLimits {
    static constexpr Bounds intensity { 0.0, 4.0 };
    static constexpr Bounds radius    { 1.0, 40.0 };
    static constexpr Bounds level     { 0.0, 15.0 };
    static constexpr Bounds heat      { 0.0, 200.0 };
    static constexpr Bounds cone      { 0.0, 180.0 };
    static constexpr Bounds softness  { 0.0, 1.0 };
};

} // namespace voxelspire

#endif // VOXELSPIRE_CORE_LIMITS_LIGHTING_HPP