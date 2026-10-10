#include "item/items.hpp"

namespace voxelspire {

void VoxelspireItems::register_categories(ItemCategoryRegistry& c) {
    const std::string owner = CORE_OWNER;
    c.add_source(owner, NAME, FIRST);
    int order = 0;
    c.add_category(owner, ItemCategories::NATURAL,  "Natural blocks",  order++);
    c.add_category(owner, ItemCategories::BUILDING, "Building blocks", order++);
    c.add_category(owner, ItemCategories::LIQUIDS,  "Liquids",         order++);
    c.add_category(owner, ItemCategories::LIGHTS,   "Light sources",   order++);
    c.add_category(owner, ItemCategories::TOOLS,    "Tools",           order++);
    c.add_category(owner, ItemCategories::WEAPONS,  "Weapons",         order++);
    c.add_category(owner, ItemCategories::ARMOR,    "Armor",           order++);
    c.add_category(owner, ItemCategories::CLOTHES,  "Clothes",         order++);
    c.add_category(owner, ItemCategories::FOOD,     "Food",            order++);
    c.add_category(owner, ItemCategories::MATERIAL, "Materials",       order++);
}

void VoxelspireItems::register_items(ItemRegistry& items, const BlockRegistry& blocks, const DefaultBlocks& ids) {
    const std::unordered_map<BlockId, const char*> place{
        { ids.grass,     ItemCategories::NATURAL },
        { ids.dirt,      ItemCategories::NATURAL },
        { ids.stone,     ItemCategories::NATURAL },
        { ids.bedrock,   ItemCategories::NATURAL },
        { ids.sand,      ItemCategories::NATURAL },
        { ids.red_sand,  ItemCategories::NATURAL },
        { ids.gravel,    ItemCategories::NATURAL },
        { ids.clay,      ItemCategories::NATURAL },
        { ids.snow,      ItemCategories::NATURAL },
        { ids.mud,       ItemCategories::NATURAL },
        { ids.ice,       ItemCategories::NATURAL },
        { ids.sandstone, ItemCategories::NATURAL },
        { ids.water,     ItemCategories::LIQUIDS },
        { ids.glass,     ItemCategories::BUILDING },
    };

    block_items(items, blocks, place);
}

void VoxelspireItems::block_items(ItemRegistry& items, const BlockRegistry& blocks, const std::unordered_map<BlockId, const char*>& place) {
    for (std::size_t i = 0; i < blocks.size(); ++i) {
        const BlockId id = static_cast<BlockId>(i);
        if (id == AIR_ID || blocks.traits(id).fluid || items.for_block(id) != NO_ITEM) continue;
        auto it = place.find(id);
        items.block_item(blocks, id, it == place.end() ? std::string() : std::string(it->second));
    }
}

} // namespace voxelspire
