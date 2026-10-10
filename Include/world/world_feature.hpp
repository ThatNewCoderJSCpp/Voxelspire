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
    virtual BlockId resolve(const Identifier& block) = 0;
    virtual void set(const BlockPos& pos, BlockId block) = 0;

    virtual void fill(const BlockPos& lo, const BlockPos& hi, BlockId block);

    void set(const BlockPos& pos, const Identifier& block) { set(pos, resolve(block)); }
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
    virtual Identifier id() const = 0;
    virtual void generate(FeatureWriter& out, const FeatureArea& area) const = 0;
    virtual int lowest_z(int surface_z)  const noexcept { return surface_z; }
    virtual int highest_z(int surface_z) const noexcept { return surface_z; }
};

class BlockBox final : public WorldFeature {
public:
    inline static const Identifier ID = core_id(Kind::Feature, "block_box");

    BlockBox(Identifier block, BlockPos min_corner, BlockPos max_corner)
        : m_block(block), m_min(min_corner), m_max(max_corner) {}

    Identifier id() const override { return ID; }
    int lowest_z(int surface_z)  const noexcept override { return vmin(surface_z, m_min.z); }
    int highest_z(int surface_z) const noexcept override { return vmax(surface_z, m_max.z - 1); }

    void generate(FeatureWriter& out, const FeatureArea& area) const override;

private:
    Identifier m_block;
    BlockPos   m_min, m_max;
};

struct StructurePart {
    Identifier block;
    BlockPos   min;
    BlockPos   max;
};

class Structure final : public WorldFeature {
public:
    Structure(Identifier id, int anchor_x, int anchor_y, std::vector<StructurePart> parts)
;

    Identifier id() const override { return m_id; }
    int lowest_z(int surface_z)  const noexcept override { return vmin(surface_z, surface_z + m_low); }
    int highest_z(int surface_z) const noexcept override { return vmax(surface_z, surface_z + m_high); }

    void generate(FeatureWriter& out, const FeatureArea& area) const override;

private:
    Identifier                 m_id;
    int                        m_x, m_y;
    std::vector<StructurePart> m_parts;
    int                        m_low  = 0;
    int                        m_high = 0;
};

} // namespace voxelspire

#endif // VOXELSPIRE_WORLD_FEATURE_HPP