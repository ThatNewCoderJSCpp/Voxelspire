#ifndef VOXELSPIRE_BLOCK_BLOCKS_HPP
#define VOXELSPIRE_BLOCK_BLOCKS_HPP

#include "block_registry.hpp"

namespace voxelspire {

class GrassBlock final : public Block {
public:
    GrassBlock() : Block("grass", BlockProperties{}) {}

    Color face_color(Face face, const BlockPos& pos) const override {
        if (face == Face::Down) return vary(Color(134, 96, 67), pos, 6);
        if (face == Face::Up)   return vary(Color(96, 170, 62), pos, 7);
        return vary(Color(84, 150, 55), pos, 5);
    }
};

class DirtBlock final : public Block {
public:
    DirtBlock() : Block("dirt", BlockProperties{}) {}
    Color face_color(Face, const BlockPos& pos) const override { return vary(Color(134, 96, 67), pos, 6); }
};

class StoneBlock final : public Block {
public:
    StoneBlock() : Block("stone", BlockProperties{}) {}
    Color face_color(Face, const BlockPos& pos) const override { return vary(Color(126, 126, 126), pos, 8); }
};

class BedrockBlock final : public Block {
public:
    BedrockBlock() : Block("bedrock", BlockProperties{}) {}
    Color face_color(Face, const BlockPos& pos) const override { return vary(Color(58, 58, 58), pos, 10); }
};

struct DefaultBlocks {
    BlockId air = AIR_ID;
    BlockId grass = AIR_ID;
    BlockId dirt = AIR_ID;
    BlockId stone = AIR_ID;
    BlockId bedrock = AIR_ID;

    static DefaultBlocks register_all(BlockRegistry& registry) {
        DefaultBlocks ids;
        ids.grass   = registry.add<GrassBlock>();
        ids.dirt    = registry.add<DirtBlock>();
        ids.stone   = registry.add<StoneBlock>();
        ids.bedrock = registry.add<BedrockBlock>();
        return ids;
    }
};

} // namespace voxelspire

#endif // VOXELSPIRE_BLOCK_BLOCKS_HPP