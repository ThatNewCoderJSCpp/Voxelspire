#ifndef VOXELSPIRE_WORLD_WORLD_HPP
#define VOXELSPIRE_WORLD_WORLD_HPP

#include <memory>
#include <unordered_map>
#include "../block/block_registry.hpp"
#include "../core/settings.hpp"
#include "chunk.hpp"

namespace voxelspire {

class World {
public:
    using ChunkMap = std::unordered_map<ChunkPos, std::unique_ptr<Chunk>, ChunkPosHash>;

    World(const BlockRegistry& registry, const WorldSettings& settings)
        : m_registry(&registry), m_settings(settings.validated()) {}

    const BlockRegistry& blocks()   const noexcept { return *m_registry; }
    const WorldSettings& settings() const noexcept { return m_settings; }
    const ChunkMap&      chunks()   const noexcept { return m_chunks; }

    static ChunkPos chunk_pos_of(const BlockPos& p) noexcept {
        return { floor_div(p.x, Chunk::SIZE), floor_div(p.y, Chunk::SIZE), floor_div(p.z, Chunk::SIZE) };
    }

    bool in_build_range(const BlockPos& p) const noexcept { return p.z >= m_settings.min_z && p.z < m_settings.max_z && in_horizontal_bounds(p); }

    bool in_horizontal_bounds(const BlockPos& p) const noexcept {
        const int limit = m_settings.horizontal_limit;
        return p.x >= -limit && p.x < limit && p.y >= -limit && p.y < limit;
    }

    BlockId block_id_at(const BlockPos& p) const noexcept {
        const Chunk* c = chunk_at(chunk_pos_of(p));
        if (!c) return AIR_ID;
        return c->get(floor_mod(p.x, Chunk::SIZE), floor_mod(p.y, Chunk::SIZE), floor_mod(p.z, Chunk::SIZE));
    }

    const Block& block_at(const BlockPos& p) const noexcept { return m_registry->get(block_id_at(p)); }
    bool is_solid(const BlockPos& p) const noexcept { return block_at(p).is_solid(); }

    bool set_block(const BlockPos& p, BlockId id) {
        if (!in_build_range(p)) return false;
        const ChunkPos cp = chunk_pos_of(p);
        Chunk* c = chunk_at(cp);

        if (!c) {
            if (id == AIR_ID) return false;
            c = &create_chunk(cp);
        }

        const int lx = floor_mod(p.x, Chunk::SIZE), ly = floor_mod(p.y, Chunk::SIZE), lz = floor_mod(p.z, Chunk::SIZE);
        if (!c->set(lx, ly, lz, id)) return false;
        if (lx == 0)               mark_dirty({ cp.x - 1, cp.y, cp.z });
        if (lx == Chunk::SIZE - 1) mark_dirty({ cp.x + 1, cp.y, cp.z });
        if (ly == 0)               mark_dirty({ cp.x, cp.y - 1, cp.z });
        if (ly == Chunk::SIZE - 1) mark_dirty({ cp.x, cp.y + 1, cp.z });
        if (lz == 0)               mark_dirty({ cp.x, cp.y, cp.z - 1 });
        if (lz == Chunk::SIZE - 1) mark_dirty({ cp.x, cp.y, cp.z + 1 });
        return true;
    }

    Chunk* chunk_at(const ChunkPos& cp) noexcept {
        auto it = m_chunks.find(cp);
        return it == m_chunks.end() ? nullptr : it->second.get();
    }

    const Chunk* chunk_at(const ChunkPos& cp) const noexcept {
        auto it = m_chunks.find(cp);
        return it == m_chunks.end() ? nullptr : it->second.get();
    }

    std::size_t block_count() const noexcept {
        std::size_t n = 0;
        for (const auto& kv : m_chunks) n += kv.second->non_air_count();
        return n;
    }

private:
    Chunk& create_chunk(const ChunkPos& cp) {
        auto chunk = std::make_unique<Chunk>(cp);
        Chunk& ref = *chunk;
        m_chunks.emplace(cp, std::move(chunk));
        return ref;
    }

    void mark_dirty(const ChunkPos& cp) noexcept { if (Chunk* c = chunk_at(cp)) c->mark_mesh_dirty(); }

    const BlockRegistry* m_registry;
    WorldSettings        m_settings;
    ChunkMap             m_chunks;
};

} // namespace voxelspire

#endif // VOXELSPIRE_WORLD_WORLD_HPP