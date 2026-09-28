#ifndef VOXELSPIRE_CORE_SETTINGS_HPP
#define VOXELSPIRE_CORE_SETTINGS_HPP

#include <memory>
#include <string>
#include <vector>
#include "types.hpp"
#include "../physics/air_resistance.hpp"
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

    static std::shared_ptr<const AirResistance> air_resistance() {
        return std::make_shared<QuadraticDrag>(QuadraticDrag::coefficient_for(terminal_velocity, gravity));
    }
};

struct WorldSettings {
    int    min_z            = WorldDefaults::min_z;
    int    max_z            = WorldDefaults::max_z;
    double gravity          = WorldDefaults::gravity;
    double void_depth       = WorldDefaults::void_depth;
    int    horizontal_limit = EngineLimits::WORLD_MAX_HORIZONTAL;

    std::shared_ptr<const AirResistance> air_resistance = WorldDefaults::air_resistance();

    double fall_height() const noexcept { return static_cast<double>(max_z - min_z) + void_depth; }

    WorldSettings validated() const {
        WorldSettings w = *this;
        w.min_z = vclamp(w.min_z, EngineLimits::WORLD_MIN_Z, EngineLimits::WORLD_MAX_Z - 1);
        w.max_z = vclamp(w.max_z, w.min_z + 1, EngineLimits::WORLD_MAX_Z);
        w.horizontal_limit = vclamp(w.horizontal_limit, EngineLimits::CHUNK_SIZE, EngineLimits::WORLD_MAX_HORIZONTAL);
        if (!w.air_resistance) w.air_resistance = std::make_shared<NoAirResistance>();
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
        std::make_shared<PillarGrid>("dirt")
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
    int    max_ticks_per_frame = 10;
};

struct RenderSettings {
    double render_distance      = 512.0;
    double render_distance_step = 64.0;
    bool   merge_faces          = true;
    double block_color_variation = 1.0;
    Color  sky_color        { 135, 190, 255 };
    Color  outline_color    { 0, 0, 0, 200 };
    unsigned int outline_width = 2;
    double outline_inflate  = 0.002;
    Color  player_color     { 45, 95, 225 };
    Color  player_visor     { 170, 215, 255 };
    int    capsule_segments = 16;
    int    capsule_rings    = 4;
    Color  crosshair_color  { 255, 255, 255, 220 };
    double hud_text_size    = 15.0;
    bool   show_debug_hud   = true;
    bool   show_last_key    = true;
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
    FlatWorldPreset    flat_world;
    CameraSettings     camera;
    DisplaySettings    display;
    ControlSettings    controls;
    SimulationSettings simulation;
    EntitySettings     entities;
    RenderSettings     render;
};

} // namespace voxelspire

#endif // VOXELSPIRE_CORE_SETTINGS_HPP