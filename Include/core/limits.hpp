#ifndef VOXELSPIRE_CORE_LIMITS_HPP
#define VOXELSPIRE_CORE_LIMITS_HPP

#include "types.hpp"

namespace voxelspire {

struct Bounds {
    double min = 0.0;
    double max = 1.0;

    constexpr double clamp(double v) const noexcept { return vclamp(v, min, max); }
    constexpr bool contains(double v) const noexcept { return v >= min && v <= max; }
    constexpr bool valid() const noexcept { return max >= min; }
};

struct EngineLimits {
    static constexpr int CHUNK_SIZE           = 16;
    static constexpr int WORLD_MIN_Z          = -512;
    static constexpr int WORLD_MAX_Z          = 1024;
    static constexpr int WORLD_MAX_HORIZONTAL = 950'000'000;
};

struct WorldLimits {
    static constexpr Bounds gravity { 0.0, 100.0 };
};

struct CameraLimits {
    static constexpr Bounds fov_y                 { 30.0, 130.0 };
    static constexpr Bounds near_plane            { 0.01, 1.0 };
    static constexpr Bounds third_person_distance { 1.0, 16.0 };
    static constexpr Bounds collision_margin      { 0.0, 1.0 };
};

struct DisplayLimits {
    static constexpr Bounds max_fps { 0.0, 1000.0 };
};

struct ControlLimits {
    static constexpr Bounds mouse_sensitivity { 0.01, 1.0 };
};

struct SimulationLimits {
    static constexpr Bounds tick_rate           { 1.0, 480.0 };
    static constexpr Bounds game_speed          { 0.1, 10.0 };
    static constexpr Bounds max_ticks_per_frame { 1.0, 60.0 };
};

struct SaveLimits {
    static constexpr Bounds autosave_minutes { 0.0, 60.0 };
};

struct MenuLimits {
    static constexpr Bounds scale { 0.6, 2.0 };
};

struct StreamingLimits {
    static constexpr Bounds simulation_distance   { 1.0, 32.0 };
    static constexpr Bounds detail_distance       { 1.0, 64.0 };
    static constexpr Bounds unload_margin         { 0.0, 8.0 };
    static constexpr Bounds max_column_jobs       { 1.0, 512.0 };
    static constexpr Bounds max_mesh_jobs         { 1.0, 1024.0 };
    static constexpr Bounds upload_megabytes      { 1.0, 256.0 };
    static constexpr Bounds result_time_budget_ms { 0.5, 33.0 };
};

struct LodLimits {
    static constexpr Bounds tile_chunks        { 1.0, 8.0 };
    static constexpr Bounds max_level          { 1.0, 16.0 };
    static constexpr Bounds heightmap_level    { 0.0, 16.0 };
    static constexpr Bounds coverage_threshold { 0.0, 1.0 };
    static constexpr Bounds max_tile_jobs      { 1.0, 128.0 };
    static constexpr Bounds exact_levels       { 0.0, 8.0 };
    static constexpr Bounds samples_per_cell   { 1.0, 8.0 };
};

struct ParticleLimits {
    static constexpr Bounds max_particles { 0.0, 1000000.0 };
    static constexpr Bounds emit_distance { 8.0, 512.0 };
    static constexpr Bounds draw_distance { 8.0, 512.0 };
};

struct FaceShadingLimits {
    static constexpr Bounds up          { 0.0, 1.0 };
    static constexpr Bounds down        { 0.0, 1.0 };
    static constexpr Bounds north_south { 0.0, 1.0 };
    static constexpr Bounds east_west   { 0.0, 1.0 };
};

struct RenderLimits {
    static constexpr Bounds render_distance      { 1.0, 62500.0 };
    static constexpr Bounds render_distance_step { 1.0, 64.0 };
    static constexpr Bounds outline_width        { 1.0, 8.0 };
    static constexpr Bounds outline_inflate      { 0.0, 0.05 };
    static constexpr Bounds wave_detail          { 0.0, 8.0 };
    static constexpr Bounds capsule_segments     { 6.0, 64.0 };
    static constexpr Bounds capsule_rings        { 2.0, 16.0 };
};

struct HudLimits {
    static constexpr Bounds scale               { 0.5, 3.0 };
    static constexpr Bounds text_size           { 8.0, 32.0 };
    static constexpr Bounds line_spacing        { 1.0, 2.0 };
    static constexpr Bounds section_spacing     { 0.0, 2.0 };
    static constexpr Bounds column_gap          { 0.0, 60.0 };
    static constexpr Bounds margin              { 0.0, 60.0 };
    static constexpr Bounds padding             { 0.0, 40.0 };
    static constexpr Bounds refresh_interval    { 0.0, 2.0 };
    static constexpr Bounds shadow_offset       { 0.0, 4.0 };
    static constexpr Bounds crosshair_size      { 2.0, 40.0 };
    static constexpr Bounds crosshair_gap       { 0.0, 20.0 };
    static constexpr Bounds crosshair_thickness { 1.0, 8.0 };
    static constexpr Bounds meter_width         { 1.0, 12.0 };
};

struct EntityLimits {
    static constexpr Bounds octree_max_per_node { 1.0, 64.0 };
    static constexpr Bounds octree_max_depth    { 1.0, 16.0 };
    static constexpr Bounds octree_looseness    { 1.0, 4.0 };
    static constexpr Bounds octree_margin       { 0.0, 4.0 };
    static constexpr Bounds push_acceleration   { 0.0, 100.0 };
    static constexpr Bounds max_push_speed      { 0.0, 20.0 };
};

struct CharacterLimits {
    static constexpr Bounds width                { 0.2, 2.0 };
    static constexpr Bounds reach                { 1.0, 20.0 };
    static constexpr Bounds mass                 { 10.0, 300.0 };
    static constexpr Bounds standing_height      { 0.2, 4.0 };
    static constexpr Bounds crouching_height     { 0.2, 4.0 };
    static constexpr Bounds crawling_height      { 0.1, 4.0 };
    static constexpr Bounds swimming_height      { 0.1, 4.0 };
    static constexpr Bounds standing_eye_height  { 0.1, 4.0 };
    static constexpr Bounds crouching_eye_height { 0.1, 4.0 };
    static constexpr Bounds crawling_eye_height  { 0.05, 4.0 };
    static constexpr Bounds swimming_eye_height  { 0.05, 4.0 };
    static constexpr Bounds walk_speed           { 0.0, 50.0 };
    static constexpr Bounds sprint_speed         { 0.0, 80.0 };
    static constexpr Bounds crouch_speed         { 0.0, 30.0 };
    static constexpr Bounds crawl_speed          { 0.0, 20.0 };
    static constexpr Bounds swim_speed           { 0.0, 30.0 };
    static constexpr Bounds stroke_speed         { 0.0, 40.0 };
    static constexpr Bounds fly_speed            { 0.0, 200.0 };
    static constexpr Bounds alt_walk_speed       { 0.0, 50.0 };
    static constexpr Bounds alt_sprint_speed     { 0.0, 80.0 };
    static constexpr Bounds alt_crouch_speed     { 0.0, 30.0 };
    static constexpr Bounds alt_crawl_speed      { 0.0, 20.0 };
    static constexpr Bounds alt_swim_speed       { 0.0, 30.0 };
    static constexpr Bounds alt_stroke_speed     { 0.0, 40.0 };
    static constexpr Bounds alt_fly_speed        { 0.0, 200.0 };
    static constexpr Bounds jump_velocity        { 0.0, 40.0 };
    static constexpr Bounds ground_acceleration  { 1.0, 300.0 };
    static constexpr Bounds air_acceleration     { 0.0, 300.0 };
    static constexpr Bounds swim_acceleration    { 1.0, 300.0 };
    static constexpr Bounds stroke_acceleration  { 1.0, 300.0 };
    static constexpr Bounds swim_rise_speed      { 0.0, 10.0 };
    static constexpr Bounds swim_sink_speed      { 0.0, 10.0 };
    static constexpr Bounds swim_vertical_accel  { 0.5, 60.0 };
    static constexpr Bounds surface_leap         { 0.0, 3.0 };
    static constexpr Bounds stroke_buoyancy      { 0.0, 2.0 };
    static constexpr Bounds fly_vertical_speed   { 0.0, 100.0 };
    static constexpr Bounds fly_vertical_accel   { 1.0, 300.0 };
};

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

struct DayCycleLimits {
    static constexpr Bounds real_day_minutes { 0.25, 1440.0 };
    static constexpr Bounds sun_tilt         { 0.0, 1.0 };
    static constexpr Bounds night_brightness { 0.0, 1.0 };
    static constexpr Bounds horizon_fade     { 0.01, 0.5 };
    static constexpr Bounds twilight         { 0.01, 0.5 };
    static constexpr Bounds dusk_sky_mix     { 0.0, 1.0 };
};

struct PhysicsLimits {
    static constexpr Bounds terminal_velocity { 5.0, 400.0 };
};

struct WaveLimits {
    static constexpr Bounds height       { 0.0, 3.0 };
    static constexpr Bounds wavelength   { 3.0, 64.0 };
    static constexpr Bounds speed        { 0.0, 4.0 };
    static constexpr Bounds overtop_rate { 0.0, 5.0 };
};

struct WaterLimits {
    static constexpr Bounds fluid_density     { 100.0, 3000.0 };
    static constexpr Bounds drag_coefficient  { 0.1, 3.0 };
    static constexpr Bounds linear_drag       { 0.0, 20.0 };
    static constexpr Bounds tick_damping      { 0.0, 1.0 };
    static constexpr Bounds damping_ticks     { 1.0, 100.0 };
    static constexpr Bounds flow_interval     { 0.02, 2.0 };
    static constexpr Bounds flow_spread       { 1.0, 7.0 };
    static constexpr Bounds slope_search      { 0.0, 8.0 };
    static constexpr Bounds min_depth         { 0.004, 0.5 };
    static constexpr Bounds drop_search       { 1.0, 8.0 };
    static constexpr Bounds updates           { 64.0, 65536.0 };
    static constexpr Bounds buoyancy          { 0.0, 1.0 };
    static constexpr Bounds sink_speed        { 0.0, 20.0 };
    static constexpr Bounds current_speed     { 0.0, 10.0 };
    static constexpr Bounds current_push      { 0.0, 60.0 };
    static constexpr Bounds wade_slowdown     { 0.0, 0.9 };
    static constexpr Bounds fall_break_depth  { 0.5, 16.0 };
    static constexpr Bounds splash            { 0.0, 3.0 };
    static constexpr Bounds rain_fill         { 0.0, 2.0 };
    static constexpr Bounds soak              { 0.0, 2.0 };
    static constexpr Bounds evaporation       { 0.0, 2.0 };
    static constexpr Bounds rain_reach        { 16.0, 256.0 };
};

struct GlowLimits {
    static constexpr Bounds size     { 1.0, 30.0 };
    static constexpr Bounds strength { 0.0, 1.0 };
    static constexpr Bounds rings    { 1.0, 64.0 };
};

struct SunLimits {
    static constexpr Bounds size           { 0.1, 20.0 };
    static constexpr Bounds segments       { 3.0, 256.0 };
    static constexpr Bounds rings          { 1.0, 64.0 };
    static constexpr Bounds brightness     { 0.0, 1.0 };
    static constexpr Bounds limb_darkening { 0.0, 1.0 };
    static constexpr Bounds edge_softness  { 0.0, 1.0 };
    static constexpr Bounds light_strength { 0.0, 3.0 };
};

struct MoonLimits {
    static constexpr Bounds size           { 0.1, 20.0 };
    static constexpr Bounds segments       { 3.0, 256.0 };
    static constexpr Bounds rings          { 1.0, 64.0 };
    static constexpr Bounds brightness     { 0.0, 1.0 };
    static constexpr Bounds edge_softness  { 0.0, 1.0 };
    static constexpr Bounds day_visibility { 0.0, 1.0 };
    static constexpr Bounds cycle_days     { 1.0, 100.0 };
    static constexpr Bounds phase_offset   { 0.0, 1.0 };
    static constexpr Bounds earthshine     { 0.0, 0.5 };
    static constexpr Bounds dark_opacity   { 0.0, 1.0 };
    static constexpr Bounds terminator     { 0.0, 1.0 };
    static constexpr Bounds light_strength { 0.0, 2.0 };
};

struct StarLimits {
    static constexpr Bounds count                { 0.0, 20000.0 };
    static constexpr Bounds size                 { 0.01, 2.0 };
    static constexpr Bounds size_variation       { 0.0, 1.0 };
    static constexpr Bounds brightness           { 0.0, 2.0 };
    static constexpr Bounds brightness_variation { 0.0, 1.0 };
    static constexpr Bounds color_variation      { 0.0, 1.0 };
    static constexpr Bounds twinkle              { 0.0, 1.0 };
    static constexpr Bounds twinkle_speed        { 0.0, 10.0 };
    static constexpr Bounds day_visibility       { 0.0, 1.0 };
    static constexpr Bounds seed                 { 0.0, 99999.0 };
};

struct CelestialLimits {
    static constexpr Bounds distance     { 0.05, 0.95 };
    static constexpr Bounds horizon_fade { 0.0, 0.3 };
};

struct SeasonLimits {
    static constexpr Bounds days_per_month    { 1.0, 100.0 };
    static constexpr Bounds months_per_season { 1.0, 12.0 };
    static constexpr Bounds sun_swing         { 0.0, 45.0 };
    static constexpr Bounds temperature       { 0.0, 3.0 };
};

struct TemperatureLimits {
    static constexpr Bounds daily_strength  { 0.0, 3.0 };
    static constexpr Bounds altitude_drop   { 0.0, 5.0 };
    static constexpr Bounds offset          { -40.0, 40.0 };
    static constexpr Bounds weather_cooling { 0.0, 20.0 };
    static constexpr Bounds cloud_damping   { 0.0, 1.0 };
};

struct WeatherLimits {
    static constexpr Bounds clear_days      { 0.02, 30.0 };
    static constexpr Bounds rain_days       { 0.02, 10.0 };
    static constexpr Bounds chance          { 0.0, 1.0 };
    static constexpr Bounds change_hours    { 0.0, 12.0 };
    static constexpr Bounds threshold       { -30.0, 40.0 };
    static constexpr Bounds dry_below       { 0.0, 1.0 };
    static constexpr Bounds darkening       { 0.0, 1.0 };
    static constexpr Bounds fog             { 0.0, 20.0 };
    static constexpr Bounds lightning_rate  { 0.0, 120.0 };
    static constexpr Bounds wind            { 0.0, 5.0 };
    static constexpr Bounds season_rain     { 0.0, 5.0 };
    static constexpr Bounds showers         { 0.0, 2.0 };
    static constexpr Bounds shower_hours    { 0.25, 24.0 };
};

struct WeatherViewLimits {
    static constexpr Bounds amount    { 0.0, 3.0 };
    static constexpr Bounds max_drops { 100.0, 50000.0 };
    static constexpr Bounds radius    { 6.0, 64.0 };
};

struct BiomeOptionLimits {
    static constexpr Bounds weight       { 0.0, 4.0 };
    static constexpr Bounds size         { 0.25, 4.0 };
    static constexpr Bounds shift        { -1.0, 1.0 };
    static constexpr Bounds filler_depth { 1.0, 16.0 };
    static constexpr Bounds temperature  { -50.0, 60.0 };
    static constexpr Bounds swing        { 0.0, 60.0 };
    static constexpr Bounds rainfall     { 0.0, 4.0 };
    static constexpr Bounds waves        { 0.0, 3.0 };
};

struct CaveLimits {
    static constexpr Bounds amount         { 0.0, 4.0 };
    static constexpr Bounds size           { 0.25, 4.0 };
    static constexpr Bounds flatness       { 0.25, 4.0 };
    static constexpr Bounds cavern_roof    { 2.0, 32.0 };
    static constexpr Bounds depth          { 0.0, 512.0 };
    static constexpr Bounds deep_growth    { 0.0, 3.0 };
    static constexpr Bounds entrance_depth { 4.0, 128.0 };
    static constexpr Bounds canyon_depth   { 4.0, 96.0 };
};

struct TerrainLimits {
    static constexpr Bounds sea_level       { EngineLimits::WORLD_MIN_Z, EngineLimits::WORLD_MAX_Z };
    static constexpr Bounds bedrock_layers  { 0.0, 8.0 };
    static constexpr Bounds biome_size      { 0.25, 8.0 };
    static constexpr Bounds continent_size  { 0.25, 8.0 };
    static constexpr Bounds size_variation  { 0.0, 0.75 };
    static constexpr Bounds climate_shift   { 0.0, 0.5 };
    static constexpr Bounds ocean_amount    { 0.0, 2.0 };
    static constexpr Bounds land_height     { 4.0, 120.0 };
    static constexpr Bounds ocean_depth     { 4.0, 120.0 };
    static constexpr Bounds mountain_amount { 0.0, 2.0 };
    static constexpr Bounds mountain_height { 0.0, 300.0 };
    static constexpr Bounds flatness        { 0.0, 2.0 };
    static constexpr Bounds hill_height     { 0.0, 60.0 };
    static constexpr Bounds roughness       { 0.0, 4.0 };
    static constexpr Bounds river_width     { 0.25, 4.0 };
    static constexpr Bounds river_depth     { 1.0, 20.0 };
    static constexpr Bounds shelf_width     { 0.25, 4.0 };
    static constexpr Bounds snow_line       { 0.2, 4.0 };
};

} // namespace voxelspire

#endif // VOXELSPIRE_CORE_LIMITS_HPP