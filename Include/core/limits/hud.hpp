#ifndef VOXELSPIRE_CORE_LIMITS_HUD_HPP
#define VOXELSPIRE_CORE_LIMITS_HUD_HPP

#include "bounds.hpp"

namespace voxelspire {

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

struct VitalsHudLimits {
    static constexpr Bounds scale     { 0.5, 3.0 };
    static constexpr Bounds bottom    { 0.0, 400.0 };
    static constexpr Bounds linger    { 0.0, 10.0 };
    static constexpr Bounds low_flash { 0.0, 1.0 };
};

} // namespace voxelspire

#endif // VOXELSPIRE_CORE_LIMITS_HUD_HPP