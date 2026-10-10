#include "item/options.hpp"

namespace voxelspire {

bool ItemOptionRegistry::remove(const Identifier& id) {
    for (auto it = m_options.begin(); it != m_options.end(); ++it) {
        if (it->id != id) continue;
        m_options.erase(it);
        return true;
    }

    return false;
}

std::vector<const ItemOption*> ItemOptionRegistry::for_item(const ItemContext& ctx) const {
    std::vector<const ItemOption*> out;
    for (const ItemOption& o : m_options) if (!o.applies || o.applies(ctx)) out.push_back(&o);
    return out;
}

void ItemOptionRegistry::register_defaults(ItemOptionRegistry& r) {
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

} // namespace voxelspire
