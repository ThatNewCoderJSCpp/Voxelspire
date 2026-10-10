#ifndef VOXELSPIRE_ITEM_ITEM_STATS_HPP
#define VOXELSPIRE_ITEM_ITEM_STATS_HPP

#include <cstdio>
#include <functional>
#include <string>
#include <utility>
#include <vector>
#include "category.hpp"
#include "stack.hpp"
#include "recipe.hpp"

namespace voxelspire {

struct ItemStat {
    std::string label;
    std::string value;
};

struct ItemStatContext {
    const ItemRegistry*         items      = nullptr;
    const BlockRegistry*        blocks     = nullptr;
    const StackRules*           rules      = nullptr;
    const RecipeRegistry*       recipes    = nullptr;
    const ItemCategoryRegistry* categories = nullptr;
};

class ItemStatRegistry {
public:
    using Source = std::function<void(const Item&, const ItemStatContext&, std::vector<ItemStat>&)>;

    void add(Identifier id, Source source) {
        remove(id);
        m_sources.emplace_back(std::move(id), std::move(source));
    }

    bool remove(const Identifier& id);

    std::vector<ItemStat> collect(const Item& item, const ItemStatContext& ctx) const {
        std::vector<ItemStat> out;
        for (const auto& s : m_sources) s.second(item, ctx, out);
        return out;
    }

    static std::string number(double v, const char* unit = "") {
        char buf[BUFFER];
        std::snprintf(buf, sizeof(buf), "%.2f%s", v, unit);
        return buf;
    }

    static std::string recipe_text(const Recipe& r, const ItemRegistry& items);

    static void register_defaults(ItemStatRegistry& r);

private:
    static constexpr std::size_t BUFFER = 64;

    std::vector<std::pair<Identifier, Source>> m_sources;
};

} // namespace voxelspire

#endif // VOXELSPIRE_ITEM_ITEM_STATS_HPP