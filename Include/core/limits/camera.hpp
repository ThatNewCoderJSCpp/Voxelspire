#ifndef VOXELSPIRE_CORE_LIMITS_CAMERA_HPP
#define VOXELSPIRE_CORE_LIMITS_CAMERA_HPP

#include "bounds.hpp"

namespace voxelspire {

struct CameraLimits {
    static constexpr Bounds fov_y                 { 30.0, 130.0 };
    static constexpr Bounds near_plane            { 0.01, 1.0 };
    static constexpr Bounds third_person_distance { 1.0, 16.0 };
    static constexpr Bounds collision_margin      { 0.0, 1.0 };
};

struct DisplayLimits {
    static constexpr Bounds max_fps      { 0.0, 1000.0 };
    static constexpr Bounds render_scale { 0.25, 1.0 };
    static constexpr Bounds sharpness    { 0.0, 1.0 };
};

struct ControlLimits {
    static constexpr Bounds mouse_sensitivity { 0.01, 1.0 };
};

} // namespace voxelspire

#endif // VOXELSPIRE_CORE_LIMITS_CAMERA_HPP