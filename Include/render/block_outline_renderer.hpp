#ifndef VOXELSPIRE_RENDER_BLOCK_OUTLINE_HPP
#define VOXELSPIRE_RENDER_BLOCK_OUTLINE_HPP

#include "../core/settings.hpp"
#include "../world/voxel_raycast.hpp"

namespace voxelspire {

class BlockOutlineRenderer {
public:
    void render(fizmo::windows::Renderer& renderer, const World& world, const RaycastHit& hit, const RenderSettings& rs) const;
};

} // namespace voxelspire

#endif // VOXELSPIRE_RENDER_BLOCK_OUTLINE_HPP