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
        : m_registry(registry), m_biomes(std::move(biomes)), m_settings(fitted(settings, world)), m_seed(world.seed),
          m_floor(world.min_z), m_ceiling(world.max_z - 1),
          m_shaper(world.seed, m_settings), m_caves(world.seed, m_settings.caves), m_serial(next_serial()) {
        for (const auto& b : m_biomes.all()) {
            const BiomeOptions& o = settings.biome(b->id().str());
            m_biomes.set_tuning(b->id(), { o.enabled ? o.weight : 0.0, o.size, o.temperature, o.humidity });
        }

        m_stone   = registry.require(BlockIds::STONE);
        m_bedrock = registry.require(BlockIds::BEDROCK);
        m_water   = registry.require(BlockIds::WATER);
        m_ice     = registry.require(BlockIds::ICE);

        for (const auto& b : m_biomes.all()) {
            const SurfaceStyle& s = b->surface();
            const BiomeOptions& o = settings.biome(b->id().str());
            auto block = [&](const std::string& chosen, Identifier fallback) {
                const auto found = chosen.empty() ? std::nullopt : registry.find(chosen);
                return found ? *found : registry.require(fallback);
            };

            m_surfaces.push_back({ block(o.top, s.top), block(o.filler, s.filler), block(o.underwater, s.underwater),
                                   block(o.cliff, s.cliff), block(o.deep, s.deep),
                                   vmax(o.filler_depth.value_or(s.filler_depth), 1), o.freezes.value_or(s.freezes) });
        }
    }

    const BiomeRegistry&   biomes()   const noexcept { return m_biomes; }
    const TerrainSettings& settings() const noexcept { return m_settings; }
    std::uint64_t          seed()     const noexcept { return m_seed; }

    std::unique_ptr<Chunk> generate_chunk(const ChunkPos& pos) const override {
        const auto map = column_map({ pos.x, pos.y });
        return build_chunk(*map, pos);
    }

    std::vector<std::unique_ptr<Chunk>> generate_column(const ColumnPos& col, int min_chunk_z, int max_chunk_z) const override {
        const auto map = column_map(col);
        std::vector<std::unique_ptr<Chunk>> out;
        const int lo = vmax(min_chunk_z, floor_div(m_floor, Chunk::SIZE));
        const int hi = vmin(max_chunk_z, floor_div(vmax(map->max_height, m_settings.sea_level), Chunk::SIZE));
        for (int z = lo; z <= hi; ++z) if (auto c = build_chunk(*map, { col.x, col.y, z })) out.push_back(std::move(c));
        return out;
    }

    bool sample_column(int x, int y, std::vector<ColumnRun>& out) const override {
        out.clear();
        const Column c = column_at(x, y);
        const Surface& s = m_surfaces[c.biome];
        const int bed = m_floor;
        const double cut = c.height < m_settings.sea_level ? 0.0 : m_caves.canyon(x, y);
        const int top = cut > 0.0 ? vmax(c.height - static_cast<int>(std::ceil(cut)), m_settings.sea_level + 1) : c.height;
        const int filler_top = top - 1;
        const int filler_bottom = vmax(top - s.depth, bed + 1);
        push(out, bed, bed + 1, m_bedrock);
        push(out, bed + 1, filler_bottom, s.deep);
        push(out, filler_bottom, filler_top + 1, filler_block(s, c));
        push(out, top, top + 1, top_block(s, c));

        if (top < m_settings.sea_level) {
            push(out, top + 1, m_settings.sea_level, m_water);
            push(out, m_settings.sea_level, m_settings.sea_level + 1, s.freezes ? m_ice : m_water);
        }

        return true;
    }

    vector3d spawn_point(const World& world) const override {
        const ColumnPos col = spawn_column();
        const int x = col.x * Chunk::SIZE + Chunk::SIZE / 2, y = col.y * Chunk::SIZE + Chunk::SIZE / 2;

        for (int z = world.settings().max_z - 1; z >= world.settings().min_z; --z)
            if (world.block_id_at({ x, y, z }) != AIR_ID) return { x + 0.5, y + 0.5, static_cast<double>(z + 1) };

        return { x + 0.5, y + 0.5, static_cast<double>(column_at(x, y).height + 1) };
    }

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

    BiomeId choose(const TerrainPoint& p, double x, double y) const {
        const std::vector<double>& scales = m_biomes.scales();
        if (scales.size() == 1) return m_biomes.select(p.climate);
        thread_local std::vector<Climate> climates;
        climates.assign(scales.size(), p.climate);
        for (std::size_t i = 1; i < scales.size(); ++i) m_shaper.rescale(climates[i], x, y, scales[i]);
        return m_biomes.select(climates.data());
    }

    int height_of(const TerrainPoint& p) const noexcept { return vclamp(static_cast<int>(std::floor(p.height)), m_floor + m_settings.bedrock_layers + 1, m_ceiling); }

    Column column_at(int x, int y) const {
        TerrainPoint p = m_shaper.sample(x, y);
        m_shaper.finish(p, x, y);
        Column c;
        c.height = height_of(p);
        c.biome = choose(p, x, y);
        const int hx = height_of(m_shaper.sample(x + 1, y)), hy = height_of(m_shaper.sample(x, y + 1));
        c.cliff = std::abs(hx - c.height) >= CLIFF_SLOPE || std::abs(hy - c.height) >= CLIFF_SLOPE;
        return c;
    }

    std::shared_ptr<const ColumnMap> column_map(const ColumnPos& col) const {
        struct Cache { std::uint64_t serial = 0; ColumnPos pos; std::shared_ptr<const ColumnMap> map; };
        thread_local Cache cache;
        if (cache.serial == m_serial && cache.pos == col && cache.map) return cache.map;
        auto map = std::make_shared<ColumnMap>();
        map->pos = col;
        std::array<int, SPAN * SPAN> heights{};
        const int ox = col.x * Chunk::SIZE - MARGIN, oy = col.y * Chunk::SIZE - MARGIN;

        for (int y = 0; y < SPAN; ++y)
            for (int x = 0; x < SPAN; ++x) heights[static_cast<std::size_t>(y * SPAN + x)] = height_of(m_shaper.sample(ox + x, oy + y));

        auto height = [&](int x, int y) { return heights[static_cast<std::size_t>((y + MARGIN) * SPAN + x + MARGIN)]; };

        for (int ly = 0; ly < Chunk::SIZE; ++ly)
            for (int lx = 0; lx < Chunk::SIZE; ++lx) {
                const int wx = col.x * Chunk::SIZE + lx, wy = col.y * Chunk::SIZE + ly;
                TerrainPoint p = m_shaper.sample(wx, wy);
                m_shaper.finish(p, wx, wy);
                Column& c = map->cells[static_cast<std::size_t>(ly * Chunk::SIZE + lx)];
                c.height = height(lx, ly);
                c.biome = choose(p, wx, wy);
                c.cliff = std::abs(height(lx + 1, ly) - c.height) >= CLIFF_SLOPE || std::abs(height(lx - 1, ly) - c.height) >= CLIFF_SLOPE
                       || std::abs(height(lx, ly + 1) - c.height) >= CLIFF_SLOPE || std::abs(height(lx, ly - 1) - c.height) >= CLIFF_SLOPE;

                for (int dy = -MARGIN; dy <= MARGIN; ++dy)
                    for (int dx = -MARGIN; dx <= MARGIN; ++dx) {
                        const int h = height(lx + dx, ly + dy);
                        if (h < m_settings.sea_level) c.guard = vmin(c.guard, h);
                    }

                map->max_height = vmax(map->max_height, c.height);
            }

        cache = { m_serial, col, map };
        return map;
    }

    std::unique_ptr<Chunk> build_chunk(const ColumnMap& map, const ChunkPos& pos) const {
        const int base = pos.z * Chunk::SIZE;
        const int top = vmax(map.max_height, m_settings.sea_level);
        if (base > top || base + Chunk::SIZE <= m_floor) return nullptr;
        auto chunk = std::make_unique<Chunk>(pos);

        for (int ly = 0; ly < Chunk::SIZE; ++ly)
            for (int lx = 0; lx < Chunk::SIZE; ++lx) fill_column(*chunk, map.at(lx, ly), lx, ly, base);

        carve(*chunk, map, pos, base);
        if (chunk->empty()) return nullptr;
        chunk->mark_pristine();
        return chunk;
    }

    void fill_column(Chunk& chunk, const Column& c, int lx, int ly, int base) const {
        const Surface& s = m_surfaces[c.biome];
        const int bed = m_floor;
        const int filler_bottom = vmax(c.height - s.depth, bed + 1);
        auto run = [&](int z0, int z1, BlockId id) { chunk.fill(lx, lx + 1, ly, ly + 1, z0 - base, z1 - base, id); };
        run(bed, bed + 1, m_bedrock);

        for (int i = 1; i < m_settings.bedrock_layers; ++i) {
            const int z = bed + i;
            if (z < base || z >= base + Chunk::SIZE) continue;
            if (SeededRandom::at(m_seed, chunk.origin().x + lx, chunk.origin().y + ly, z, BEDROCK_SALT).integer(0, m_settings.bedrock_layers) >= i) run(z, z + 1, m_bedrock);
            else run(z, z + 1, s.deep);
        }

        run(bed + vmax(m_settings.bedrock_layers, 1), filler_bottom, s.deep);
        run(filler_bottom, c.height, filler_block(s, c));
        run(c.height, c.height + 1, top_block(s, c));

        if (c.height < m_settings.sea_level) {
            run(c.height + 1, m_settings.sea_level, m_water);
            run(m_settings.sea_level, m_settings.sea_level + 1, s.freezes ? m_ice : m_water);
        }
    }

    void carve(Chunk& chunk, const ColumnMap& map, const ChunkPos& pos, int base) const {
        if (!m_caves.enabled()) return;
        const CaveSettings& cs = m_caves.settings();
        const int floor_z = m_floor + m_settings.bedrock_layers;
        if (base + Chunk::SIZE <= floor_z || base > map.max_height) return;
        const int ox = pos.x * Chunk::SIZE, oy = pos.y * Chunk::SIZE;
        CaveCarver::Grid grid;
        m_caves.fill(grid, { ox, oy, base });
        thread_local std::vector<CaveEntrance> entrances;
        m_caves.entrances(ox, oy, ox + Chunk::SIZE, oy + Chunk::SIZE, [this](int x, int y) { return height_of(m_shaper.sample(x, y)); }, m_settings.sea_level, entrances);
        const int sea = m_settings.sea_level;

        for (int ly = 0; ly < Chunk::SIZE; ++ly)
            for (int lx = 0; lx < Chunk::SIZE; ++lx) {
                const Column& c = map.at(lx, ly);
                const bool wet = c.height < sea;
                const bool coast = !wet && c.guard != NO_WATER;
                const int wx = ox + lx, wy = oy + ly;
                const double cut = wet ? 0.0 : m_caves.canyon(wx, wy);
                int ceiling = c.height;
                if (!cs.underwater && c.guard != NO_WATER) ceiling = vmin(c.height, c.guard - WATER_GAP - 1);
                int lowest = floor_z;
                if (cs.underwater && coast) lowest = vmax(lowest, sea + 1);
                const int z0 = vmax(base, lowest), z1 = vmin(base + Chunk::SIZE - 1, ceiling);

                for (int z = z0; z <= z1; ++z) {
                    const int lz = z - base, depth = c.height - z;
                    bool hole = m_caves.hollow(grid, lx, ly, lz, depth) || (depth < cut && z > sea + 1);

                    if (!hole && !wet)
                        for (const CaveEntrance& e : entrances)
                            if (e.contains(wx + HALF, wy + HALF, z + HALF)) { hole = true; break; }

                    if (!hole) continue;
                    chunk.set(lx, ly, lz, wet && cs.underwater && z < sea ? m_water : AIR_ID);
                }
            }
    }

    ColumnPos find_spawn() const {
        for (int r = 0; r <= SPAWN_RADIUS; r += SPAWN_STEP)
            for (int dy = -r; dy <= r; dy += SPAWN_STEP)
                for (int dx = -r; dx <= r; dx += SPAWN_STEP) {
                    if (std::abs(dx) != r && std::abs(dy) != r) continue;
                    TerrainPoint p = m_shaper.sample(dx, dy);
                    if (p.climate.elevation < SPAWN_ELEVATION || p.climate.elevation > SPAWN_HIGHEST || p.climate.river > 0.0) continue;
                    return { floor_div(dx, Chunk::SIZE), floor_div(dy, Chunk::SIZE) };
                }

        return { 0, 0 };
    }

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

inline WorldGeneratorFactory terrain_world(TerrainSettings settings, BiomeRegistry biomes = BiomeRegistry::builtin()) {
    return [settings, biomes](const BlockRegistry& registry, const WorldSettings& world) -> std::unique_ptr<WorldGenerator> {
        return std::make_unique<TerrainGenerator>(registry, biomes, settings, world);
    };
}

} // namespace voxelspire

#endif // VOXELSPIRE_WORLD_TERRAIN_GENERATOR_HPP