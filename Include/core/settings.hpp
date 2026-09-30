#ifndef VOXELSPIRE_CORE_SETTINGS_HPP
#define VOXELSPIRE_CORE_SETTINGS_HPP

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
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
    static constexpr double MIN_RENDER_DISTANCE = 16.0;
    static constexpr double MAX_RENDER_DISTANCE = 1000000.0;
    static constexpr int CHUNK_SIZE  = 16;
    static constexpr int WORLD_MIN_Z = -512;
    static constexpr int WORLD_MAX_Z = 1024;
    static constexpr int WORLD_MAX_HORIZONTAL = 950'000'000;
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

    std::uint64_t seed      = RANDOM_SEED;
    int    min_z            = WorldDefaults::min_z;
    int    max_z            = WorldDefaults::max_z;
    double gravity          = WorldDefaults::gravity;
    double void_depth       = WorldDefaults::void_depth;
    int    horizontal_limit = EngineLimits::WORLD_MAX_HORIZONTAL;
    LightFormat light_format = LightFormat::Colored;

    std::shared_ptr<const AirResistance> air_resistance = WorldDefaults::air_resistance();
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

struct DisplaySettings {
    bool   vsync   = false;
    double max_fps = 0.0;
};

struct ControlSettings {
    double mouse_sensitivity = 0.12;
    bool   invert_y          = false;
};

struct SimulationSettings {
    double tick_rate           = 60.0;
    double game_speed          = 1.0;
    int    max_ticks_per_frame = 10;
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

struct LodSettings {
    bool        enabled            = true;
    int         tile_cells         = 32;
    int         max_level          = 12;
    int         heightmap_level    = 4;
    double      coverage_threshold = 0.5;
    std::size_t max_tile_jobs      = 32;
    int         exact_levels       = 2;
    int         samples_per_cell   = 4;
};

struct ParticleSettings {
    bool        enabled           = true;
    std::size_t max_particles     = 200000;
    double      emit_distance     = 160.0;
    double      draw_distance     = 160.0;
    double      recenter_distance = 512.0;
};

struct FaceShadingSettings {
    double up = 1.0, down = 0.5, north_south = 0.8, east_west = 0.62;
};

struct RenderSettings {
    double render_distance      = 512.0;
    double render_distance_step = 64.0;
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
    HudSections sections;
};

struct EntitySettings {
    std::size_t octree_max_per_node = 8;
    std::size_t octree_max_depth    = 8;
    double      octree_looseness    = 2.0;
    double      octree_margin       = 1.0;
    double      push_acceleration   = 24.0;
    double      max_push_speed      = 4.0;
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