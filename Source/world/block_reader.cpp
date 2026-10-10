#include "world/block_reader.hpp"

namespace voxelspire {

BlockId BlockReader::id_at(const BlockPos& p) noexcept {
    const ChunkPos cp = World::chunk_pos_of(p);

    if (!m_cached || cp != m_pos) {
        m_chunk  = m_world->chunk_at(cp);
        m_pos    = cp;
        m_cached = true;
    }

    if (!m_chunk) return AIR_ID;
    return m_chunk->get(floor_mod(p.x, Chunk::SIZE), floor_mod(p.y, Chunk::SIZE), floor_mod(p.z, Chunk::SIZE));
}

AABB BlockReader::collision_box(const BlockPos& p, BlockId id) const noexcept {
    return m_registry->traits(id).full_cube ? AABB::unit_block(p) : m_registry->get(id).collision_box(p);
}

} // namespace voxelspire
