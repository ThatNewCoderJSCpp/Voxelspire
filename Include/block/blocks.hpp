#ifndef VOXELSPIRE_BLOCK_BLOCKS_HPP
#define VOXELSPIRE_BLOCK_BLOCKS_HPP

#include <string>
#include <utility>
#include "block_registry.hpp"

namespace voxelspire {

inline BlockProperties with_variation(double color_variation, BlockProperties props = BlockProperties{}) noexcept {
    props.color_variation = color_variation;
    return props;
}

struct BlockWeights {
    static constexpr double GRASS     = 1.2;
    static constexpr double DIRT      = 1.3;
    static constexpr double STONE     = 2.6;
    static constexpr double BEDROCK   = 3.0;
    static constexpr double SAND      = 1.6;
    static constexpr double SANDSTONE = 2.3;
    static constexpr double GRAVEL    = 1.8;
    static constexpr double CLAY      = 1.8;
    static constexpr double SNOW      = 0.3;
    static constexpr double MUD       = 1.7;
    static constexpr double ICE       = 0.9;
    static constexpr double GLASS     = 2.5;
    static constexpr double WATER     = 1.0;
};

struct Materials {
    static BlockThermal thermal(double conductivity, double insulation) noexcept {
        BlockThermal t;
        t.conductivity = conductivity;
        t.insulation   = insulation;
        return t;
    }

    static BlockThermal frozen(double conductivity, double insulation) noexcept {
        BlockThermal t = thermal(conductivity, insulation);
        t.max_temperature = FREEZING;
        return t;
    }

    static BlockProperties with(BlockThermal t, BlockProperties props = BlockProperties{}) noexcept {
        props.thermal = t;
        return props;
    }

    static constexpr double FREEZING = 0.0;
};

class GrassBlock final : public Block {
public:
    explicit GrassBlock(double color_variation = 1.0);

    FaceAppearance face_appearance(Face face) const override;
};

class DirtBlock final : public Block {
public:
    explicit DirtBlock(double color_variation = 1.0);
    FaceAppearance face_appearance(Face) const override { return appearance(Color(134, 96, 67), 6); }
};

class StoneBlock final : public Block {
public:
    explicit StoneBlock(double color_variation = 1.0);
    FaceAppearance face_appearance(Face) const override { return appearance(Color(126, 126, 126), 8); }
};

class BedrockBlock final : public Block {
public:
    explicit BedrockBlock(double color_variation = 1.0);
    FaceAppearance face_appearance(Face) const override { return appearance(Color(58, 58, 58), 10); }
};

struct NaturalStyle {
    Identifier   id;
    Color        color;
    int          variation = 6;
    Color        top;
    bool         own_top   = false;
    BlockThermal thermal;
    double       weight    = BlockProperties::DEFAULT_WEIGHT;

    static NaturalStyle plain(Identifier id, Color color, int variation, BlockThermal t, double kg) { return { id, color, variation, color, false, t, kg }; }
    static NaturalStyle topped(Identifier id, Color color, Color top, int variation, BlockThermal t, double kg) { return { id, color, variation, top, true, t, kg }; }

    static NaturalStyle sand();
    static NaturalStyle red_sand();
    static NaturalStyle sandstone();
    static NaturalStyle gravel();
    static NaturalStyle clay();
    static NaturalStyle snow();
    static NaturalStyle mud();
};

class NaturalBlock final : public Block {
public:
    NaturalBlock(const NaturalStyle& style, double color_variation = 1.0);

    FaceAppearance face_appearance(Face face) const override;

private:
    NaturalStyle m_style;
};

class IceBlock final : public Block {
public:
    static constexpr int LIGHT_OPACITY = 2;

    explicit IceBlock(double color_variation = 1.0)
;
    FaceAppearance face_appearance(Face) const override { return appearance(Color(145, 183, 253, 170), 2); }
};

class GlassBlock final : public Block {
public:
    explicit GlassBlock(double color_variation = 1.0)
;
    FaceAppearance face_appearance(Face) const override { return appearance(Color(205, 232, 240, 48), 0); }
};

class WaterBlock final : public Block {
public:
    static constexpr int LIGHT_OPACITY = 1;

    explicit WaterBlock(double color_variation = 1.0)
;
    FaceAppearance face_appearance(Face) const override { return appearance(Color(46, 96, 205, 150), 3); }
};

struct DefaultBlocks {
    BlockId air = AIR_ID;
    BlockId grass = AIR_ID;
    BlockId dirt = AIR_ID;
    BlockId stone = AIR_ID;
    BlockId bedrock = AIR_ID;
    BlockId sand = AIR_ID;
    BlockId red_sand = AIR_ID;
    BlockId sandstone = AIR_ID;
    BlockId gravel = AIR_ID;
    BlockId clay = AIR_ID;
    BlockId snow = AIR_ID;
    BlockId mud = AIR_ID;
    BlockId ice = AIR_ID;
    BlockId glass = AIR_ID;
    BlockId water = AIR_ID;

    static DefaultBlocks register_all(BlockRegistry& registry, double color_variation = 1.0);
};

} // namespace voxelspire

#endif // VOXELSPIRE_BLOCK_BLOCKS_HPP