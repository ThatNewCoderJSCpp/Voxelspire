#ifndef VOXELSPIRE_CORE_SETTINGS_HPP
#define VOXELSPIRE_CORE_SETTINGS_HPP

#include <string>
#include <vector>
#include "types.hpp"

namespace voxelspire {

struct EngineLimits {
    static constexpr int CHUNK_SIZE   = 16;
    static constexpr int WORLD_MIN_Z  = -2048;
    static constexpr int WORLD_MAX_Z  = 2048;
};

struct WorldSettings {
    int    min_z             = -64;
    int    max_z             = 320;
    double gravity           = 32.0;
    double terminal_velocity = 78.4;
    double void_depth        = 64.0;

    WorldSettings validated() const noexcept {
        WorldSettings w = *this;
        w.min_z = vclamp(w.min_z, EngineLimits::WORLD_MIN_Z, EngineLimits::WORLD_MAX_Z - 1);
        w.max_z = vclamp(w.max_z, w.min_z + 1, EngineLimits::WORLD_MAX_Z);
        return w;
    }
};

struct FlatLayer {
    std::string block;
    int         thickness = 1;
};

struct FlatWorldPreset {
    int half_width = 128;
    int bottom_z   =   0;

    std::vector<FlatLayer> layers = { 
        { "bedrock", 1 }, 
        { "stone", 4 }, 
        { "dirt", 5 }, 
        { "grass", 2 } 
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
    double far_plane             = 1000.0;
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

struct GameSettings {
    WorldSettings      world;
    FlatWorldPreset    flat_world;
    CameraSettings     camera;
    DisplaySettings    display;
    ControlSettings    controls;
    SimulationSettings simulation;
    RenderSettings     render;
};

} // namespace voxelspire

#endif // VOXELSPIRE_CORE_SETTINGS_HPP