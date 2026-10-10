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

Face face_from_normal(int nx, int ny, int nz) noexcept;

std::optional<RaycastHit> raycast_blocks(const World& world, const vector3d& origin, const vector3d& direction, double max_distance);

} // namespace voxelspire

#endif // VOXELSPIRE_WORLD_VOXEL_RAYCAST_HPP