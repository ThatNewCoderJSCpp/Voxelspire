#ifndef VOXELSPIRE_CORE_LIMITS_PHYSICS_HPP
#define VOXELSPIRE_CORE_LIMITS_PHYSICS_HPP

#include "bounds.hpp"

namespace voxelspire {

struct HeatLimits {
    static constexpr Bounds body          { 25.0, 45.0 };
    static constexpr Bounds comfort       { -30.0, 50.0 };
    static constexpr Bounds rate          { 0.0, 1.0 };
    static constexpr Bounds recovery_rate { 0.0, 10.0 };
    static constexpr Bounds exchange      { 0.0, 20.0 };
    static constexpr Bounds strength      { 0.0, 5.0 };
    static constexpr Bounds faintest      { 0.01, 5.0 };
    static constexpr Bounds insulation    { 0.0, 3.0 };
    static constexpr Bounds speed         { 0.1, 1.0 };
    static constexpr Bounds simple_radius { 1.0, 32.0 };
    static constexpr Bounds threshold     { -40.0, 60.0 };
    static constexpr Bounds seconds       { 1.0, 600.0 };
};

struct PhysicsLimits {
    static constexpr Bounds terminal_velocity { 5.0, 400.0 };
};

} // namespace voxelspire

#endif // VOXELSPIRE_CORE_LIMITS_PHYSICS_HPP