#ifndef VOXELSPIRE_WORLD_WORLD_HPP
#define VOXELSPIRE_WORLD_WORLD_HPP

#include <algorithm>
#include <array>
#include <limits>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include "../block/block_registry.hpp"
#include "../core/settings.hpp"
#include "../lighting/engine.hpp"
#include "chunk.hpp"

namespace voxelspire {

struct ChunkColumn {
    static constexpr int           AREA     = Chunk::SIZE * Chunk::SIZE;
    static constexpr std::int32_t  NO_FLOOR = std::numeric_limits<std::int32_t>::max();

    std::array<std::int32_t, AREA> floor{};
    std::vector<int>               chunk_zs;

    static constexpr int cell(int lx, int ly) noexcept { return ly * Chunk::SIZE + lx; }
};

enum class ColumnEvent : std::uint8_t { Loaded, Unloaded };

class World {
public:
    using ChunkMap  = std::unordered_map<ChunkPos, std::unique_ptr<Chunk>, ChunkPosHash>;
    using ColumnMap = std::unordered_map<ColumnPos, ChunkColumn, ColumnPosHash>;

    World(const BlockRegistry& registry, const WorldSettings& settings)
        : m_registry(&registry), m_settings(settings.validated()) {}

    const BlockRegistry& blocks()   const noexcept { return *m_registry; }
    const WorldSettings& settings() const noexcept { return m_settings; }

    void set_physics(const WorldSettings& physics) {
        m_settings.gravity          = physics.gravity;
        m_settings.air_resistance   = physics.air_resistance;
        m_settings.fluid_resistance = physics.fluid_resistance;
        m_settings.fluid_buoyancy   = physics.fluid_buoyancy;
        m_settings.fluid_sink_speed = physics.fluid_sink_speed;
        m_settings = m_settings.validated();
    }

    const ChunkMap&  chunks()  const noexcept { return m_chunks; }
    const ColumnMap& columns() const noexcept { return m_columns; }

    static ChunkPos chunk_pos_of(const BlockPos& p) noexcept {
        return { floor_div(p.x, Chunk::SIZE), floor_div(p.y, Chunk::SIZE), floor_div(p.z, Chunk::SIZE) };
    }

    static ColumnPos column_of(const BlockPos& p) noexcept { return { floor_div(p.x, Chunk::SIZE), floor_div(p.y, Chunk::SIZE) }; }
    static ColumnPos column_of(const ChunkPos& p) noexcept { return { p.x, p.y }; }
    static ColumnPos column_of(const vector3d& p) noexcept { return column_of(BlockPos::containing(p)); }

    int min_chunk_z() const noexcept { return floor_div(m_settings.min_z, Chunk::SIZE); }
    int max_chunk_z() const noexcept { return floor_div(m_settings.max_z - 1, Chunk::SIZE); }

    bool in_build_range(const BlockPos& p) const noexcept { return p.z >= m_settings.min_z && p.z < m_settings.max_z && in_horizontal_bounds(p); }

    bool in_horizontal_bounds(const BlockPos& p) const noexcept {
        const int limit = m_settings.horizontal_limit;
        return p.x >= -limit && p.x < limit && p.y >= -limit && p.y < limit;
    }

    bool column_in_bounds(const ColumnPos& c) const noexcept {
        return in_horizontal_bounds({ c.x * Chunk::SIZE, c.y * Chunk::SIZE, 0 })
            || in_horizontal_bounds({ c.x * Chunk::SIZE + Chunk::SIZE - 1, c.y * Chunk::SIZE + Chunk::SIZE - 1, 0 });
    }

    bool column_loaded(const ColumnPos& c) const noexcept { return m_columns.count(c) != 0; }
    
    const ChunkColumn* column(const ColumnPos& c) const noexcept {
        auto it = m_columns.find(c);
        return it == m_columns.end() ? nullptr : &it->second;
    }

    BlockId block_id_at(const BlockPos& p) const noexcept {
        const Chunk* c = chunk_at(chunk_pos_of(p));
        if (!c) return AIR_ID;
        return c->get(floor_mod(p.x, Chunk::SIZE), floor_mod(p.y, Chunk::SIZE), floor_mod(p.z, Chunk::SIZE));
    }

