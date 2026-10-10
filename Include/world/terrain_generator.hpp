#ifndef VOXELSPIRE_WORLD_TERRAIN_GENERATOR_HPP
#define VOXELSPIRE_WORLD_TERRAIN_GENERATOR_HPP

#include <array>
#include <atomic>
#include <cmath>
#include <memory>
#include <mutex>
#include <optional>
#include <vector>
#include "caves.hpp"
#include "climate.hpp"
#include "world_generator.hpp"

namespace voxelspire {

class TerrainGenerator final : public WorldGenerator {
public:
    static constexpr int    MARGIN          = 2;
    static constexpr int    SPAN            = Chunk::SIZE + 2 * MARGIN;
    static constexpr int    CLIFF_SLOPE     = 3;
    static constexpr int    WATER_GAP       = 2;
    static constexpr int    NO_WATER        = std::numeric_limits<int>::max();
    static constexpr int    SPAWN_STEP      = 16;
    static constexpr int    SPAWN_RADIUS    = 4096;
    static constexpr double SPAWN_ELEVATION = 0.08;
    static constexpr double SPAWN_HIGHEST   = 0.45;

    TerrainGenerator(const BlockRegistry& registry, BiomeRegistry biomes, TerrainSettings settings, const WorldSettings& world)
;

    const BiomeRegistry&   biomes()   const noexcept { return m_biomes; }
    const TerrainSettings& settings() const noexcept { return m_settings; }
    std::uint64_t          seed()     const noexcept { return m_seed; }

    std::unique_ptr<Chunk> generate_chunk(const ChunkPos& pos) const override {
        const auto map = column_map({ pos.x, pos.y });
        return build_chunk(*map, pos);
    }

    std::vector<std::unique_ptr<Chunk>> generate_column(const ColumnPos& col, int min_chunk_z, int max_chunk_z) const override;

    bool sample_column(int x, int y, std::vector<ColumnRun>& out) const override;

    vector3d spawn_point(const World& world) const override;

    ColumnPos spawn_column() const override {
        std::call_once(m_spawn_once, [this] { m_spawn = find_spawn(); });
        return m_spawn;
    }

    const Biome* biome_at(int x, int y) const override { return &m_biomes.at(column_at(x, y).biome); }
    std::optional<double> climate_temperature(int x, int y) const override { return m_shaper.sea_level_temperature(x, y); }

private:
    struct Surface {
        BlockId top, filler, underwater, cliff, deep;
        int     depth;
        bool    freezes;
    };

    struct Column {
        int     height = 0;
        BiomeId biome  = 0;
        bool    cliff  = false;
        int     guard  = NO_WATER;
    };

    struct ColumnMap {
        ColumnPos                                     pos;
        std::array<Column, Chunk::SIZE * Chunk::SIZE> cells{};
        int                                           max_height = 0;

        const Column& at(int lx, int ly) const noexcept { return cells[static_cast<std::size_t>(ly * Chunk::SIZE + lx)]; }
    };

    static TerrainSettings fitted(TerrainSettings s, const WorldSettings& world) noexcept {
        s.sea_level = vclamp(s.sea_level, world.min_z + s.bedrock_layers + 1, world.max_z - 2);
        return s;
    }

    static std::uint64_t next_serial() noexcept {
        static std::atomic<std::uint64_t> counter{ 0 };
        return ++counter;
    }

    static void push(std::vector<ColumnRun>& out, int z0, int z1, BlockId id) {
        if (z1 > z0) out.push_back({ z0, z1, id });
    }

    BlockId top_block(const Surface& s, const Column& c) const noexcept {
        if (c.height < m_settings.sea_level) return s.underwater;
        return c.cliff ? s.cliff : s.top;
    }

    BlockId filler_block(const Surface& s, const Column& c) const noexcept {
        if (c.height < m_settings.sea_level) return s.underwater;
        return c.cliff ? s.cliff : s.filler;
    }

    BiomeId choose(const TerrainPoint& p, double x, double y) const;

    int height_of(const TerrainPoint& p) const noexcept;

    Column column_at(int x, int y) const;

    std::shared_ptr<const ColumnMap> column_map(const ColumnPos& col) const;

    std::unique_ptr<Chunk> build_chunk(const ColumnMap& map, const ChunkPos& pos) const;

    void fill_column(Chunk& chunk, const Column& c, int lx, int ly, int base) const;

    void carve(Chunk& chunk, const ColumnMap& map, const ChunkPos& pos, int base) const;

    ColumnPos find_spawn() const;

    static constexpr std::uint64_t BEDROCK_SALT = 0xBED0C4ull;
    static constexpr double        HALF         = 0.5;

    const BlockRegistry&     m_registry;
    BiomeRegistry            m_biomes;
    TerrainSettings          m_settings;
    std::uint64_t            m_seed;
    int                      m_floor;
    int                      m_ceiling;
    TerrainShaper            m_shaper;
    CaveCarver               m_caves;
    std::uint64_t            m_serial;
    std::vector<Surface>     m_surfaces;
    BlockId                  m_stone = AIR_ID, m_bedrock = AIR_ID, m_water = AIR_ID, m_ice = AIR_ID;
    mutable std::once_flag   m_spawn_once;
    mutable ColumnPos        m_spawn{};
};

WorldGeneratorFactory terrain_world(TerrainSettings settings, BiomeRegistry biomes = BiomeRegistry::builtin());

} // namespace voxelspire

#endif // VOXELSPIRE_WORLD_TERRAIN_GENERATOR_HPP