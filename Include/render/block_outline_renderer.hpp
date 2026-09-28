#ifndef VOXELSPIRE_RENDER_BLOCK_OUTLINE_HPP
#define VOXELSPIRE_RENDER_BLOCK_OUTLINE_HPP

#include "../core/settings.hpp"
#include "../world/voxel_raycast.hpp"

namespace voxelspire {

class BlockOutlineRenderer {
public:
    void render(fizmo::windows::Renderer& renderer, const World& world, const RaycastHit& hit, const RenderSettings& rs) const {
        const AABB box = world.block_at(hit.block).collision_box(hit.block).inflated(rs.outline_inflate);
        renderer.draw_box_3d(box.min, box.max, rs.outline_color, static_cast<float>(rs.outline_width), true);
    }
};

} // namespace voxelspire

#endif // VOXELSPIRE_RENDER_BLOCK_OUTLINE_HPP