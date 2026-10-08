#ifndef VOXELSPIRE_ITEM_ITEM_STACK_HPP
#define VOXELSPIRE_ITEM_ITEM_STACK_HPP

#include <cstdint>
#include <unordered_map>
#include "item.hpp"

namespace voxelspire {

struct ItemStack {
    ItemHandle item  = NO_ITEM;
    int        count = 0;

    bool empty() const noexcept { return item == NO_ITEM || count <= 0; }
    void clear() noexcept { item = NO_ITEM; count = 0; }
    bool same(const ItemStack& o) const noexcept { return item == o.item; }

    bool operator==(const ItemStack& o) const noexcept { return (empty() && o.empty()) || (item == o.item && count == o.count); }
    bool operator!=(const ItemStack& o) const noexcept { return !(*this == o); }

    static ItemStack of(ItemHandle item, int count) noexcept { return { item, count }; }
};

class StackRules {
public:
    explicit StackRules(const ItemRegistry& items, const ItemRules* rules = nullptr) : m_items(&items), m_rules(rules) {}

    int limit(ItemHandle item) const {
        auto it = m_overrides.find(item);
        if (it != m_overrides.end()) return it->second;
        const Item* i = m_items->get(item);
        const int own = i ? i->properties().max_stack : ItemProperties::USE_DEFAULT;
        if (own != ItemProperties::USE_DEFAULT) return clamp(own);
        return clamp(m_rules ? m_rules->default_stack : ItemDefaults::stack);
    }

    bool overridden(ItemHandle item) const { return m_overrides.count(item) != 0; }

    void set(ItemHandle item, int size) {
        m_overrides[item] = clamp(size);
        ++m_version;
    }

    void reset(ItemHandle item) {
        if (m_overrides.erase(item)) ++m_version;
    }

    void clear() noexcept { m_overrides.clear(); ++m_version; }

    const std::unordered_map<ItemHandle, int>& overrides() const noexcept { return m_overrides; }
    std::uint64_t version() const noexcept { return m_version; }

    static int clamp(int size) noexcept {
        return static_cast<int>(vclamp(static_cast<double>(size), ItemLimits::stack.min, ItemLimits::stack.max));
    }

private:
    const ItemRegistry*                 m_items;
    const ItemRules*                    m_rules;
    std::unordered_map<ItemHandle, int> m_overrides;
    std::uint64_t                       m_version = 0;
};

} // namespace voxelspire

#endif // VOXELSPIRE_ITEM_ITEM_STACK_HPP