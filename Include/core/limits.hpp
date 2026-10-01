#ifndef VOXELSPIRE_CORE_LIMITS_HPP
#define VOXELSPIRE_CORE_LIMITS_HPP

#include "types.hpp"

namespace voxelspire {

struct Bounds {
    double min = 0.0;
    double max = 1.0;

    constexpr double clamp(double v) const noexcept { return vclamp(v, min, max); }
    constexpr bool contains(double v) const noexcept { return v >= min && v <= max; }
    constexpr bool valid() const noexcept { return max >= min; }
};

} // namespace voxelspire

#endif // VOXELSPIRE_CORE_LIMITS_HPP