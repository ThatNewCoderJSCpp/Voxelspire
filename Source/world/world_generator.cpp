#include "world/world_generator.hpp"

namespace voxelspire {

std::vector<std::unique_ptr<Chunk>> WorldGenerator::generate_column(const ColumnPos& col, int min_chunk_z, int max_chunk_z) const {
    std::vector<std::unique_ptr<Chunk>> out;
    for (int z = min_chunk_z; z <= max_chunk_z; ++z)
        if (auto c = generate_chunk({ col.x, col.y, z })) out.push_back(std::move(c));
    return out;
}

BlockId RegistryFeatureWriter::resolve(const Identifier& block) {
    auto it = m_ids.find(block);
    if (it != m_ids.end()) return it->second;
    return m_ids.emplace(block, m_registry.require(block)).first->second;
}

void ChunkFeatureWriter::set(const BlockPos& pos, BlockId block) {
    const BlockPos l = pos - m_origin;
    if (Chunk::in_bounds(l.x, l.y, l.z)) m_chunk.set(l.x, l.y, l.z, block);
}

void ColumnSampleWriter::set(const BlockPos& pos, BlockId block) {
    if (pos.x != m_x || pos.y != m_y) return;
    const int i = pos.z - m_z0;
    if (i >= 0 && i < static_cast<int>(m_ids.size())) m_ids[static_cast<std::size_t>(i)] = block;
}

void ColumnSampleWriter::fill(const BlockPos& lo, const BlockPos& hi, BlockId block) {
    if (m_x < lo.x || m_x >= hi.x || m_y < lo.y || m_y >= hi.y) return;
    for (int z = lo.z; z < hi.z; ++z) set({ m_x, m_y, z }, block);
}

} // namespace voxelspire
