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

class GrassBlock final : public Block {
public:
    explicit GrassBlock(double color_variation = 1.0) : Block(BlockIds::GRASS, with_variation(color_variation)) {}

    FaceAppearance face_appearance(Face face) const override {
        if (face == Face::Down) return appearance(Color(134, 96, 67), 6);
        if (face == Face::Up)   return appearance(Color(96, 170, 62), 7);
        return appearance(Color(84, 150, 55), 5);
    }
};

class DirtBlock final : public Block {
public:
    explicit DirtBlock(double color_variation = 1.0) : Block(BlockIds::DIRT, with_variation(color_variation)) {}
    FaceAppearance face_appearance(Face) const override { return appearance(Color(134, 96, 67), 6); }
};

class StoneBlock final : public Block {
public:
    explicit StoneBlock(double color_variation = 1.0) : Block(BlockIds::STONE, with_variation(color_variation)) {}
    FaceAppearance face_appearance(Face) const override { return appearance(Color(126, 126, 126), 8); }
};

class BedrockBlock final : public Block {
public:
    explicit BedrockBlock(double color_variation = 1.0) : Block(BlockIds::BEDROCK, with_variation(color_variation)) {}
    FaceAppearance face_appearance(Face) const override { return appearance(Color(58, 58, 58), 10); }
};

struct NaturalStyle {
    Identifier id;
    Color      color;
    int        variation = 6;
    Color      top;
    bool       own_top   = false;

    static NaturalStyle plain(Identifier id, Color color, int variation) { return { id, color, variation, color, false }; }
    static NaturalStyle topped(Identifier id, Color color, Color top, int variation) { return { id, color, variation, top, true }; }

    static NaturalStyle sand()      { return plain(BlockIds::SAND, Color(219, 207, 163), 5); }
    static NaturalStyle red_sand()  { return plain(BlockIds::RED_SAND, Color(190, 102, 33), 6); }
    static NaturalStyle sandstone() { return topped(BlockIds::SANDSTONE, Color(216, 202, 155), Color(224, 212, 168), 4); }
    static NaturalStyle gravel()    { return plain(BlockIds::GRAVEL, Color(131, 127, 126), 12); }
    static NaturalStyle clay()      { return plain(BlockIds::CLAY, Color(160, 166, 179), 4); }
    static NaturalStyle snow()      { return plain(BlockIds::SNOW, Color(240, 248, 250), 3); }
    static NaturalStyle mud()       { return plain(BlockIds::MUD, Color(60, 57, 61), 6); }
};

class NaturalBlock final : public Block {
public:
    NaturalBlock(const NaturalStyle& style, double color_variation = 1.0) : Block(style.id, with_variation(color_variation)), m_style(style) {}

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
        : Block(BlockIds::ICE, with_variation(color_variation, BlockProperties::see_through(true, false, LIGHT_OPACITY).finished(SurfaceFinish::Glossy))) {}
    FaceAppearance face_appearance(Face) const override { return appearance(Color(145, 183, 253, 170), 2); }
};

class GlassBlock final : public Block {
public:
    explicit GlassBlock(double color_variation = 1.0)
        : Block(BlockIds::GLASS, with_variation(color_variation, BlockProperties::see_through(true).finished(SurfaceFinish::Glossy))) {}
    FaceAppearance face_appearance(Face) const override { return appearance(Color(205, 232, 240, 48), 0); }
};

class WaterBlock final : public Block {
public:
    static constexpr int LIGHT_OPACITY = 1;

    explicit WaterBlock(double color_variation = 1.0)
        : Block(BlockIds::WATER, with_variation(color_variation, BlockProperties::see_through(false, true, LIGHT_OPACITY).finished(SurfaceFinish::Liquid))) {}
    FaceAppearance face_appearance(Face) const override { return appearance(Color(46, 96, 205, 150), 3); }
};

struct LampStyle {
    Identifier    id;
    Color         color;
    LightEmission light;
    int           variation = 2;
};

class LampBlock final : public Block {
public:
    LampBlock(const LampStyle& style, double color_variation = 1.0)
        : Block(style.id, with_variation(color_variation, BlockProperties().glowing(style.light))), m_style(style) {}

    FaceAppearance face_appearance(Face) const override { return appearance(m_style.color, m_style.variation); }

    static LampStyle warm()  { return { BlockIds::WARM_LAMP,  Color(255, 214, 140), LightEmission(15, 13, 10) }; }
    static LampStyle blue()  { return { BlockIds::BLUE_LAMP,  Color(130, 175, 255), LightEmission(6, 10, 15) }; }
    static LampStyle red()   { return { BlockIds::RED_LAMP,   Color(255, 110, 90),  LightEmission(15, 5, 4) }; }
    static LampStyle green() { return { BlockIds::GREEN_LAMP, Color(120, 240, 130), LightEmission(5, 15, 6) }; }

private:
    LampStyle m_style;
};

struct MirrorStyle {
    Color  glass     = Color(222, 228, 234);
    Color  frame     = Color(96, 72, 50);
    double thickness = 1.0 / 16.0;
    int    variation = 3;
};

class MirrorBlock final : public Block {
public:
    MirrorBlock(Face facing, const MirrorStyle& style = MirrorStyle{}, double color_variation = 1.0)
        : Block(BlockIds::mirror(facing), with_variation(color_variation, BlockProperties().shaped(BlockShape::panel(opposite(facing), style.thickness)).finished(SurfaceFinish::Mirror))),
          m_facing(facing), m_style(style) {}

    Face facing() const noexcept { return m_facing; }

    FaceAppearance face_appearance(Face face) const override {
        if (face == m_facing) return appearance(m_style.glass, 0);
        return appearance(m_style.frame, m_style.variation, true);
    }

private:
    Face        m_facing;
    MirrorStyle m_style;
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
    BlockId lamp = AIR_ID;
    BlockId blue_lamp = AIR_ID;
    BlockId red_lamp = AIR_ID;
    BlockId green_lamp = AIR_ID;
    std::array<BlockId, FACE_COUNT> mirrors{};

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
        ids.lamp       = registry.add<LampBlock>(LampBlock::warm(), color_variation);
        ids.blue_lamp  = registry.add<LampBlock>(LampBlock::blue(), color_variation);
        ids.red_lamp   = registry.add<LampBlock>(LampBlock::red(), color_variation);
        ids.green_lamp = registry.add<LampBlock>(LampBlock::green(), color_variation);
        for (Face f : ALL_FACES) ids.mirrors[static_cast<std::size_t>(f)] = registry.add<MirrorBlock>(f, MirrorStyle{}, color_variation);
        return ids;
    }
};

} // namespace voxelspire

#endif // VOXELSPIRE_BLOCK_BLOCKS_HPP