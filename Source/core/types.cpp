#include "core/types.hpp"

namespace voxelspire {

int floor_to_int(double v) noexcept {
    constexpr double lo = static_cast<double>(std::numeric_limits<int>::min());
    constexpr double hi = static_cast<double>(std::numeric_limits<int>::max());
    if (std::isnan(v)) return 0;
    return static_cast<int>(vclamp(std::floor(v), lo, hi));
}

std::size_t BlockPosHash::operator()(const BlockPos& p) const noexcept {
    std::uint64_t h = static_cast<std::uint32_t>(p.x) * 73856093ull;
    h ^= static_cast<std::uint32_t>(p.y) * 19349663ull;
    h ^= static_cast<std::uint32_t>(p.z) * 83492791ull;
    return static_cast<std::size_t>(h);
}

AABB AABB::united(const AABB& o) const noexcept {
    return { { vmin(min.x, o.min.x), vmin(min.y, o.min.y), vmin(min.z, o.min.z) },
             { vmax(max.x, o.max.x), vmax(max.y, o.max.y), vmax(max.z, o.max.z) } };
}

bool AABB::overlaps_on(int axis, const AABB& o, double eps) const noexcept {
    return component(max, axis) > component(o.min, axis) + eps && component(min, axis) < component(o.max, axis) - eps;
}

} // namespace voxelspire
