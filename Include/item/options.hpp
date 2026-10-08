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

    bool remove(const Identifier& id) {
        for (auto it = m_options.begin(); it != m_options.end(); ++it) {
            if (it->id != id) continue;
            m_options.erase(it);
            return true;
        }

        return false;
    }

    std::vector<const ItemOption*> for_item(const ItemContext& ctx) const {
        std::vector<const ItemOption*> out;
        for (const ItemOption& o : m_options) if (!o.applies || o.applies(ctx)) out.push_back(&o);
        return out;
    }

    static void register_defaults(ItemOptionRegistry& r) {
        ItemOption stack;
        stack.id          = ItemOptions::StackSize;
        stack.label       = "Stack size";
        stack.description = "How many of this item fit in one slot, for every stack of it in this world.";
        stack.limits      = ItemLimits::stack;
        stack.get         = [](const ItemContext& c) { return c.rules->limit(c.item); };
        stack.set         = [](const ItemContext& c, int v) {
            c.rules->set(c.item, v);
            if (c.inventory) c.inventory->spread(c.item, *c.rules);
        };
        stack.changed     = [](const ItemContext& c) { return c.rules->overridden(c.item); };
        stack.reset       = [](const ItemContext& c) {
            c.rules->reset(c.item);
            if (c.inventory) c.inventory->spread(c.item, *c.rules);
        };
        r.add(std::move(stack));
    }

private:
    std::vector<ItemOption> m_options;
};

} // namespace voxelspire

#endif // VOXELSPIRE_ITEM_ITEM_OPTIONS_HPP