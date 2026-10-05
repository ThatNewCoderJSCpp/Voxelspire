#ifndef VOXELSPIRE_DEV_FLAT_WORLD_HPP
#define VOXELSPIRE_DEV_FLAT_WORLD_HPP

#include <memory>
#include <string>
#include <vector>
#include "../world/world_generator.hpp"
#include "showcase.hpp"

namespace voxelspire::dev {

class CheckerboardSurface final : public WorldFeature {
public:
    inline static const Identifier ID = core_id(Kind::Feature, { "dev", "checkerboard_surface" });

    explicit CheckerboardSurface(Identifier block, int cell_size = 1)
        : m_block(block), m_cell(vmax(cell_size, 1)) {}

    Identifier id() const override { return ID; }

    void generate(FeatureWriter& out, const FeatureArea& area) const override {
        const BlockId block = out.resolve(m_block);
        for (int x = area.min_x; x < area.max_x; ++x)
            for (int y = area.min_y; y < area.max_y; ++y)
                if (((floor_div(x, m_cell) + floor_div(y, m_cell)) & 1) != 0) out.set({ x, y, area.surface_z }, block);
    }

private:
    Identifier m_block;
    int        m_cell;
};

struct PillarGridShape {
    int           spacing_x  = 4;
    int           spacing_y  = 5;
    int           min_height = 1;
    int           max_height = 6;
    std::uint64_t seed       = 0x5EEDu;
};

class PillarGrid final : public WorldFeature {
public:
    inline static const Identifier ID = core_id(Kind::Feature, { "dev", "pillar_grid" });

    explicit PillarGrid(Identifier block, PillarGridShape shape = PillarGridShape{})
        : m_block(block), m_shape(shape) {
        m_shape.spacing_x  = vmax(m_shape.spacing_x, 1);
        m_shape.spacing_y  = vmax(m_shape.spacing_y, 1);
        m_shape.min_height = vmax(m_shape.min_height, 0);
        m_shape.max_height = vmax(m_shape.max_height, m_shape.min_height);
    }

    Identifier id() const override { return ID; }
    int highest_z(int surface_z) const noexcept override { return surface_z + m_shape.max_height; }

    void generate(FeatureWriter& out, const FeatureArea& area) const override {
        const BlockId block = out.resolve(m_block);

        for (int x = area.min_x; x < area.max_x; ++x) {
            if (floor_mod(x, m_shape.spacing_x) != 0) continue;

            for (int y = area.min_y; y < area.max_y; ++y) {
                if (floor_mod(y, m_shape.spacing_y) != 0) continue;
                const int height = SeededRandom::at(area.seed, x, y, 0, m_shape.seed).integer(m_shape.min_height, m_shape.max_height);
                out.fill({ x, y, area.surface_z + 1 }, { x + 1, y + 1, area.surface_z + height + 1 }, block);
            }
        }
    }

private:
    Identifier      m_block;
    PillarGridShape m_shape;
};

struct FlatLayer {
    Identifier block;
    int        thickness = 1;
};

struct FlatWorldPreset {
    int half_width = 1024;
    int bottom_z   =    0;

    std::vector<FlatLayer> layers = {
        { BlockIds::BEDROCK, 1 },
        { BlockIds::STONE,   4 },
        { BlockIds::DIRT,    5 },
        { BlockIds::GRASS,   2 }
    };

    std::vector<std::shared_ptr<const WorldFeature>> features = {
        std::make_shared<CheckerboardSurface>(BlockIds::STONE),
        std::make_shared<PillarGrid>(BlockIds::DIRT),
        Showcase::feature()
    };

    int top_z() const noexcept {
        int z = bottom_z;
        for (const FlatLayer& l : layers) z += vmax(l.thickness, 0);
        return z - 1;
    }
};

class FlatWorldGenerator final : public WorldGenerator {
public:
    FlatWorldGenerator(FlatWorldPreset preset, const BlockRegistry& registry, std::uint64_t seed)
        : m_preset(std::move(preset)), m_registry(registry), m_seed(seed) {
        for (const FlatLayer& layer : m_preset.layers)
            m_layer_ids.insert(m_layer_ids.end(), static_cast<std::size_t>(vmax(layer.thickness, 0)), registry.require(layer.block));

        m_lowest  = m_preset.bottom_z;
        m_highest = m_preset.top_z();

        for (const auto& f : m_preset.features) {
            if (!f) continue;
            m_lowest  = vmin(m_lowest, f->lowest_z(m_preset.top_z()));
            m_highest = vmax(m_highest, f->highest_z(m_preset.top_z()));
        }
    }

