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
    const Recipe& add(Recipe recipe) {
        if (!recipe.id) throw std::runtime_error("recipes need an id");
        if (recipe.output.empty()) throw std::runtime_error("recipe has no output: " + recipe.id.str());
        if (recipe.shape == RecipeShape::Shaped && static_cast<int>(recipe.inputs.size()) != recipe.width * recipe.height)
            throw std::runtime_error("shaped recipe grid does not match its size: " + recipe.id.str());
        m_recipes.push_back(std::make_unique<Recipe>(std::move(recipe)));
        const Recipe& r = *m_recipes.back();
        m_by_output[r.output.item].push_back(&r);
        ++m_version;
        return r;
    }

    const std::vector<const Recipe*>& making(ItemHandle item) const {
        static const std::vector<const Recipe*> none;
        auto it = m_by_output.find(item);
        return it == m_by_output.end() ? none : it->second;
    }

    std::size_t   size()    const noexcept { return m_recipes.size(); }
    std::uint64_t version() const noexcept { return m_version; }

private:
    std::vector<std::unique_ptr<Recipe>>                         m_recipes;
    std::unordered_map<ItemHandle, std::vector<const Recipe*>>   m_by_output;
    std::uint64_t                                                m_version = 0;
};

} // namespace voxelspire

#endif // VOXELSPIRE_ITEM_RECIPE_HPP