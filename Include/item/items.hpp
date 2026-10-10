#ifndef VOXELSPIRE_ITEM_VOXELSPIRE_ITEMS_HPP
#define VOXELSPIRE_ITEM_VOXELSPIRE_ITEMS_HPP

#include <string>
#include <unordered_map>
#include "../block/blocks.hpp"
#include "category.hpp"
#include "recipe.hpp"

namespace voxelspire {

namespace ItemCategories {
    inline constexpr const char* NATURAL  = "natural_blocks";
    inline constexpr const char* BUILDING = "building_blocks";
    inline constexpr const char* LIQUIDS  = "liquids";
    inline constexpr const char* LIGHTS   = "light_sources";
    inline constexpr const char* TOOLS    = "tools";
    inline constexpr const char* WEAPONS  = "weapons";
    inline constexpr const char* ARMOR    = "armor";
    inline constexpr const char* CLOTHES  = "clothes";
    inline constexpr const char* FOOD     = "food";
    inline constexpr const char* MATERIAL = "materials";
} // namespace ItemCategories

struct VoxelspireItems {
    static constexpr const char* NAME = "Voxelspire";
    static constexpr int         FIRST = 0;

    static void register_categories(ItemCategoryRegistry& c);

    static void register_items(ItemRegistry& items, const BlockRegistry& blocks, const DefaultBlocks& ids);

    static void block_items(ItemRegistry& items, const BlockRegistry& blocks, const std::unordered_map<BlockId, const char*>& place);

    static void register_recipes(RecipeRegistry&, const ItemRegistry&) {}
};

} // namespace voxelspire

#endif // VOXELSPIRE_ITEM_VOXELSPIRE_ITEMS_HPP