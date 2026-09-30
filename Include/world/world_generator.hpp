#ifndef VOXELSPIRE_WORLD_GENERATOR_HPP
#define VOXELSPIRE_WORLD_GENERATOR_HPP

#include <algorithm>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
#include "world.hpp"
#include "world_feature.hpp"

namespace voxelspire {

struct ColumnRun {
    int     z0 = 0, z1 = 0;
    BlockId id = AIR_ID;
};

class WorldGenerator {
public:
    virtual ~WorldGenerator() = default;

    virtual std::unique_ptr<Chunk> generate_chunk(const ChunkPos& pos) const = 0;
    virtual bool sample_column(int, int, std::vector<ColumnRun>&) const { return false; }
    virtual vector3d spawn_point(const World& world) const = 0;
    virtual ColumnPos spawn_column() const { return { 0, 0 }; }

    std::vector<std::unique_ptr<Chunk>> generate_column(const ColumnPos& col, int min_chunk_z, int max_chunk_z) const {
        std::vector<std::unique_ptr<Chunk>> out;
        for (int z = min_chunk_z; z <= max_chunk_z; ++z)
            if (auto c = generate_chunk({ col.x, col.y, z })) out.push_back(std::move(c));
        return out;
    }
};

class RegistryFeatureWriter : public FeatureWriter {
public:
    explicit RegistryFeatureWriter(const BlockRegistry& registry) : m_registry(registry) {}

    BlockId resolve(const std::string& name) override {
        auto it = m_ids.find(name);
        if (it != m_ids.end()) return it->second;
        return m_ids.emplace(name, m_registry.require(name)).first->second;
    }

    using FeatureWriter::set;

private:
    const BlockRegistry&                     m_registry;
    std::unordered_map<std::string, BlockId> m_ids;
};

class ChunkFeatureWriter final : public RegistryFeatureWriter {
public:
    ChunkFeatureWriter(const BlockRegistry& registry, Chunk& chunk) : RegistryFeatureWriter(registry), m_chunk(chunk), m_origin(chunk.origin()) {}

    using RegistryFeatureWriter::set;

    void set(const BlockPos& pos, BlockId block) override {
        const BlockPos l = pos - m_origin;
        if (Chunk::in_bounds(l.x, l.y, l.z)) m_chunk.set(l.x, l.y, l.z, block);
    }

    void fill(const BlockPos& lo, const BlockPos& hi, BlockId block) override {
        const BlockPos a = lo - m_origin, b = hi - m_origin;
        m_chunk.fill(a.x, b.x, a.y, b.y, a.z, b.z, block);
    }

private:
    Chunk&   m_chunk;
    BlockPos m_origin;
};

class ColumnSampleWriter final : public RegistryFeatureWriter {
public:
    ColumnSampleWriter(const BlockRegistry& registry, int x, int y, int z0, std::vector<BlockId>& ids)
        : RegistryFeatureWriter(registry), m_x(x), m_y(y), m_z0(z0), m_ids(ids) {}

    using RegistryFeatureWriter::set;

    void set(const BlockPos& pos, BlockId block) override {
        if (pos.x != m_x || pos.y != m_y) return;
        const int i = pos.z - m_z0;
        if (i >= 0 && i < static_cast<int>(m_ids.size())) m_ids[static_cast<std::size_t>(i)] = block;
    }

    void fill(const BlockPos& lo, const BlockPos& hi, BlockId block) override {
        if (m_x < lo.x || m_x >= hi.x || m_y < lo.y || m_y >= hi.y) return;
        for (int z = lo.z; z < hi.z; ++z) set({ m_x, m_y, z }, block);
    }

private:
    int                   m_x, m_y, m_z0;
    std::vector<BlockId>& m_ids;
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

} // namespace voxelspire

#endif // VOXELSPIRE_WORLD_GENERATOR_HPP