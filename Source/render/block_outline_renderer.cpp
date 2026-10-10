#include "render/block_outline_renderer.hpp"

namespace voxelspire {

void BlockOutlineRenderer::render(fizmo::windows::Renderer& renderer, const World& world, const RaycastHit& hit, const RenderSettings& rs) const {
    const AABB box = world.block_at(hit.block).collision_box(hit.block).inflated(rs.outline_inflate);
    renderer.draw_box_3d(box.min, box.max, rs.outline_color, static_cast<float>(rs.outline_width), true);
}

} // namespace voxelspire
