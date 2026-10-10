#ifndef VOXELSPIRE_ITEM_RECIPE_HPP
#define VOXELSPIRE_ITEM_RECIPE_HPP

#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include "stack.hpp"

namespace voxelspire {

enum class RecipeShape : std::uint8_t { Shaped = 0, Shapeless };

struct RecipeIngredient {
    ItemHandle item  = NO_ITEM;
    int        count = 1;
};

struct Recipe {
    Identifier                    id;
    std::string                   station = "Crafting";
    RecipeShape                   shape   = RecipeShape::Shapeless;
    int                           width   = 0;
    int                           height  = 0;
    std::vector<RecipeIngredient> inputs;
    ItemStack                     output;
};

class RecipeRegistry {
public:
    const Recipe& add(Recipe recipe);

    const std::vector<const Recipe*>& making(ItemHandle item) const;

    std::size_t   size()    const noexcept { return m_recipes.size(); }
    std::uint64_t version() const noexcept { return m_version; }

private:
    std::vector<std::unique_ptr<Recipe>>                         m_recipes;
    std::unordered_map<ItemHandle, std::vector<const Recipe*>>   m_by_output;
    std::uint64_t                                                m_version = 0;
};

} // namespace voxelspire

#endif // VOXELSPIRE_ITEM_RECIPE_HPP