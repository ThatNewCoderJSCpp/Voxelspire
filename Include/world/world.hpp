#ifndef VOXELSPIRE_WORLD_WORLD_HPP
#define VOXELSPIRE_WORLD_WORLD_HPP

#include <algorithm>
#include <array>
#include <functional>
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
#include <cmath>

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

    void set_physics(const WorldSettings& physics);

    const ChunkMap&   chunks()  const noexcept { return m_chunks; }
    const ColumnMap&  columns() const noexcept { return m_columns; }
    const FluidRules& fluid_rules() const noexcept { return *m_settings.fluid_rules; }
    std::uint64_t     revision()    const noexcept { return m_revision; }

    static ChunkPos chunk_pos_of(const BlockPos& p) noexcept;

    static ColumnPos column_of(const BlockPos& p) noexcept { return { floor_div(p.x, Chunk::SIZE), floor_div(p.y, Chunk::SIZE) }; }
    static ColumnPos column_of(const ChunkPos& p) noexcept { return { p.x, p.y }; }
    static ColumnPos column_of(const vector3d& p) noexcept { return column_of(BlockPos::containing(p)); }

    int min_chunk_z() const noexcept { return floor_div(m_settings.min_z, Chunk::SIZE); }
    int max_chunk_z() const noexcept { return floor_div(m_settings.max_z - 1, Chunk::SIZE); }

    bool in_build_range(const BlockPos& p) const noexcept { return p.z >= m_settings.min_z && p.z < m_settings.max_z && in_horizontal_bounds(p); }

    bool in_horizontal_bounds(const BlockPos& p) const noexcept;

    bool column_in_bounds(const ColumnPos& c) const noexcept;

    bool column_loaded(const ColumnPos& c) const noexcept { return m_columns.count(c) != 0; }
    
    const ChunkColumn* column(const ColumnPos& c) const noexcept {
        auto it = m_columns.find(c);
        return it == m_columns.end() ? nullptr : &it->second;
    }

    BlockId block_id_at(const BlockPos& p) const noexcept;

    const Block&       block_at(const BlockPos& p)  const noexcept { return m_registry->get(block_id_at(p)); }
    const BlockTraits& traits_at(const BlockPos& p) const noexcept { return m_registry->traits(block_id_at(p)); }
    bool is_solid(const BlockPos& p) const noexcept { return traits_at(p).solid; }

    std::uint8_t fluid_state(const BlockPos& p) const noexcept;

    double fluid_height(const BlockPos& p) const noexcept;

    double fluid_level(const BlockPos& p, std::uint8_t state) const noexcept;

    FluidOccupancy occupancy(const BlockPos& p) const noexcept;

    using WaveSampler = std::function<double(double, double)>;

    void   set_wave_sampler(WaveSampler sampler) { m_waves = std::move(sampler); }
    double wave_offset(double x, double y) const { return m_waves ? m_waves(x, y) : 0.0; }

    void displace(const std::vector<FluidDisplacer>& bodies);

    double fluid_surface(const BlockPos& p) const noexcept { return p.z + fluid_height(p); }

    vector3d fluid_flow(const BlockPos& p) const noexcept;

    void attach_lighting(std::unique_ptr<LightEngine> engine);

    const LightEngine* lighting() const noexcept { return m_light.get(); }
    LightEngine*       lighting()       noexcept { return m_light.get(); }
    LightFormat light_format() const noexcept { return m_light ? m_light->format() : LightFormat::None; }

    LightLevel light_at(const BlockPos& p) const noexcept { return m_light ? m_light->level_at(p) : LightLevel::open_sky(); }
    bool light_settled(const ColumnPos& c) const noexcept { return !m_light || m_light->settled(c); }

    void update_lighting(double budget_ms);

    bool set_block(const BlockPos& p, BlockId id, std::uint8_t state = 0);

    std::vector<BlockPos> take_fluid_wakes() {
        std::vector<BlockPos> out;
        out.swap(m_fluid_wakes);
        return out;
    }

    void wake_all_fluids();

    Chunk* chunk_at(const ChunkPos& cp) noexcept {
        auto it = m_chunks.find(cp);
        return it == m_chunks.end() ? nullptr : it->second.get();
    }

    const Chunk* chunk_at(const ChunkPos& cp) const noexcept {
        auto it = m_chunks.find(cp);
        return it == m_chunks.end() ? nullptr : it->second.get();
    }

    void insert_column(const ColumnPos& col, std::vector<std::unique_ptr<Chunk>> chunks);

    std::vector<std::unique_ptr<Chunk>> remove_column(const ColumnPos& col);

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
    static constexpr std::array<BlockPos, 4> HORIZONTAL_STEPS{ BlockPos{ 1, 0, 0 }, BlockPos{ -1, 0, 0 }, BlockPos{ 0, 1, 0 }, BlockPos{ 0, -1, 0 } };
    static constexpr std::array<BlockPos, 5> SPILL_STEPS{ BlockPos{ 1, 0, 0 }, BlockPos{ -1, 0, 0 }, BlockPos{ 0, 1, 0 }, BlockPos{ 0, -1, 0 }, BlockPos{ 0, 0, -1 } };
    static constexpr std::array<BlockPos, 7> WAKE_STEPS{ BlockPos{ 0, 0, 0 }, BlockPos{ 1, 0, 0 }, BlockPos{ -1, 0, 0 }, BlockPos{ 0, 1, 0 }, BlockPos{ 0, -1, 0 }, BlockPos{ 0, 0, 1 }, BlockPos{ 0, 0, -1 } };

    using OccupancyMap = std::unordered_map<BlockPos, FluidOccupancy, BlockPosHash>;

    void mark_around(const BlockPos& p);

    void occupy(const AABB& box, double share);

    void occupancy_changed(const BlockPos& p) {
        if (m_registry->traits(block_id_at(p)).fluid) mark_around(p);
        wake_around(p);
    }

    void wake_around(const BlockPos& p);

    bool spills(const BlockPos& p) const noexcept;

    void wake_chunk(const Chunk& c, int x0, int x1, int y0, int y1);

    void wake_column(const ColumnPos& col);

    void wake_column_edge(const ColumnPos& col, const BlockPos& side);

    Chunk& add_chunk(ChunkColumn& column, std::unique_ptr<Chunk> chunk);

    void mark_changed(const ChunkPos& cp);

    void rebuild_floor(ChunkColumn& column, const ColumnPos& col);

    void update_floor(ChunkColumn& column, const ColumnPos& col, int lx, int ly, int z, BlockId id);

    const BlockRegistry*                           m_registry;
    WorldSettings                                  m_settings;
    ChunkMap                                       m_chunks;
    ColumnMap                                      m_columns;
    std::vector<ChunkPos>                          m_changed;
    std::unordered_set<ChunkPos, ChunkPosHash>     m_changed_set;
    std::vector<std::pair<ColumnPos, ColumnEvent>> m_column_events;
    std::vector<ColumnPos>                         m_edited;
    std::unordered_set<ColumnPos, ColumnPosHash>   m_edited_set;
    std::unique_ptr<LightEngine>                   m_light;
    std::vector<BlockPos>                          m_fluid_wakes;
    WaveSampler                                    m_waves;
    OccupancyMap                                   m_occupancy;
    OccupancyMap                                   m_next_occupancy;
    std::uint64_t                                  m_revision = 0;
};

} // namespace voxelspire

#endif // VOXELSPIRE_WORLD_WORLD_HPP