    const Block&       block_at(const BlockPos& p)  const noexcept { return m_registry->get(block_id_at(p)); }
    const BlockTraits& traits_at(const BlockPos& p) const noexcept { return m_registry->traits(block_id_at(p)); }
    bool is_solid(const BlockPos& p) const noexcept { return traits_at(p).solid; }

    void attach_lighting(std::unique_ptr<LightEngine> engine) {
        m_light = std::move(engine);
        if (!m_light) return;
        for (const auto& kv : m_columns) m_light->column_loaded(kv.first);
    }

    const LightEngine* lighting() const noexcept { return m_light.get(); }
    LightEngine*       lighting()       noexcept { return m_light.get(); }
    LightFormat light_format() const noexcept { return m_light ? m_light->format() : LightFormat::None; }

    LightLevel light_at(const BlockPos& p) const noexcept { return m_light ? m_light->level_at(p) : LightLevel::open_sky(); }
    bool light_settled(const ColumnPos& c) const noexcept { return !m_light || m_light->settled(c); }

    void update_lighting(double budget_ms) {
        if (!m_light) return;
        m_light->update(budget_ms, [this](const ChunkPos& p) { mark_changed(p); });
    }

    bool set_block(const BlockPos& p, BlockId id) {
        if (!in_build_range(p)) return false;
        const ColumnPos col = column_of(p);
        auto colit = m_columns.find(col);
        if (colit == m_columns.end()) return false;
        const ChunkPos cp = chunk_pos_of(p);
        Chunk* c = chunk_at(cp);

        if (!c) {
            if (id == AIR_ID) return false;
            c = &add_chunk(colit->second, std::make_unique<Chunk>(cp));
        }

        const int lx = floor_mod(p.x, Chunk::SIZE), ly = floor_mod(p.y, Chunk::SIZE), lz = floor_mod(p.z, Chunk::SIZE);
        const BlockId before = c->get(lx, ly, lz);
        if (!c->set(lx, ly, lz, id)) return false;
        if (m_light) m_light->block_changed(p, before, id);
        mark_changed(cp);
        if (lx == 0)               mark_changed({ cp.x - 1, cp.y, cp.z });
        if (lx == Chunk::SIZE - 1) mark_changed({ cp.x + 1, cp.y, cp.z });
        if (ly == 0)               mark_changed({ cp.x, cp.y - 1, cp.z });
        if (ly == Chunk::SIZE - 1) mark_changed({ cp.x, cp.y + 1, cp.z });
        if (lz == 0)               mark_changed({ cp.x, cp.y, cp.z - 1 });
        if (lz == Chunk::SIZE - 1) mark_changed({ cp.x, cp.y, cp.z + 1 });
        update_floor(colit->second, col, lx, ly, p.z, id);
        if (m_edited_set.insert(col).second) m_edited.push_back(col);
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

    void insert_column(const ColumnPos& col, std::vector<std::unique_ptr<Chunk>> chunks) {
        if (column_loaded(col)) return;
        ChunkColumn& column = m_columns[col];

        for (auto& chunk : chunks) {
            if (!chunk || chunk->empty()) continue;
            add_chunk(column, std::move(chunk));
        }

        rebuild_floor(column, col);
        for (int z : column.chunk_zs) mark_changed({ col.x, col.y, z });
        m_column_events.emplace_back(col, ColumnEvent::Loaded);
        if (m_light) m_light->column_loaded(col);
    }

    std::vector<std::unique_ptr<Chunk>> remove_column(const ColumnPos& col) {
        std::vector<std::unique_ptr<Chunk>> out;
        auto it = m_columns.find(col);
        if (it == m_columns.end()) return out;

        for (int z : it->second.chunk_zs) {
            auto cit = m_chunks.find({ col.x, col.y, z });
            if (cit == m_chunks.end()) continue;
            out.push_back(std::move(cit->second));
            m_chunks.erase(cit);
        }

        m_columns.erase(it);
        m_column_events.emplace_back(col, ColumnEvent::Unloaded);
        if (m_light) m_light->column_unloaded(col);
        return out;
    }

    std::vector<ChunkPos> take_changed() {
        std::vector<ChunkPos> out;
        out.swap(m_changed);
        m_changed_set.clear();
        return out;
    }

    std::vector<ColumnPos> take_edited_columns() {
        std::vector<ColumnPos> out;
        out.swap(m_edited);
        m_edited_set.clear();
        return out;
    }

    std::vector<std::pair<ColumnPos, ColumnEvent>> take_column_events() {
        std::vector<std::pair<ColumnPos, ColumnEvent>> out;
        out.swap(m_column_events);
        return out;
    }

    std::size_t block_count() const noexcept {
        std::size_t n = 0;
        for (const auto& kv : m_chunks) n += kv.second->non_air_count();
        return n;
    }

private:
    Chunk& add_chunk(ChunkColumn& column, std::unique_ptr<Chunk> chunk) {
        Chunk& ref = *chunk;
        const ChunkPos cp = chunk->pos();
        column.chunk_zs.push_back(cp.z);
        m_chunks[cp] = std::move(chunk);
        return ref;
    }

    void mark_changed(const ChunkPos& cp) {
        if (!column_loaded(column_of(cp))) return;
        if (m_changed_set.insert(cp).second) m_changed.push_back(cp);
    }

    void rebuild_floor(ChunkColumn& column, const ColumnPos& col) {
        column.floor.fill(ChunkColumn::NO_FLOOR);
        std::vector<int> zs = column.chunk_zs;
        std::sort(zs.begin(), zs.end());
        int remaining = ChunkColumn::AREA;

        for (int cz : zs) {
            const Chunk* c = chunk_at({ col.x, col.y, cz });
            if (!c || c->empty()) continue;

            for (int ly = 0; ly < Chunk::SIZE && remaining > 0; ++ly)
                for (int lx = 0; lx < Chunk::SIZE; ++lx) {
                    std::int32_t& f = column.floor[ChunkColumn::cell(lx, ly)];
                    if (f != ChunkColumn::NO_FLOOR) continue;
                    for (int lz = 0; lz < Chunk::SIZE; ++lz)
                        if (c->get(lx, ly, lz) != AIR_ID) { f = cz * Chunk::SIZE + lz; --remaining; break; }
                }

            if (remaining == 0) break;
        }
    }

    void update_floor(ChunkColumn& column, const ColumnPos& col, int lx, int ly, int z, BlockId id) {
        std::int32_t& f = column.floor[ChunkColumn::cell(lx, ly)];
        const std::int32_t old = f;

        if (id != AIR_ID) {
            if (z < f) f = z;
        } else if (z == f) {
            f = ChunkColumn::NO_FLOOR;
            const int x = col.x * Chunk::SIZE + lx, y = col.y * Chunk::SIZE + ly;
            for (int zz = z + 1; zz < m_settings.max_z; ++zz)
                if (block_id_at({ x, y, zz }) != AIR_ID) { f = zz; break; }
        }

        if (f == old) return;
        if (old != ChunkColumn::NO_FLOOR) mark_changed({ col.x, col.y, floor_div(old, Chunk::SIZE) });
        if (f != ChunkColumn::NO_FLOOR)   mark_changed({ col.x, col.y, floor_div(f, Chunk::SIZE) });
    }

    const BlockRegistry* m_registry;
    WorldSettings        m_settings;
    ChunkMap             m_chunks;
    ColumnMap            m_columns;
    std::vector<ChunkPos> m_changed;
    std::unordered_set<ChunkPos, ChunkPosHash> m_changed_set;
    std::vector<std::pair<ColumnPos, ColumnEvent>> m_column_events;
    std::vector<ColumnPos> m_edited;
    std::unordered_set<ColumnPos, ColumnPosHash> m_edited_set;
    std::unique_ptr<LightEngine> m_light;
};

} // namespace voxelspire

#endif // VOXELSPIRE_WORLD_WORLD_HPP