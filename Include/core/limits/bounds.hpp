#ifndef VOXELSPIRE_CORE_LIMITS_BOUNDS_HPP
#define VOXELSPIRE_CORE_LIMITS_BOUNDS_HPP

#include <limits>
#include "../types.hpp"

namespace voxelspire {

struct Bounds {
    static constexpr double NONE = std::numeric_limits<double>::infinity();

    double min       = 0.0;
    double max       = 1.0;
    double usual_min = 0.0;
    double usual_max = 0.0;

    static constexpr Bounds at_least(double lowest, double usual_highest) noexcept { return { lowest, NONE, lowest, usual_highest }; }
    static constexpr Bounds at_most(double usual_lowest, double highest) noexcept { return { -NONE, highest, usual_lowest, highest }; }
    static constexpr Bounds any(double usual_lowest, double usual_highest) noexcept { return { -NONE, NONE, usual_lowest, usual_highest }; }

    constexpr bool has_usual() const noexcept { return usual_max > usual_min; }
    constexpr bool open_below() const noexcept { return min == -NONE; }
    constexpr bool open_above() const noexcept { return max == NONE; }

    constexpr Bounds usual() const noexcept { return has_usual() ? Bounds{ usual_min, usual_max } : Bounds{ min, max }; }

    constexpr double clamp(double v) const noexcept { return vclamp(v, min, max); }
    constexpr bool contains(double v) const noexcept { return v >= min && v <= max; }
    constexpr bool valid() const noexcept { return max >= min; }
};

} // namespace voxelspire

#endif // VOXELSPIRE_CORE_LIMITS_BOUNDS_HPP