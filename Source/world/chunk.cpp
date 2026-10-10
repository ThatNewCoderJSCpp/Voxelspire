#include "world/chunk.hpp"

namespace voxelspire {

std::size_t ColumnPosHash::operator()(const ColumnPos& p) const noexcept {
    return static_cast<std::size_t>((static_cast<std::uint64_t>(static_cast<std::uint32_t>(p.x)) << 32) ^ static_cast<std::uint32_t>(p.y) * 0x9E3779B1u);
}

bool Chunk::set(int lx, int ly, int lz, BlockId id, std::uint8_t state) noexcept {
    const std::size_t i = index(lx, ly, lz);
    BlockId& cell = m_blocks[i];
    if (cell == id && this->state(lx, ly, lz) == state) return false;
    if (cell == AIR_ID && id != AIR_ID) ++m_non_air;
    if (cell != AIR_ID && id == AIR_ID) --m_non_air;
    cell = id;
    if (state != 0 && m_states.empty()) m_states.assign(VOLUME, 0);
    if (!m_states.empty()) m_states[i] = state;
    m_modified = true;
    ++m_revision;
    return true;
}

std::size_t Chunk::fill(int x0, int x1, int y0, int y1, int z0, int z1, BlockId id) noexcept {
    x0 = vclamp(x0, 0, SIZE); x1 = vclamp(x1, 0, SIZE);
    y0 = vclamp(y0, 0, SIZE); y1 = vclamp(y1, 0, SIZE);
    z0 = vclamp(z0, 0, SIZE); z1 = vclamp(z1, 0, SIZE);
    std::size_t changed = 0;

    for (int ly = y0; ly < y1; ++ly)
        for (int lz = z0; lz < z1; ++lz) {
            BlockId* row = &m_blocks[index(0, ly, lz)];

            for (int lx = x0; lx < x1; ++lx) {
                BlockId& cell = row[lx];
                if (cell == id) continue;
                if (cell == AIR_ID) ++m_non_air;
                if (id == AIR_ID)   --m_non_air;
                cell = id;
                if (!m_states.empty()) m_states[index(lx, ly, lz)] = 0;
                ++changed;
            }
        }

    if (changed) { m_modified = true; ++m_revision; }
    return changed;
}

void Chunk::set_blocks(const std::vector<BlockId>& blocks) noexcept {
    m_non_air = 0;

    for (int i = 0; i < VOLUME; ++i) {
        m_blocks[static_cast<std::size_t>(i)] = blocks[static_cast<std::size_t>(i)];
        if (m_blocks[static_cast<std::size_t>(i)] != AIR_ID) ++m_non_air;
    }

    ++m_revision;
}

} // namespace voxelspire
