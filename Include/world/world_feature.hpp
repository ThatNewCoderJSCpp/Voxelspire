#ifndef VOXELSPIRE_WORLD_FEATURE_HPP
#define VOXELSPIRE_WORLD_FEATURE_HPP

#include <cstdint>
#include <string>
#include <utility>
#include "../core/types.hpp"

namespace voxelspire {

class FeatureWriter {
public:
    virtual ~FeatureWriter() = default;
    virtual void set(const BlockPos& pos, const std::string& block) = 0;
};

struct FeatureArea {
    int min_x = 0, max_x = 0;
    int min_y = 0, max_y = 0;
    int surface_z = 0;
};

class WorldFeature {
public:
    virtual ~WorldFeature() = default;
    virtual std::string id() const = 0;
    virtual void generate(FeatureWriter& out, const FeatureArea& area) const = 0;

protected:
    static std::uint32_t hash(int x, int y, std::uint32_t seed) noexcept {
        std::uint32_t h = seed;
        h ^= static_cast<std::uint32_t>(x) * 374761393u;
        h ^= static_cast<std::uint32_t>(y) * 668265263u;
        h = (h ^ (h >> 13)) * 1274126177u;
        return h ^ (h >> 16);
    }
};

class CheckerboardSurface final : public WorldFeature {
public:
    explicit CheckerboardSurface(std::string block, int cell_size = 1)
        : m_block(std::move(block)), m_cell(vmax(cell_size, 1)) {}

    std::string id() const override { return "voxelspire:checkerboard_surface"; }

    void generate(FeatureWriter& out, const FeatureArea& area) const override {
        for (int x = area.min_x; x < area.max_x; ++x)
            for (int y = area.min_y; y < area.max_y; ++y)
                if (((floor_div(x, m_cell) + floor_div(y, m_cell)) & 1) != 0) out.set({ x, y, area.surface_z }, m_block);
    }

private:
    std::string m_block;
    int         m_cell;
};

struct PillarGridShape {
    int           spacing_x  = 4;
    int           spacing_y  = 5;
    int           min_height = 1;
    int           max_height = 6;
    std::uint32_t seed       = 0x5EEDu;
};

class PillarGrid final : public WorldFeature {
public:
    explicit PillarGrid(std::string block, PillarGridShape shape = PillarGridShape{})
        : m_block(std::move(block)), m_shape(shape) {
        m_shape.spacing_x  = vmax(m_shape.spacing_x, 1);
        m_shape.spacing_y  = vmax(m_shape.spacing_y, 1);
        m_shape.min_height = vmax(m_shape.min_height, 0);
        m_shape.max_height = vmax(m_shape.max_height, m_shape.min_height);
    }

    std::string id() const override { return "voxelspire:pillar_grid"; }

    void generate(FeatureWriter& out, const FeatureArea& area) const override {
        const std::uint32_t range = static_cast<std::uint32_t>(m_shape.max_height - m_shape.min_height + 1);

        for (int x = area.min_x; x < area.max_x; ++x) {
            if (floor_mod(x, m_shape.spacing_x) != 0) continue;

            for (int y = area.min_y; y < area.max_y; ++y) {
                if (floor_mod(y, m_shape.spacing_y) != 0) continue;
                const int height = m_shape.min_height + static_cast<int>(hash(x, y, m_shape.seed) % range);
                for (int z = 1; z <= height; ++z) out.set({ x, y, area.surface_z + z }, m_block);
            }
        }
    }

private:
    std::string     m_block;
    PillarGridShape m_shape;
};

} // namespace voxelspire

#endif // VOXELSPIRE_WORLD_FEATURE_HPP