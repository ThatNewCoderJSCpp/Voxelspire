#ifndef VOXELSPIRE_CORE_TYPES_HPP
#define VOXELSPIRE_CORE_TYPES_HPP

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include "../fizmo.hpp"

namespace voxelspire {

using fizmo::vector3d;
using fizmo::graphics::Color;

template <typename T> constexpr T vmin(T a, T b) noexcept { return a < b ? a : b; }
template <typename T> constexpr T vmax(T a, T b) noexcept { return a > b ? a : b; }
template <typename T> constexpr T vclamp(T v, T lo, T hi) noexcept { return v < lo ? lo : (v > hi ? hi : v); }

constexpr double PI = fizmo::constants::pi();
constexpr double deg_to_rad(double d) noexcept { return d * fizmo::constants::pi_180(); }
constexpr double rad_to_deg(double r) noexcept { return r * fizmo::constants::reciprocal_pi_180(); }

inline int floor_to_int(double v) noexcept {
    constexpr double lo = static_cast<double>(std::numeric_limits<int>::min());
    constexpr double hi = static_cast<double>(std::numeric_limits<int>::max());
    if (std::isnan(v)) return 0;
    return static_cast<int>(vclamp(std::floor(v), lo, hi));
}

constexpr int floor_div(int a, int b) noexcept { return (a >= 0) ? a / b : -((-a + b - 1) / b); }
constexpr int floor_mod(int a, int b) noexcept { return a - floor_div(a, b) * b; }

inline double component(const vector3d& v, int axis) noexcept { return axis == 0 ? v.x : (axis == 1 ? v.y : v.z); }
inline void set_component(vector3d& v, int axis, double value) noexcept { (axis == 0 ? v.x : (axis == 1 ? v.y : v.z)) = value; }
inline vector3d lerp(const vector3d& a, const vector3d& b, double t) noexcept { return a + (b - a) * t; }

struct BlockPos {
    int x = 0, y = 0, z = 0;

    constexpr BlockPos() noexcept = default;
    constexpr BlockPos(int x_, int y_, int z_) noexcept : x(x_), y(y_), z(z_) {}

    static BlockPos containing(const vector3d& p) noexcept { return { floor_to_int(p.x), floor_to_int(p.y), floor_to_int(p.z) }; }

    constexpr BlockPos operator+(const BlockPos& o) const noexcept { return { x + o.x, y + o.y, z + o.z }; }
    constexpr BlockPos operator-(const BlockPos& o) const noexcept { return { x - o.x, y - o.y, z - o.z }; }
    constexpr bool operator==(const BlockPos& o) const noexcept { return x == o.x && y == o.y && z == o.z; }
    constexpr bool operator!=(const BlockPos& o) const noexcept { return !(*this == o); }

    vector3d min_corner() const noexcept { return { double(x), double(y), double(z) }; }
    vector3d center() const noexcept { return { x + 0.5, y + 0.5, z + 0.5 }; }
};

struct BlockPosHash {
    std::size_t operator()(const BlockPos& p) const noexcept {
        std::uint64_t h = static_cast<std::uint32_t>(p.x) * 73856093ull;
        h ^= static_cast<std::uint32_t>(p.y) * 19349663ull;
        h ^= static_cast<std::uint32_t>(p.z) * 83492791ull;
        return static_cast<std::size_t>(h);
    }
};

enum class Face : std::uint8_t { West = 0, East, South, North, Down, Up };
constexpr int FACE_COUNT = 6;
constexpr std::array<Face, FACE_COUNT> ALL_FACES = { Face::West, Face::East, Face::South, Face::North, Face::Down, Face::Up };

// +x (east), +y (north), +z (up)
constexpr BlockPos face_offset(Face f) noexcept {
    switch (f) {
        case Face::West:  return { -1,  0,  0 };
        case Face::East:  return {  1,  0,  0 };
        case Face::South: return {  0, -1,  0 };
        case Face::North: return {  0,  1,  0 };
        case Face::Down:  return {  0,  0, -1 };
        case Face::Up:    return {  0,  0,  1 };
    }
    return {};
}

inline vector3d face_normal(Face f) noexcept {
    const BlockPos o = face_offset(f);
    return { double(o.x), double(o.y), double(o.z) };
}

constexpr int face_axis(Face f) noexcept { return static_cast<int>(f) / 2; }
constexpr bool face_positive(Face f) noexcept { return (static_cast<int>(f) & 1) != 0; }

constexpr Face opposite(Face f) noexcept {
    return static_cast<Face>(static_cast<int>(f) ^ 1);
}

constexpr const char* face_name(Face f) noexcept {
    switch (f) {
        case Face::West:  return "west";
        case Face::East:  return "east";
        case Face::South: return "south";
        case Face::North: return "north";
        case Face::Down:  return "down";
        case Face::Up:    return "up";
    }
    return "?";
}

struct AABB {
    vector3d min{}, max{};

    AABB() noexcept = default;
    AABB(const vector3d& mn, const vector3d& mx) noexcept : min(mn), max(mx) {}

    static AABB unit_block(const BlockPos& p) noexcept { return { p.min_corner(), p.min_corner() + vector3d{1.0, 1.0, 1.0} }; }

    AABB translated(const vector3d& d) const noexcept { return { min + d, max + d }; }
    AABB inflated(double e) const noexcept { return { min - vector3d{e, e, e}, max + vector3d{e, e, e} }; }

    AABB united(const AABB& o) const noexcept {
        return { { vmin(min.x, o.min.x), vmin(min.y, o.min.y), vmin(min.z, o.min.z) },
                 { vmax(max.x, o.max.x), vmax(max.y, o.max.y), vmax(max.z, o.max.z) } };
    }

    bool overlaps_on(int axis, const AABB& o, double eps = 1e-9) const noexcept {
        return component(max, axis) > component(o.min, axis) + eps && component(min, axis) < component(o.max, axis) - eps;
    }

    bool intersects(const AABB& o, double eps = 1e-9) const noexcept {
        return overlaps_on(0, o, eps) && overlaps_on(1, o, eps) && overlaps_on(2, o, eps);
    }

    vector3d center() const noexcept { return (min + max) * 0.5; }
};

} // namespace voxelspire

#endif // VOXELSPIRE_CORE_TYPES_HPP