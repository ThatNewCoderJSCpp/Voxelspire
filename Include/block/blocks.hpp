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
    explicit GrassBlock(double color_variation = 1.0) : Block(BlockIds::GRASS, with_variation(color_variation, Materials::with(Materials::thermal(0.8, 0.75)))) {}

    FaceAppearance face_appearance(Face face) const override {
        if (face == Face::Down) return appearance(Color(134, 96, 67), 6);
        if (face == Face::Up)   return appearance(Color(96, 170, 62), 7);
        return appearance(Color(84, 150, 55), 5);
    }
};

class DirtBlock final : public Block {
public:
    explicit DirtBlock(double color_variation = 1.0) : Block(BlockIds::DIRT, with_variation(color_variation, Materials::with(Materials::thermal(0.9, 0.8)))) {}
    FaceAppearance face_appearance(Face) const override { return appearance(Color(134, 96, 67), 6); }
};

class StoneBlock final : public Block {
public:
    explicit StoneBlock(double color_variation = 1.0) : Block(BlockIds::STONE, with_variation(color_variation, Materials::with(Materials::thermal(2.0, 0.6)))) {}
    FaceAppearance face_appearance(Face) const override { return appearance(Color(126, 126, 126), 8); }
};

class BedrockBlock final : public Block {
public:
    explicit BedrockBlock(double color_variation = 1.0) : Block(BlockIds::BEDROCK, with_variation(color_variation, Materials::with(Materials::thermal(2.0, 0.6)))) {}
    FaceAppearance face_appearance(Face) const override { return appearance(Color(58, 58, 58), 10); }
};

struct NaturalStyle {
    Identifier   id;
    Color        color;
    int          variation = 6;
    Color        top;
    bool         own_top   = false;
    BlockThermal thermal;

    static NaturalStyle plain(Identifier id, Color color, int variation, BlockThermal t) { return { id, color, variation, color, false, t }; }
    static NaturalStyle topped(Identifier id, Color color, Color top, int variation, BlockThermal t) { return { id, color, variation, top, true, t }; }

    static NaturalStyle sand()      { return plain(BlockIds::SAND, Color(219, 207, 163), 5, Materials::thermal(1.2, 0.75)); }
    static NaturalStyle red_sand()  { return plain(BlockIds::RED_SAND, Color(190, 102, 33), 6, Materials::thermal(1.2, 0.75)); }
    static NaturalStyle sandstone() { return topped(BlockIds::SANDSTONE, Color(216, 202, 155), Color(224, 212, 168), 4, Materials::thermal(1.6, 0.65)); }
    static NaturalStyle gravel()    { return plain(BlockIds::GRAVEL, Color(131, 127, 126), 12, Materials::thermal(1.4, 0.6)); }
    static NaturalStyle clay()      { return plain(BlockIds::CLAY, Color(160, 166, 179), 4, Materials::thermal(1.3, 0.7)); }
    static NaturalStyle snow()      { return plain(BlockIds::SNOW, Color(240, 248, 250), 3, Materials::frozen(0.6, 0.9)); }
    static NaturalStyle mud()       { return plain(BlockIds::MUD, Color(60, 57, 61), 6, Materials::thermal(1.5, 0.7)); }
};

class NaturalBlock final : public Block {
public:
    NaturalBlock(const NaturalStyle& style, double color_variation = 1.0) : Block(style.id, with_variation(color_variation, Materials::with(style.thermal))), m_style(style) {}

    FaceAppearance face_appearance(Face face) const override {
        if (m_style.own_top && (face == Face::Up || face == Face::Down)) return appearance(m_style.top, m_style.variation);
        return appearance(m_style.color, m_style.variation);
    }

private:
    NaturalStyle m_style;
};

class IceBlock final : public Block {
public:
    static constexpr int LIGHT_OPACITY = 2;

    explicit IceBlock(double color_variation = 1.0)
        : Block(BlockIds::ICE, with_variation(color_variation, Materials::with(Materials::frozen(2.2, 0.4), BlockProperties::see_through(true, false, LIGHT_OPACITY)).finished(SurfaceFinish::Glossy))) {}
    FaceAppearance face_appearance(Face) const override { return appearance(Color(145, 183, 253, 170), 2); }
};

class GlassBlock final : public Block {
public:
    explicit GlassBlock(double color_variation = 1.0)
        : Block(BlockIds::GLASS, with_variation(color_variation, Materials::with(Materials::thermal(1.0, 0.3), BlockProperties::see_through(true)).finished(SurfaceFinish::Glossy))) {}
    FaceAppearance face_appearance(Face) const override { return appearance(Color(205, 232, 240, 48), 0); }
};

class WaterBlock final : public Block {
public:
    static constexpr int LIGHT_OPACITY = 1;

    explicit WaterBlock(double color_variation = 1.0)
        : Block(BlockIds::WATER, with_variation(color_variation, BlockProperties::see_through(false, true, LIGHT_OPACITY).finished(SurfaceFinish::Liquid))) {}
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

    static DefaultBlocks register_all(BlockRegistry& registry, double color_variation = 1.0) {
        DefaultBlocks ids;
        ids.grass      = registry.add<GrassBlock>(color_variation);
        ids.dirt       = registry.add<DirtBlock>(color_variation);
        ids.stone      = registry.add<StoneBlock>(color_variation);
        ids.bedrock    = registry.add<BedrockBlock>(color_variation);
        ids.sand       = registry.add<NaturalBlock>(NaturalStyle::sand(), color_variation);
        ids.red_sand   = registry.add<NaturalBlock>(NaturalStyle::red_sand(), color_variation);
        ids.sandstone  = registry.add<NaturalBlock>(NaturalStyle::sandstone(), color_variation);
        ids.gravel     = registry.add<NaturalBlock>(NaturalStyle::gravel(), color_variation);
        ids.clay       = registry.add<NaturalBlock>(NaturalStyle::clay(), color_variation);
        ids.snow       = registry.add<NaturalBlock>(NaturalStyle::snow(), color_variation);
        ids.mud        = registry.add<NaturalBlock>(NaturalStyle::mud(), color_variation);
        ids.ice        = registry.add<IceBlock>(color_variation);
        ids.glass      = registry.add<GlassBlock>(color_variation);
        ids.water      = registry.add<WaterBlock>(color_variation);
        return ids;
    }
};

} // namespace voxelspire

#endif // VOXELSPIRE_BLOCK_BLOCKS_HPP