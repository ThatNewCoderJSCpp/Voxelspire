#ifndef VOXELSPIRE_WORLD_GENERATOR_HPP
#define VOXELSPIRE_WORLD_GENERATOR_HPP

#include <stdexcept>
#include <string>
#include <unordered_map>
#include "world.hpp"
#include "world_feature.hpp"

namespace voxelspire {

class WorldGenerator {
public:
    virtual ~WorldGenerator() = default;
    virtual void generate(World& world) = 0;
    virtual vector3d spawn_point(const World& world) const = 0;
};

class NamedBlockWriter final : public FeatureWriter {
public:
    explicit NamedBlockWriter(World& world) : m_world(world) {}

    BlockId resolve(const std::string& name) {
        auto it = m_ids.find(name);
        if (it != m_ids.end()) return it->second;
        const auto id = m_world.blocks().find(name);
        if (!id) throw std::runtime_error("world generation uses unknown block: " + name);
        return m_ids.emplace(name, *id).first->second;
    }

    void set(const BlockPos& pos, const std::string& block) override { m_world.set_block(pos, resolve(block)); }

private:
    World&                                   m_world;
    std::unordered_map<std::string, BlockId> m_ids;
};

class FlatWorldGenerator final : public WorldGenerator {
public:
    explicit FlatWorldGenerator(FlatWorldPreset preset) : m_preset(std::move(preset)) {}

    const FlatWorldPreset& preset() const noexcept { return m_preset; }

    void generate(World& world) override {
        const int lo = -m_preset.half_width, hi = m_preset.half_width;
        NamedBlockWriter writer(world);
        int z = m_preset.bottom_z;

        for (const FlatLayer& layer : m_preset.layers) {
            const BlockId id = writer.resolve(layer.block);
            for (int i = 0; i < layer.thickness; ++i, ++z)
                for (int x = lo; x < hi; ++x)
                    for (int y = lo; y < hi; ++y)
                        world.set_block({ x, y, z }, id);
        }

        const FeatureArea area{ lo, hi, lo, hi, m_preset.top_z() };
        for (const auto& feature : m_preset.features) if (feature) feature->generate(writer, area);
    }

    vector3d spawn_point(const World& world) const override {
        const BlockPos column{ 0, 0, 0 };
        for (int z = world.settings().max_z - 1; z >= world.settings().min_z; --z) {
            if (world.is_solid({ column.x, column.y, z })) return { column.x + 0.5, column.y + 0.5, static_cast<double>(z + 1) };
        }
        return { column.x + 0.5, column.y + 0.5, static_cast<double>(m_preset.top_z() + 1) };
    }

private:
    FlatWorldPreset m_preset;
};

} // namespace voxelspire

#endif // VOXELSPIRE_WORLD_GENERATOR_HPP