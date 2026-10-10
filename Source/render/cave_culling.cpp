#include "render/cave_culling.hpp"

namespace voxelspire {

bool CaveCulling::inside(const ChunkPos& p) const noexcept {
    const int x = p.x - m_x0, y = p.y - m_y0, z = p.z - m_z0;
    if (x < 0 || y < 0 || z < 0 || x >= m_w || y >= m_h || z >= m_d) return false;
    return m_in_region[static_cast<std::size_t>(y) * m_w + x] != 0;
}

} // namespace voxelspire
