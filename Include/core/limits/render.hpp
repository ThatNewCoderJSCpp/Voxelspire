#ifndef VOXELSPIRE_CORE_LIMITS_RENDER_HPP
#define VOXELSPIRE_CORE_LIMITS_RENDER_HPP

#include "bounds.hpp"

namespace voxelspire {

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

} // namespace voxelspire

#endif // VOXELSPIRE_CORE_LIMITS_RENDER_HPP