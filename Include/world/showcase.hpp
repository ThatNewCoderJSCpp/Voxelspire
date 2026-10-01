#ifndef VOXELSPIRE_WORLD_SHOWCASE_HPP
#define VOXELSPIRE_WORLD_SHOWCASE_HPP

#include <memory>
#include <vector>
#include "world_feature.hpp"

namespace voxelspire {

struct Showcase {
    static constexpr int ANCHOR_X = 4;
    static constexpr int ANCHOR_Y = 4;

    static std::vector<StructurePart> parts() {
        return {
            { "air",        {  0,  0,  1 }, { 40, 52, 12 } },

            { "stone",      {  4,  2,  1 }, {  5,  3,  3 } },
            { "lamp",       {  4,  2,  3 }, {  5,  3,  4 } },
            { "stone",      { 10,  2,  1 }, { 11,  3,  3 } },
            { "blue_lamp",  { 10,  2,  3 }, { 11,  3,  4 } },
            { "stone",      { 16,  2,  1 }, { 17,  3,  3 } },
            { "red_lamp",   { 16,  2,  3 }, { 17,  3,  4 } },
            { "stone",      { 22,  2,  1 }, { 23,  3,  3 } },
            { "green_lamp", { 22,  2,  3 }, { 23,  3,  4 } },

            { "stone",      {  3,  9, -5 }, { 17, 21,  1 } },
            { "water",      {  4, 10, -4 }, { 16, 20,  1 } },
            { "lamp",       {  9, 14, -5 }, { 10, 15, -4 } },
            { "blue_lamp",  { 14, 18, -5 }, { 15, 19, -4 } },

            { "glass",      { 21,  9,  1 }, { 29, 17,  6 } },
            { "air",        { 22, 10,  1 }, { 28, 16,  5 } },
            { "air",        { 24,  9,  1 }, { 26, 10,  3 } },
            { "lamp",       { 24, 13,  1 }, { 25, 14,  2 } },
            { "water",      { 26, 14,  1 }, { 28, 16,  2 } },

            { "glass",      { 31,  9,  1 }, { 38, 16,  4 } },
            { "water",      { 32, 10,  1 }, { 37, 15,  4 } },
            { "stone",      { 34,  6,  1 }, { 35,  7,  2 } },
            { "stone",      { 34,  7,  1 }, { 35,  8,  3 } },
            { "stone",      { 34,  8,  1 }, { 35,  9,  4 } },

            { "stone",      {  3, 25,  1 }, { 15, 35,  7 } },
            { "air",        {  4, 26,  1 }, { 14, 34,  6 } },
            { "air",        {  8, 25,  1 }, { 10, 26,  4 } },
            { "glass",      { 14, 28,  2 }, { 15, 31,  4 } },
            { "lamp",       {  8, 30,  5 }, {  9, 31,  6 } },
            { "blue_lamp",  {  4, 32,  2 }, {  5, 33,  3 } },
            { "red_lamp",   { 13, 27,  1 }, { 14, 28,  2 } },
            { "mirror_south", { 6, 33,  2 }, { 10, 34,  5 } },

            { "stone",      { 20, 24,  5 }, { 32, 36,  6 } },
            { "stone",      { 20, 24,  1 }, { 21, 25,  5 } },
            { "stone",      { 31, 24,  1 }, { 32, 25,  5 } },
            { "stone",      { 20, 35,  1 }, { 21, 36,  5 } },
            { "stone",      { 31, 35,  1 }, { 32, 36,  5 } },
            { "stone",      { 25, 29,  1 }, { 27, 31,  3 } },

            { "stone",      {  4, 40,  1 }, {  7, 43,  5 } },
            { "water",      {  5, 41,  5 }, {  6, 42,  6 } },

            { "stone",      { 12, 40,  1 }, { 20, 48,  4 } },
            { "stone",      { 12, 40,  4 }, { 20, 48,  8 } },
            { "water",      { 13, 41,  4 }, { 19, 47,  7 } },
            { "air",        { 15, 40,  4 }, { 17, 41,  7 } },

            { "stone",      { 24, 40,  1 }, { 36, 41,  2 } },
            { "stone",      { 24, 46,  1 }, { 36, 47,  2 } },
            { "stone",      { 24, 40,  1 }, { 25, 47,  4 } },
            { "water",      { 25, 41,  1 }, { 27, 46,  3 } },
            { "stone",      { 27, 41,  1 }, { 28, 46,  2 } },
        };
    }

    static std::shared_ptr<WorldFeature> feature(int anchor_x = ANCHOR_X, int anchor_y = ANCHOR_Y) {
        return std::make_shared<Structure>("voxelspire:showcase", anchor_x, anchor_y, parts());
    }
};

} // namespace voxelspire

#endif // VOXELSPIRE_WORLD_SHOWCASE_HPP