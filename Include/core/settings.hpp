#ifndef VOXELSPIRE_CORE_SETTINGS_HPP
#define VOXELSPIRE_CORE_SETTINGS_HPP

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include "limits.hpp"
#include "types.hpp"
#include "../entity/character_settings.hpp"
#include "../input/input_bindings.hpp"
#include "../lighting/dynamic_light.hpp"
#include "../lighting/settings.hpp"
#include "../physics/settings.hpp"
#include "../sky/settings.hpp"
#include "../world/showcase.hpp"
#include "../world/world_feature.hpp"

namespace voxelspire {

struct EngineLimits {
    static constexpr int    CHUNK_SIZE           = 16;
    static constexpr int    WORLD_MIN_Z          = -512;
    static constexpr int    WORLD_MAX_Z          = 1024;
    static constexpr int    WORLD_MAX_HORIZONTAL = 950'000'000;
};

struct WorldDefaults {
    static constexpr double gravity           = 32.0;
    static constexpr double terminal_velocity = 78.4;
    static constexpr double void_depth        = 64.0;
    static constexpr int    min_z             = -128;
    static constexpr int    max_z             = 512;
    static constexpr double fluid_buoyancy    = 0.85;
    static constexpr double fluid_sink_speed  = 2.0;

    static std::shared_ptr<const AirResistance> air_resistance() {
        return std::make_shared<QuadraticDrag>(QuadraticDrag::coefficient_for(terminal_velocity, gravity));
    }

    static std::shared_ptr<const FluidResistance> fluid_resistance() {
        return std::make_shared<QuadraticFluidDrag>();
    }
};

struct WorldSettings {
    static constexpr std::uint64_t RANDOM_SEED = 0;

    std::uint64_t seed       = RANDOM_SEED;
    int    min_z             = WorldDefaults::min_z;
    int    max_z             = WorldDefaults::max_z;
    double gravity           = WorldDefaults::gravity;
    double void_depth        = WorldDefaults::void_depth;
    int    horizontal_limit  = EngineLimits::WORLD_MAX_HORIZONTAL;
    LightFormat light_format = LightFormat::Colored;

    std::shared_ptr<const AirResistance> air_resistance     = WorldDefaults::air_resistance();
    std::shared_ptr<const FluidResistance> fluid_resistance = WorldDefaults::fluid_resistance();
    double fluid_buoyancy   = WorldDefaults::fluid_buoyancy;
    double fluid_sink_speed = WorldDefaults::fluid_sink_speed;

    double fall_height() const noexcept { return static_cast<double>(max_z - min_z) + void_depth; }

    WorldSettings validated() const {
        WorldSettings w = *this;
        w.min_z = vclamp(w.min_z, EngineLimits::WORLD_MIN_Z, EngineLimits::WORLD_MAX_Z - 1);
        w.max_z = vclamp(w.max_z, w.min_z + 1, EngineLimits::WORLD_MAX_Z);
        w.horizontal_limit = vclamp(w.horizontal_limit, EngineLimits::CHUNK_SIZE, EngineLimits::WORLD_MAX_HORIZONTAL);
        if (!w.air_resistance) w.air_resistance = std::make_shared<NoAirResistance>();
        if (!w.fluid_resistance) w.fluid_resistance = std::make_shared<NoFluidResistance>();
        return w;
    }
};

struct WorldLimits {
    static constexpr Bounds gravity          { 0.0, 100.0 };
    static constexpr Bounds fluid_buoyancy   { 0.0, 1.0 };
    static constexpr Bounds fluid_sink_speed { 0.0, 20.0 };
};

struct FlatLayer {
    std::string block;
    int         thickness = 1;
};

struct FlatWorldPreset {
    int half_width = 1024;
    int bottom_z   =    0;

    std::vector<FlatLayer> layers = { 
        { "bedrock", 1 }, 
        { "stone", 4 }, 
        { "dirt", 5 }, 
        { "grass", 2 } 
    };

    std::vector<std::shared_ptr<const WorldFeature>> features = {
        std::make_shared<CheckerboardSurface>("stone"),
        std::make_shared<PillarGrid>("dirt"),
        Showcase::feature()
    };

