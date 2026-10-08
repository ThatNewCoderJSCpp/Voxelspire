#ifndef VOXELSPIRE_CORE_SETTINGS_RENDER_HPP
#define VOXELSPIRE_CORE_SETTINGS_RENDER_HPP

#include <cstddef>
#include "../limits.hpp"
#include "../types.hpp"

namespace voxelspire {

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
    int    render_distance      = 32;
    int    render_distance_step = 4;
    bool   merge_faces          = true;
    bool   cave_culling         = true;
    bool   face_culling         = true;
    bool   cull_void_faces      = true;
    double block_color_variation = 1.0;
    int    wave_detail          = 2;
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

} // namespace voxelspire

#endif // VOXELSPIRE_CORE_SETTINGS_RENDER_HPP