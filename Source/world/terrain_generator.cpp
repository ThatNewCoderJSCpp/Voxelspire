#include "world/terrain_generator.hpp"

namespace voxelspire {

TerrainGenerator::TerrainGenerator(const BlockRegistry& registry, BiomeRegistry biomes, TerrainSettings settings, const WorldSettings& world) : m_registry(registry), m_biomes(std::move(biomes)), m_settings(fitted(settings, world)), m_seed(world.seed),
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

std::vector<std::unique_ptr<Chunk>> TerrainGenerator::generate_column(const ColumnPos& col, int min_chunk_z, int max_chunk_z) const {
    const auto map = column_map(col);
    std::vector<std::unique_ptr<Chunk>> out;
    const int lo = vmax(min_chunk_z, floor_div(m_floor, Chunk::SIZE));
    const int hi = vmin(max_chunk_z, floor_div(vmax(map->max_height, m_settings.sea_level), Chunk::SIZE));
    for (int z = lo; z <= hi; ++z) if (auto c = build_chunk(*map, { col.x, col.y, z })) out.push_back(std::move(c));
    return out;
}

bool TerrainGenerator::sample_column(int x, int y, std::vector<ColumnRun>& out) const {
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

vector3d TerrainGenerator::spawn_point(const World& world) const {
    const ColumnPos col = spawn_column();
    const int x = col.x * Chunk::SIZE + Chunk::SIZE / 2, y = col.y * Chunk::SIZE + Chunk::SIZE / 2;

    for (int z = world.settings().max_z - 1; z >= world.settings().min_z; --z)
        if (world.block_id_at({ x, y, z }) != AIR_ID) return { x + 0.5, y + 0.5, static_cast<double>(z + 1) };

    return { x + 0.5, y + 0.5, static_cast<double>(column_at(x, y).height + 1) };
}

BiomeId TerrainGenerator::choose(const TerrainPoint& p, double x, double y) const {
    const std::vector<double>& scales = m_biomes.scales();
    if (scales.size() == 1) return m_biomes.select(p.climate);
    thread_local std::vector<Climate> climates;
    climates.assign(scales.size(), p.climate);
    for (std::size_t i = 1; i < scales.size(); ++i) m_shaper.rescale(climates[i], x, y, scales[i]);
    return m_biomes.select(climates.data());
}

int TerrainGenerator::height_of(const TerrainPoint& p) const noexcept { return vclamp(static_cast<int>(std::floor(p.height)), m_floor + m_settings.bedrock_layers + 1, m_ceiling); }

auto TerrainGenerator::column_at(int x, int y) const -> Column {
    TerrainPoint p = m_shaper.sample(x, y);
    m_shaper.finish(p, x, y);
    Column c;
    c.height = height_of(p);
    c.biome = choose(p, x, y);
    const int hx = height_of(m_shaper.sample(x + 1, y)), hy = height_of(m_shaper.sample(x, y + 1));
    c.cliff = std::abs(hx - c.height) >= CLIFF_SLOPE || std::abs(hy - c.height) >= CLIFF_SLOPE;
    return c;
}

auto TerrainGenerator::column_map(const ColumnPos& col) const -> std::shared_ptr<const ColumnMap> {
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

std::unique_ptr<Chunk> TerrainGenerator::build_chunk(const ColumnMap& map, const ChunkPos& pos) const {
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

void TerrainGenerator::fill_column(Chunk& chunk, const Column& c, int lx, int ly, int base) const {
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

void TerrainGenerator::carve(Chunk& chunk, const ColumnMap& map, const ChunkPos& pos, int base) const {
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

ColumnPos TerrainGenerator::find_spawn() const {
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

WorldGeneratorFactory terrain_world(TerrainSettings settings, BiomeRegistry biomes) {
    return [settings, biomes](const BlockRegistry& registry, const WorldSettings& world) -> std::unique_ptr<WorldGenerator> {
        return std::make_unique<TerrainGenerator>(registry, biomes, settings, world);
    };
}

} // namespace voxelspire
