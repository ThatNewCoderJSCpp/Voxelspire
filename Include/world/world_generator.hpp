#ifndef VOXELSPIRE_WORLD_GENERATOR_HPP
#define VOXELSPIRE_WORLD_GENERATOR_HPP

#include <stdexcept>
#include "world.hpp"

namespace voxelspire {

class WorldGenerator {
public:
    virtual ~WorldGenerator() = default;
    virtual void generate(World& world) = 0;
    virtual vector3d spawn_point(const World& world) const = 0;
};

class FlatWorldGenerator final : public WorldGenerator {
public:
    explicit FlatWorldGenerator(FlatWorldPreset preset) : m_preset(std::move(preset)) {}

    const FlatWorldPreset& preset() const noexcept { return m_preset; }

    void generate(World& world) override {
        int z = m_preset.bottom_z;
        for (const FlatLayer& layer : m_preset.layers) {
            const auto id = world.blocks().find(layer.block);
            if (!id) throw std::runtime_error("flat world layer uses unknown block: " + layer.block);

            for (int i = 0; i < layer.thickness; ++i, ++z)
                for (int x = -m_preset.half_width; x < m_preset.half_width; ++x)
                    for (int y = -m_preset.half_width; y < m_preset.half_width; ++y)
                        world.set_block({ x, y, z }, *id);
        }
    }

    vector3d spawn_point(const World&) const override {
        return { 0.5, 0.5, static_cast<double>(m_preset.top_z() + 1) };
    }

private:
    FlatWorldPreset m_preset;
};

} // namespace voxelspire

#endif // VOXELSPIRE_WORLD_GENERATOR_HPP