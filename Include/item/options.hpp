#ifndef VOXELSPIRE_ITEM_ITEM_OPTIONS_HPP
#define VOXELSPIRE_ITEM_ITEM_OPTIONS_HPP

#include <functional>
#include <string>
#include <utility>
#include <vector>
#include "inventory.hpp"

namespace voxelspire {

struct ItemContext {
    ItemHandle          item  = NO_ITEM;
    int                 slot  = Inventory::NONE;
    Inventory*          inventory = nullptr;
    StackRules*         rules = nullptr;
    const ItemRegistry* items = nullptr;
};

enum class ItemOptionKind : std::uint8_t { Number = 0, Action };

struct ItemOption {
    Identifier                              id;
    std::string                             label;
    std::string                             description;
    ItemOptionKind                          kind = ItemOptionKind::Number;
    Bounds                                  limits{ 0.0, 1.0 };
    std::function<bool(const ItemContext&)> applies;
    std::function<int(const ItemContext&)>  get;
    std::function<void(const ItemContext&, int)> set;
    std::function<void(const ItemContext&)> run;
    std::function<bool(const ItemContext&)> changed;
    std::function<void(const ItemContext&)> reset;
};

namespace ItemOptions {
    inline const Identifier StackSize = core_id(Kind::Item, { "option", "stack_size" });
} // namespace ItemOptions

class ItemOptionRegistry {
public:
    void add(ItemOption option) {
        remove(option.id);
        m_options.push_back(std::move(option));
    }

    bool remove(const Identifier& id);

    std::vector<const ItemOption*> for_item(const ItemContext& ctx) const;

    static void register_defaults(ItemOptionRegistry& r);

private:
    std::vector<ItemOption> m_options;
};

} // namespace voxelspire

#endif // VOXELSPIRE_ITEM_ITEM_OPTIONS_HPP