#ifndef VOXELSPIRE_CORE_LIMITS_WORLD_HPP
#define VOXELSPIRE_CORE_LIMITS_WORLD_HPP

#include "bounds.hpp"

namespace voxelspire {

struct WorldLimits {
    static constexpr Bounds gravity { 0.0, 100.0 };
};

struct SaveLimits {
    static constexpr Bounds autosave_minutes { 0.0, 60.0 };
};

} // namespace voxelspire

#endif // VOXELSPIRE_CORE_LIMITS_WORLD_HPP