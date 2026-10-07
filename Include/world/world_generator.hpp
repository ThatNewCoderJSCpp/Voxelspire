#ifndef VOXELSPIRE_WORLD_GENERATOR_HPP
#define VOXELSPIRE_WORLD_GENERATOR_HPP

#include <algorithm>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>
#include "biome.hpp"
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
    virtual const Biome* biome_at(int, int) const { return nullptr; }
    virtual std::optional<double> climate_temperature(int, int) const { return std::nullopt; }

    virtual std::vector<std::unique_ptr<Chunk>> generate_column(const ColumnPos& col, int min_chunk_z, int max_chunk_z) const {
        std::vector<std::unique_ptr<Chunk>> out;
        for (int z = min_chunk_z; z <= max_chunk_z; ++z)
            if (auto c = generate_chunk({ col.x, col.y, z })) out.push_back(std::move(c));
        return out;
    }
};

using WorldGeneratorFactory = std::function<std::unique_ptr<WorldGenerator>(const BlockRegistry&, const WorldSettings&)>;

class RegistryFeatureWriter : public FeatureWriter {
public:
    explicit RegistryFeatureWriter(const BlockRegistry& registry) : m_registry(registry) {}

    BlockId resolve(const Identifier& block) override {
        auto it = m_ids.find(block);
        if (it != m_ids.end()) return it->second;
        return m_ids.emplace(block, m_registry.require(block)).first->second;
    }

    using FeatureWriter::set;

private:
    const BlockRegistry&                                    m_registry;
    std::unordered_map<Identifier, BlockId, IdentifierHash> m_ids;
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

} // namespace voxelspire

#endif // VOXELSPIRE_WORLD_GENERATOR_HPP