    int top_z() const noexcept {
        int z = bottom_z;
        for (const FlatLayer& l : layers) z += vmax(l.thickness, 0);
        return z - 1;
    }
};

struct CameraSettings {
    double fov_y                 = 70.0;
    double near_plane            = 0.05;
    double third_person_distance = 4.0;
    double collision_margin      = 0.2;
};

struct CameraLimits {
    static constexpr Bounds fov_y                 { 30.0, 130.0 };
    static constexpr Bounds near_plane            { 0.01, 1.0 };
    static constexpr Bounds third_person_distance { 1.0, 16.0 };
    static constexpr Bounds collision_margin      { 0.0, 1.0 };
};

struct DisplaySettings {
    bool vsync   = false;
    int  max_fps = 0;
};

struct DisplayLimits {
    static constexpr Bounds max_fps { 0.0, 1000.0 };
};

struct ControlSettings {
    double mouse_sensitivity = 0.12;
    bool   invert_y          = false;
};

struct ControlLimits {
    static constexpr Bounds mouse_sensitivity { 0.01, 1.0 };
};

struct SimulationSettings {
    double tick_rate           = 60.0;
    double game_speed          = 1.0;
    int    max_ticks_per_frame = 10;
};

struct SimulationLimits {
    static constexpr Bounds tick_rate           { 1.0, 480.0 };
    static constexpr Bounds game_speed          { 0.1, 10.0 };
    static constexpr Bounds max_ticks_per_frame { 1.0, 60.0 };
};

struct MenuSettings {
    bool        pause_game    = true;
    bool        save_on_close = true;
    double      scale         = 1.0;
    std::string file          = "voxelspire_settings.cfg";
    Color       backdrop      { 0, 0, 0, 150 };
    Color       panel         { 20, 23, 31, 245 };
    Color       sidebar       { 14, 16, 22, 255 };
    Color       row_hover     { 255, 255, 255, 14 };
    Color       text          { 232, 236, 244 };
    Color       muted         { 140, 150, 168 };
    Color       accent        { 92, 164, 255 };
    Color       toggle_on     { 64, 186, 104 };
    Color       control       { 42, 47, 60 };
    Color       danger        { 214, 88, 88 };
    Color       error         { 255, 96, 96 };
};

struct MenuLimits {
    static constexpr Bounds scale { 0.6, 2.0 };
};

struct StreamingSettings {
    int         simulation_distance    = 8;
    int         detail_distance        = 12;
    int         unload_margin          = 2;
    int         max_column_jobs        = 64;
    int         max_mesh_jobs          = 256;
    std::size_t upload_bytes_per_frame = 16u * 1024u * 1024u;
    double      result_time_budget_ms  = 4.0;
    int         worker_threads         = 0;
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

struct LodSettings {
    bool        enabled            = true;
    int         tile_chunks        = 2;
    int         max_level          = 12;
    int         heightmap_level    = 4;
    double      coverage_threshold = 0.5;
    std::size_t max_tile_jobs      = 32;
    int         exact_levels       = 2;
    int         samples_per_cell   = 4;
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

struct ParticleSettings {
    bool        enabled           = true;
    std::size_t max_particles     = 200000;
    double      emit_distance     = 160.0;
    double      draw_distance     = 160.0;
    double      recenter_distance = 512.0;
};

struct ParticleLimits {
    static constexpr Bounds max_particles { 0.0, 1000000.0 };
    static constexpr Bounds emit_distance { 8.0, 512.0 };
    static constexpr Bounds draw_distance { 8.0, 512.0 };
};

struct FaceShadingSettings {
    double up = 1.0, down = 0.5, north_south = 0.8, east_west = 0.62;
};

struct FaceShadingLimits {
    static constexpr Bounds up          { 0.0, 1.0 };
    static constexpr Bounds down        { 0.0, 1.0 };
    static constexpr Bounds north_south { 0.0, 1.0 };
    static constexpr Bounds east_west   { 0.0, 1.0 };
};

struct RenderSettings {
    int    render_distance      = 32;
    int    render_distance_step = 4;
    bool   merge_faces          = true;
    bool   cave_culling         = true;
    bool   face_culling         = true;
    bool   cull_void_faces      = true;
    double block_color_variation = 1.0;
    FaceShadingSettings shading;
    Color  sky_color        { 135, 190, 255 };
    Color  outline_color    { 0, 0, 0, 200 };
    unsigned int outline_width = 2;
    double outline_inflate  = 0.002;
    Color  player_color     { 45, 95, 225 };
    Color  player_visor     { 170, 215, 255 };
    int    capsule_segments = 16;
    int    capsule_rings    = 4;
    bool   first_person_body = true;

