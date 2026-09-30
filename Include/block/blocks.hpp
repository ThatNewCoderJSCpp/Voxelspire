#ifndef VOXELSPIRE_BLOCK_BLOCKS_HPP
#define VOXELSPIRE_BLOCK_BLOCKS_HPP

#include "block_registry.hpp"

namespace voxelspire {

inline BlockProperties with_variation(double color_variation, BlockProperties props = BlockProperties{}) noexcept {
    props.color_variation = color_variation;
    return props;
}

class GrassBlock final : public Block {
public:
    explicit GrassBlock(double color_variation = 1.0) : Block("grass", with_variation(color_variation)) {}

    FaceAppearance face_appearance(Face face) const override {
        if (face == Face::Down) return appearance(Color(134, 96, 67), 6);
        if (face == Face::Up)   return appearance(Color(96, 170, 62), 7);
        return appearance(Color(84, 150, 55), 5);
    }
};

class DirtBlock final : public Block {
public:
    explicit DirtBlock(double color_variation = 1.0) : Block("dirt", with_variation(color_variation)) {}
    FaceAppearance face_appearance(Face) const override { return appearance(Color(134, 96, 67), 6); }
};

class StoneBlock final : public Block {
public:
    explicit StoneBlock(double color_variation = 1.0) : Block("stone", with_variation(color_variation)) {}
    FaceAppearance face_appearance(Face) const override { return appearance(Color(126, 126, 126), 8); }
};

class BedrockBlock final : public Block {
public:
    explicit BedrockBlock(double color_variation = 1.0) : Block("bedrock", with_variation(color_variation)) {}
    FaceAppearance face_appearance(Face) const override { return appearance(Color(58, 58, 58), 10); }
};

class GlassBlock final : public Block {
public:
    explicit GlassBlock(double color_variation = 1.0)
        : Block("glass", with_variation(color_variation, BlockProperties::see_through(true).finished(SurfaceFinish::Glossy))) {}
    FaceAppearance face_appearance(Face) const override { return appearance(Color(205, 232, 240, 48), 0); }
};

class WaterBlock final : public Block {
public:
    static constexpr int LIGHT_OPACITY = 1;

    explicit WaterBlock(double color_variation = 1.0)
        : Block("water", with_variation(color_variation, BlockProperties::see_through(false, true, LIGHT_OPACITY).finished(SurfaceFinish::Liquid))) {}
    FaceAppearance face_appearance(Face) const override { return appearance(Color(46, 96, 205, 150), 3); }
};

struct LampStyle {
    std::string   name;
    Color         color;
    LightEmission light;
    int           variation = 2;
};

class LampBlock final : public Block {
public:
    LampBlock(const LampStyle& style, double color_variation = 1.0)
        : Block(style.name, with_variation(color_variation, BlockProperties().glowing(style.light))), m_style(style) {}

    FaceAppearance face_appearance(Face) const override { return appearance(m_style.color, m_style.variation); }

    static LampStyle warm()  { return { "lamp",      Color(255, 214, 140), LightEmission(15, 13, 10) }; }
    static LampStyle blue()  { return { "blue_lamp", Color(130, 175, 255), LightEmission(6, 10, 15) }; }
    static LampStyle red()   { return { "red_lamp",  Color(255, 110, 90),  LightEmission(15, 5, 4) }; }
    static LampStyle green() { return { "green_lamp", Color(120, 240, 130), LightEmission(5, 15, 6) }; }

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
        : Block(name_for(facing), with_variation(color_variation, BlockProperties().shaped(BlockShape::panel(opposite(facing), style.thickness)).finished(SurfaceFinish::Mirror))),
          m_facing(facing), m_style(style) {}

    Face facing() const noexcept { return m_facing; }

    FaceAppearance face_appearance(Face face) const override {
        if (face == m_facing) return appearance(m_style.glass, 0);
        return appearance(m_style.frame, m_style.variation, true);
    }

    static std::string name_for(Face facing) { return std::string("mirror_") + face_name(facing); }

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