    const FlatWorldPreset& preset() const noexcept { return m_preset; }
    std::uint64_t          seed()   const noexcept { return m_seed; }

    std::unique_ptr<Chunk> generate_chunk(const ChunkPos& pos) const override {
        const BlockPos o{ pos.x * Chunk::SIZE, pos.y * Chunk::SIZE, pos.z * Chunk::SIZE };
        const int lo = -m_preset.half_width, hi = m_preset.half_width;
        if (o.z > m_highest || o.z + Chunk::SIZE <= m_lowest) return nullptr;
        if (o.x >= hi || o.x + Chunk::SIZE <= lo || o.y >= hi || o.y + Chunk::SIZE <= lo) return nullptr;
        auto chunk = std::make_unique<Chunk>(pos);
        const int x0 = vmax(o.x, lo) - o.x, x1 = vmin(o.x + Chunk::SIZE, hi) - o.x;
        const int y0 = vmax(o.y, lo) - o.y, y1 = vmin(o.y + Chunk::SIZE, hi) - o.y;

        for (int lz = 0; lz < Chunk::SIZE; ++lz) {
            const int layer = o.z + lz - m_preset.bottom_z;
            if (layer < 0 || layer >= static_cast<int>(m_layer_ids.size())) continue;
            chunk->fill(x0, x1, y0, y1, lz, lz + 1, m_layer_ids[static_cast<std::size_t>(layer)]);
        }

        if (!m_preset.features.empty()) {
            ChunkFeatureWriter writer(m_registry, *chunk);
            const FeatureArea area{ o.x + x0, o.x + x1, o.y + y0, o.y + y1, m_preset.top_z(), m_seed };
            for (const auto& feature : m_preset.features) if (feature) feature->generate(writer, area);
        }

        if (chunk->empty()) return nullptr;
        chunk->mark_pristine();
        return chunk;
    }

    bool sample_column(int x, int y, std::vector<ColumnRun>& out) const override {
        out.clear();
        const int lo = -m_preset.half_width, hi = m_preset.half_width;
        if (x < lo || x >= hi || y < lo || y >= hi) return true;
        const int z0 = m_lowest;
        std::vector<BlockId> ids(static_cast<std::size_t>(m_highest - m_lowest + 1), AIR_ID);
        std::copy(m_layer_ids.begin(), m_layer_ids.end(), ids.begin() + (m_preset.bottom_z - m_lowest));
        ColumnSampleWriter writer(m_registry, x, y, z0, ids);
        const FeatureArea area{ x, x + 1, y, y + 1, m_preset.top_z(), m_seed };
        for (const auto& feature : m_preset.features) if (feature) feature->generate(writer, area);

        for (std::size_t i = 0; i < ids.size(); ++i) {
            if (ids[i] == AIR_ID) continue;
            const int z = z0 + static_cast<int>(i);
            if (!out.empty() && out.back().id == ids[i] && out.back().z1 == z) ++out.back().z1;
            else out.push_back({ z, z + 1, ids[i] });
        }

        return true;
    }

    vector3d spawn_point(const World& world) const override {
        const BlockPos column{ 0, 0, 0 };
        
        for (int z = world.settings().max_z - 1; z >= world.settings().min_z; --z) {
            if (world.is_solid({ column.x, column.y, z })) return { column.x + 0.5, column.y + 0.5, static_cast<double>(z + 1) };
        }

        return { column.x + 0.5, column.y + 0.5, static_cast<double>(m_preset.top_z() + 1) };
    }

private:
    FlatWorldPreset      m_preset;
    const BlockRegistry& m_registry;
    std::uint64_t        m_seed;
    std::vector<BlockId> m_layer_ids;
    int                  m_lowest  = 0;
    int                  m_highest = 0;
};

inline WorldGeneratorFactory flat_world(FlatWorldPreset preset = FlatWorldPreset{}) {
    return [preset](const BlockRegistry& registry, const WorldSettings& world) -> std::unique_ptr<WorldGenerator> {
        return std::make_unique<FlatWorldGenerator>(preset, registry, world.seed);
    };
}

} // namespace voxelspire::dev

#endif // VOXELSPIRE_DEV_FLAT_WORLD_HPP