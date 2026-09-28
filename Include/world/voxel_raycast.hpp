#ifndef VOXELSPIRE_WORLD_VOXEL_RAYCAST_HPP
#define VOXELSPIRE_WORLD_VOXEL_RAYCAST_HPP

#include <optional>
#include "world.hpp"

namespace voxelspire {

struct RaycastHit {
    BlockPos block;
    Face     face;
    double   distance;
    vector3d point;
};

inline Face face_from_normal(int nx, int ny, int nz) noexcept {
    if (nx < 0) return Face::West;
    if (nx > 0) return Face::East;
    if (ny < 0) return Face::South;
    if (ny > 0) return Face::North;
    if (nz < 0) return Face::Down;
    return Face::Up;
}

inline std::optional<RaycastHit> raycast_blocks(const World& world, const vector3d& origin, const vector3d& direction, double max_distance) {
    const fizmo::geometry::Ray3D ray{ origin, direction };
    std::optional<RaycastHit> result;

    fizmo::geometry::raycast_grid(ray, max_distance, [&](int x, int y, int z) {
        const BlockPos p{ x, y, z };
        const Block& block = world.block_at(p);
        if (!block.is_solid()) return false;
        const AABB box = block.collision_box(p);
        const auto hit = fizmo::geometry::ray_aabb(ray, box.min, box.max, max_distance);
        if (!hit) return false;

        const Face face = hit->inside ? Face::Up : face_from_normal(static_cast<int>(hit->normal.x), static_cast<int>(hit->normal.y), static_cast<int>(hit->normal.z));
        result = RaycastHit{ p, face, hit->distance, hit->point };
        return true;
    });

    return result;
}

} // namespace voxelspire

#endif // VOXELSPIRE_WORLD_VOXEL_RAYCAST_HPP