    double render_distance_blocks() const noexcept { return static_cast<double>(render_distance) * EngineLimits::CHUNK_SIZE; }
};

struct RenderLimits {
    static constexpr Bounds render_distance      { 1.0, 62500.0 };
    static constexpr Bounds render_distance_step { 1.0, 64.0 };
    static constexpr Bounds outline_width        { 1.0, 8.0 };
    static constexpr Bounds outline_inflate      { 0.0, 0.05 };
    static constexpr Bounds capsule_segments     { 6.0, 64.0 };
    static constexpr Bounds capsule_rings        { 2.0, 16.0 };
};

enum class HudCorner : std::uint8_t { TopLeft = 0, TopRight, BottomLeft, BottomRight };

struct HudSections {
    bool performance = true;
    bool player      = true;
    bool world       = true;
    bool rendering   = true;
    bool lighting    = true;
};

struct HudSettings {
    bool        show_debug       = true;
    bool        show_crosshair   = true;
    bool        show_last_key    = true;
    HudCorner   corner           = HudCorner::TopLeft;
    HudCorner   hint_corner      = HudCorner::BottomLeft;
    double      scale            = 1.0;
    double      text_size        = 15.0;
    double      hint_text_size   = 13.0;
    double      line_spacing     = 1.3;
    double      section_spacing  = 0.5;
    double      column_gap       = 14.0;
    double      margin           = 12.0;
    double      padding          = 10.0;
    double      refresh_interval = 0.25;
    bool        bold_headers     = true;
    bool        fit_to_screen    = true;
    Color       text_color       { 238, 242, 250 };
    Color       label_color      { 150, 172, 205 };
    Color       header_color     { 255, 208, 110 };
    Color       background       { 8, 10, 16, 185 };
    Color       text_shadow      { 0, 0, 0, 210 };
    double      shadow_offset    = 1.0;
    Color       crosshair_color  { 255, 255, 255, 220 };
    double      crosshair_size   = 8.0;
    double      crosshair_gap    = 0.0;
    double      crosshair_thickness = 2.0;
    double      meter_width      = 5.0;
    Color       meter_background { 255, 255, 255, 38 };
    Color       sky_meter        { 125, 195, 255 };
    Color       level_meter      { 255, 212, 96 };
    Color       red_meter        { 235, 80, 80 };
    Color       green_meter      { 90, 210, 110 };
    Color       blue_meter       { 95, 145, 255 };
    HudSections sections;
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

struct EntitySettings {
    std::size_t octree_max_per_node = 8;
    std::size_t octree_max_depth    = 8;
    double      octree_looseness    = 2.0;
    double      octree_margin       = 1.0;
    double      push_acceleration   = 24.0;
    double      max_push_speed      = 4.0;
};

struct EntityLimits {
    static constexpr Bounds octree_max_per_node { 1.0, 64.0 };
    static constexpr Bounds octree_max_depth    { 1.0, 16.0 };
    static constexpr Bounds octree_looseness    { 1.0, 4.0 };
    static constexpr Bounds octree_margin       { 0.0, 4.0 };
    static constexpr Bounds push_acceleration   { 0.0, 100.0 };
    static constexpr Bounds max_push_speed      { 0.0, 20.0 };
};

struct GameSettings {
    WorldSettings      world;
    PhysicsSettings    physics;
    CharacterSettings  character;
    InputBindings      bindings = InputBindings::defaults();
    MenuSettings       menu;
    FlatWorldPreset    flat_world;
    StreamingSettings  streaming;
    LodSettings        lod;
    LightingSettings   lighting;
    DayCycleSettings   day_cycle;
    CelestialSettings  celestial;
    DynamicLight       hand_light = DynamicLight::glow(Color(255, 190, 120), 11.0, true);
    ParticleSettings   particles;
    CameraSettings     camera;
    DisplaySettings    display;
    ControlSettings    controls;
    SimulationSettings simulation;
    EntitySettings     entities;
    RenderSettings     render;
    HudSettings        hud;
};

} // namespace voxelspire

#endif // VOXELSPIRE_CORE_SETTINGS_HPP