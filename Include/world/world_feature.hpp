#ifndef VOXELSPIRE_WORLD_FEATURE_HPP
#define VOXELSPIRE_WORLD_FEATURE_HPP

#include <cstdint>
#include <string>
#include <utility>
#include <vector>
#include "../block/block.hpp"
#include "../core/random.hpp"

namespace voxelspire {

class FeatureWriter {
public:
    virtual ~FeatureWriter() = default;
    virtual BlockId resolve(const std::string& block) = 0;
    virtual void set(const BlockPos& pos, BlockId block) = 0;

    virtual void fill(const BlockPos& lo, const BlockPos& hi, BlockId block) {
        for (int x = lo.x; x < hi.x; ++x)
            for (int y = lo.y; y < hi.y; ++y)
                for (int z = lo.z; z < hi.z; ++z) set({ x, y, z }, block);
    }

    void set(const BlockPos& pos, const std::string& block) { set(pos, resolve(block)); }
};

struct FeatureArea {
    int           min_x = 0, max_x = 0;
    int           min_y = 0, max_y = 0;
    int           surface_z = 0;
    std::uint64_t seed = 0;
};

class WorldFeature {
public:
    virtual ~WorldFeature() = default;
    virtual std::string id() const = 0;
    virtual void generate(FeatureWriter& out, const FeatureArea& area) const = 0;
    virtual int lowest_z(int surface_z)  const noexcept { return surface_z; }
    virtual int highest_z(int surface_z) const noexcept { return surface_z; }
};

class CheckerboardSurface final : public WorldFeature {
public:
    explicit CheckerboardSurface(std::string block, int cell_size = 1)
        : m_block(std::move(block)), m_cell(vmax(cell_size, 1)) {}

    std::string id() const override { return "voxelspire:checkerboard_surface"; }

    void generate(FeatureWriter& out, const FeatureArea& area) const override {
        const BlockId block = out.resolve(m_block);
        for (int x = area.min_x; x < area.max_x; ++x)
            for (int y = area.min_y; y < area.max_y; ++y)
                if (((floor_div(x, m_cell) + floor_div(y, m_cell)) & 1) != 0) out.set({ x, y, area.surface_z }, block);
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
    std::uint64_t seed       = 0x5EEDu;
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
    std::string     m_block;
    PillarGridShape m_shape;
};

class BlockBox final : public WorldFeature {
public:
    BlockBox(std::string block, BlockPos min_corner, BlockPos max_corner)
        : m_block(std::move(block)), m_min(min_corner), m_max(max_corner) {}

    std::string id() const override { return "voxelspire:block_box"; }
    int lowest_z(int surface_z)  const noexcept override { return vmin(surface_z, m_min.z); }
    int highest_z(int surface_z) const noexcept override { return vmax(surface_z, m_max.z - 1); }

    void generate(FeatureWriter& out, const FeatureArea& area) const override {
        const BlockPos lo{ vmax(m_min.x, area.min_x), vmax(m_min.y, area.min_y), m_min.z };
        const BlockPos hi{ vmin(m_max.x, area.max_x), vmin(m_max.y, area.max_y), m_max.z };
        if (lo.x >= hi.x || lo.y >= hi.y || lo.z >= hi.z) return;
        out.fill(lo, hi, out.resolve(m_block));
    }

private:
    std::string m_block;
    BlockPos    m_min, m_max;
};

struct StructurePart {
    std::string block;
    BlockPos    min;
    BlockPos    max;
};

class Structure final : public WorldFeature {
public:
    Structure(std::string id, int anchor_x, int anchor_y, std::vector<StructurePart> parts)
        : m_id(std::move(id)), m_x(anchor_x), m_y(anchor_y), m_parts(std::move(parts)) {
        for (const StructurePart& p : m_parts) {
            m_low  = vmin(m_low, p.min.z);
            m_high = vmax(m_high, p.max.z - 1);
        }
    }

    std::string id() const override { return m_id; }
    int lowest_z(int surface_z)  const noexcept override { return vmin(surface_z, surface_z + m_low); }
    int highest_z(int surface_z) const noexcept override { return vmax(surface_z, surface_z + m_high); }

    void generate(FeatureWriter& out, const FeatureArea& area) const override {
        for (const StructurePart& p : m_parts) {
            const BlockPos lo{ vmax(m_x + p.min.x, area.min_x), vmax(m_y + p.min.y, area.min_y), area.surface_z + p.min.z };
            const BlockPos hi{ vmin(m_x + p.max.x, area.max_x), vmin(m_y + p.max.y, area.max_y), area.surface_z + p.max.z };
            if (lo.x >= hi.x || lo.y >= hi.y || lo.z >= hi.z) continue;
            out.fill(lo, hi, out.resolve(p.block));
        }
    }

private:
    std::string                m_id;
    int                        m_x, m_y;
    std::vector<StructurePart> m_parts;
    int                        m_low  = 0;
    int                        m_high = 0;
};

} // namespace voxelspire

#endif // VOXELSPIRE_WORLD_FEATURE_HPP