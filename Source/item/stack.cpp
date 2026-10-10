#include "item/stack.hpp"

namespace voxelspire {

int StackRules::limit(ItemHandle item) const {
    auto it = m_overrides.find(item);
    if (it != m_overrides.end()) return it->second;
    const Item* i = m_items->get(item);
    const int own = i ? i->properties().max_stack : ItemProperties::USE_DEFAULT;
    if (own != ItemProperties::USE_DEFAULT) return clamp(own);
    return clamp(m_rules ? m_rules->default_stack : ItemDefaults::stack);
}

int StackRules::clamp(int size) noexcept {
    return static_cast<int>(vclamp(static_cast<double>(size), ItemLimits::stack.min, ItemLimits::stack.max));
}

} // namespace voxelspire
