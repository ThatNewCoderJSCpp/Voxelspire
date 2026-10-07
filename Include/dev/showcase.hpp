#ifndef VOXELSPIRE_DEV_SHOWCASE_HPP
#define VOXELSPIRE_DEV_SHOWCASE_HPP

#include <memory>
#include <vector>
#include "../world/world_feature.hpp"

namespace voxelspire::dev {

struct Showcase {
    static constexpr int ANCHOR_X = 4;
    static constexpr int ANCHOR_Y = 4;
    inline static const Identifier ID = core_id(Kind::Feature, { "dev", "showcase" });

    static std::vector<StructurePart> parts() {
        namespace B = BlockIds;
        return {
            { B::AIR,    {  0,  0,  1 }, { 40, 52, 12 } },

            { B::STONE,  {  4,  2,  1 }, {  5,  3,  3 } },
            { B::STONE,  { 10,  2,  1 }, { 11,  3,  3 } },
            { B::STONE,  { 16,  2,  1 }, { 17,  3,  3 } },
            { B::STONE,  { 22,  2,  1 }, { 23,  3,  3 } },

            { B::STONE,  {  3,  9, -5 }, { 17, 21,  1 } },
            { B::WATER,  {  4, 10, -4 }, { 16, 20,  1 } },

            { B::GLASS,  { 21,  9,  1 }, { 29, 17,  6 } },
            { B::AIR,    { 22, 10,  1 }, { 28, 16,  5 } },
            { B::AIR,    { 24,  9,  1 }, { 26, 10,  3 } },
            { B::WATER,  { 26, 14,  1 }, { 28, 16,  2 } },

            { B::GLASS,  { 31,  9,  1 }, { 38, 16,  4 } },
            { B::WATER,  { 32, 10,  1 }, { 37, 15,  4 } },
            { B::STONE,  { 34,  6,  1 }, { 35,  7,  2 } },
            { B::STONE,  { 34,  7,  1 }, { 35,  8,  3 } },
            { B::STONE,  { 34,  8,  1 }, { 35,  9,  4 } },

            { B::STONE,  {  3, 25,  1 }, { 15, 35,  7 } },
            { B::AIR,    {  4, 26,  1 }, { 14, 34,  6 } },
            { B::AIR,    {  8, 25,  1 }, { 10, 26,  4 } },
            { B::GLASS,  { 14, 28,  2 }, { 15, 31,  4 } },

            { B::STONE,  { 20, 24,  5 }, { 32, 36,  6 } },
            { B::STONE,  { 20, 24,  1 }, { 21, 25,  5 } },
            { B::STONE,  { 31, 24,  1 }, { 32, 25,  5 } },
            { B::STONE,  { 20, 35,  1 }, { 21, 36,  5 } },
            { B::STONE,  { 31, 35,  1 }, { 32, 36,  5 } },
            { B::STONE,  { 25, 29,  1 }, { 27, 31,  3 } },

            { B::STONE,  {  4, 40,  1 }, {  7, 43,  5 } },
            { B::WATER,  {  5, 41,  5 }, {  6, 42,  6 } },

            { B::STONE,  { 12, 40,  1 }, { 20, 48,  4 } },
            { B::STONE,  { 12, 40,  4 }, { 20, 48,  8 } },
            { B::WATER,  { 13, 41,  4 }, { 19, 47,  7 } },
            { B::AIR,    { 15, 40,  4 }, { 17, 41,  7 } },

            { B::STONE,  { 24, 40,  1 }, { 36, 41,  2 } },
            { B::STONE,  { 24, 46,  1 }, { 36, 47,  2 } },
            { B::STONE,  { 24, 40,  1 }, { 25, 47,  4 } },
            { B::WATER,  { 25, 41,  1 }, { 27, 46,  3 } },
            { B::STONE,  { 27, 41,  1 }, { 28, 46,  2 } },
        };
    }

    static std::shared_ptr<WorldFeature> feature(int anchor_x = ANCHOR_X, int anchor_y = ANCHOR_Y) {
        return std::make_shared<Structure>(ID, anchor_x, anchor_y, parts());
    }
};

} // namespace voxelspire::dev

#endif // VOXELSPIRE_DEV_SHOWCASE_HPP