#ifndef VOXELSPIRE_WORLD_BLOCK_READER_HPP
#define VOXELSPIRE_WORLD_BLOCK_READER_HPP

#include "world.hpp"

namespace voxelspire {

class BlockReader {
public:
    explicit BlockReader(const World& world) noexcept : m_world(&world), m_registry(&world.blocks()) {}

    BlockId id_at(const BlockPos& p) noexcept {
        const ChunkPos cp = World::chunk_pos_of(p);

        if (!m_cached || cp != m_pos) {
            m_chunk  = m_world->chunk_at(cp);
            m_pos    = cp;
            m_cached = true;
        }

        if (!m_chunk) return AIR_ID;
        return m_chunk->get(floor_mod(p.x, Chunk::SIZE), floor_mod(p.y, Chunk::SIZE), floor_mod(p.z, Chunk::SIZE));
    }

    const BlockTraits& traits_at(const BlockPos& p) noexcept { return m_registry->traits(id_at(p)); }
    const Block&       block_at(const BlockPos& p)  noexcept { return m_registry->get(id_at(p)); }

    bool solid_at(const BlockPos& p) noexcept { return traits_at(p).solid; }
    bool fluid_at(const BlockPos& p) noexcept { return traits_at(p).fluid; }

    AABB collision_box(const BlockPos& p, BlockId id) const noexcept {
        return m_registry->traits(id).full_cube ? AABB::unit_block(p) : m_registry->get(id).collision_box(p);
    }

    const World& world() const noexcept { return *m_world; }

private:
    const World*         m_world;
    const BlockRegistry* m_registry;
    const Chunk*         m_chunk  = nullptr;
    ChunkPos             m_pos{};
    bool                 m_cached = false;
};

} // namespace voxelspire

#endif // VOXELSPIRE_WORLD_BLOCK_READER_HPP