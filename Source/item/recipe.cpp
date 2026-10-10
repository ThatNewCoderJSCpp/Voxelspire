#include "item/recipe.hpp"

namespace voxelspire {

const Recipe& RecipeRegistry::add(Recipe recipe) {
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

const std::vector<const Recipe*>& RecipeRegistry::making(ItemHandle item) const {
    static const std::vector<const Recipe*> none;
    auto it = m_by_output.find(item);
    return it == m_by_output.end() ? none : it->second;
}

} // namespace voxelspire
