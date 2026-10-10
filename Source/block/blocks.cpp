#include "block/blocks.hpp"

namespace voxelspire {

GrassBlock::GrassBlock(double color_variation) : Block(BlockIds::GRASS, with_variation(color_variation, Materials::with(Materials::thermal(0.8, 0.75)).weighing(BlockWeights::GRASS))) {}

FaceAppearance GrassBlock::face_appearance(Face face) const {
    if (face == Face::Down) return appearance(Color(134, 96, 67), 6);
    if (face == Face::Up)   return appearance(Color(96, 170, 62), 7);
    return appearance(Color(84, 150, 55), 5);
}

DirtBlock::DirtBlock(double color_variation) : Block(BlockIds::DIRT, with_variation(color_variation, Materials::with(Materials::thermal(0.9, 0.8)).weighing(BlockWeights::DIRT))) {}

StoneBlock::StoneBlock(double color_variation) : Block(BlockIds::STONE, with_variation(color_variation, Materials::with(Materials::thermal(2.0, 0.6)).weighing(BlockWeights::STONE))) {}

BedrockBlock::BedrockBlock(double color_variation) : Block(BlockIds::BEDROCK, with_variation(color_variation, Materials::with(Materials::thermal(2.0, 0.6)).weighing(BlockWeights::BEDROCK))) {}

NaturalStyle NaturalStyle::sand() { return plain(BlockIds::SAND, Color(219, 207, 163), 5, Materials::thermal(1.2, 0.75), BlockWeights::SAND); }

NaturalStyle NaturalStyle::red_sand() { return plain(BlockIds::RED_SAND, Color(190, 102, 33), 6, Materials::thermal(1.2, 0.75), BlockWeights::SAND); }

NaturalStyle NaturalStyle::sandstone() { return topped(BlockIds::SANDSTONE, Color(216, 202, 155), Color(224, 212, 168), 4, Materials::thermal(1.6, 0.65), BlockWeights::SANDSTONE); }

NaturalStyle NaturalStyle::gravel() { return plain(BlockIds::GRAVEL, Color(131, 127, 126), 12, Materials::thermal(1.4, 0.6), BlockWeights::GRAVEL); }

NaturalStyle NaturalStyle::clay() { return plain(BlockIds::CLAY, Color(160, 166, 179), 4, Materials::thermal(1.3, 0.7), BlockWeights::CLAY); }

NaturalStyle NaturalStyle::snow() { return plain(BlockIds::SNOW, Color(240, 248, 250), 3, Materials::frozen(0.6, 0.9), BlockWeights::SNOW); }

NaturalStyle NaturalStyle::mud() { return plain(BlockIds::MUD, Color(60, 57, 61), 6, Materials::thermal(1.5, 0.7), BlockWeights::MUD); }

NaturalBlock::NaturalBlock(const NaturalStyle& style, double color_variation) : Block(style.id, with_variation(color_variation, Materials::with(style.thermal).weighing(style.weight))), m_style(style) {}

FaceAppearance NaturalBlock::face_appearance(Face face) const {
    if (m_style.own_top && (face == Face::Up || face == Face::Down)) return appearance(m_style.top, m_style.variation);
    return appearance(m_style.color, m_style.variation);
}

IceBlock::IceBlock(double color_variation) : Block(BlockIds::ICE, with_variation(color_variation, Materials::with(Materials::frozen(2.2, 0.4), BlockProperties::see_through(true, false, LIGHT_OPACITY)).finished(SurfaceFinish::Glossy).weighing(BlockWeights::ICE))) {}

GlassBlock::GlassBlock(double color_variation) : Block(BlockIds::GLASS, with_variation(color_variation, Materials::with(Materials::thermal(1.0, 0.3), BlockProperties::see_through(true)).finished(SurfaceFinish::Glossy).weighing(BlockWeights::GLASS))) {}

WaterBlock::WaterBlock(double color_variation) : Block(BlockIds::WATER, with_variation(color_variation, BlockProperties::see_through(false, true, LIGHT_OPACITY).finished(SurfaceFinish::Liquid).weighing(BlockWeights::WATER))) {}

DefaultBlocks DefaultBlocks::register_all(BlockRegistry& registry, double color_variation) {
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

} // namespace voxelspire
