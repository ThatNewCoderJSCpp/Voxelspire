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

    Color face_color(Face face, const BlockPos& pos) const override {
        if (face == Face::Down) return vary(Color(134, 96, 67), pos, 6);
        if (face == Face::Up)   return vary(Color(96, 170, 62), pos, 7);
        return vary(Color(84, 150, 55), pos, 5);
    }
};

class DirtBlock final : public Block {
public:
    explicit DirtBlock(double color_variation = 1.0) : Block("dirt", with_variation(color_variation)) {}
    Color face_color(Face, const BlockPos& pos) const override { return vary(Color(134, 96, 67), pos, 6); }
};

class StoneBlock final : public Block {
public:
    explicit StoneBlock(double color_variation = 1.0) : Block("stone", with_variation(color_variation)) {}
    Color face_color(Face, const BlockPos& pos) const override { return vary(Color(126, 126, 126), pos, 8); }
};

class BedrockBlock final : public Block {
public:
    explicit BedrockBlock(double color_variation = 1.0) : Block("bedrock", with_variation(color_variation)) {}
    Color face_color(Face, const BlockPos& pos) const override { return vary(Color(58, 58, 58), pos, 10); }
};

struct DefaultBlocks {
    BlockId air = AIR_ID;
    BlockId grass = AIR_ID;
    BlockId dirt = AIR_ID;
    BlockId stone = AIR_ID;
    BlockId bedrock = AIR_ID;

    static DefaultBlocks register_all(BlockRegistry& registry, double color_variation = 1.0) {
        DefaultBlocks ids;
        ids.grass   = registry.add<GrassBlock>(color_variation);
        ids.dirt    = registry.add<DirtBlock>(color_variation);
        ids.stone   = registry.add<StoneBlock>(color_variation);
        ids.bedrock = registry.add<BedrockBlock>(color_variation);
        return ids;
    }
};

} // namespace voxelspire

#endif // VOXELSPIRE_BLOCK_BLOCKS_HPP