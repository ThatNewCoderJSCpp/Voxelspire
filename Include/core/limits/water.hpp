#ifndef VOXELSPIRE_CORE_LIMITS_WATER_HPP
#define VOXELSPIRE_CORE_LIMITS_WATER_HPP

#include "bounds.hpp"

namespace voxelspire {

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

} // namespace voxelspire

#endif // VOXELSPIRE_CORE_LIMITS_WATER_HPP