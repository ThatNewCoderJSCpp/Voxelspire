#include "item/system.hpp"

namespace voxelspire {

ItemSystem::ItemSystem(const BlockRegistry& blocks, const DefaultBlocks& ids) : m_blocks(&blocks) {
    VoxelspireItems::register_categories(m_categories);
    VoxelspireItems::register_items(m_items, blocks, ids);
    VoxelspireItems::register_recipes(m_recipes, m_items);
    ItemOptionRegistry::register_defaults(m_options);
    ItemStatRegistry::register_defaults(m_stats);
}

ItemStatContext ItemSystem::stat_context(const StackRules* rules) const noexcept {
    ItemStatContext c;
    c.items      = &m_items;
    c.blocks     = m_blocks;
    c.rules      = rules;
    c.recipes    = &m_recipes;
    c.categories = &m_categories;
    return c;
}

void ItemSystem::refresh_places() {
    const std::uint64_t version = m_items.version() * VERSION_MIX + m_categories.version();
    if (version == m_places_version && !m_places.empty()) return;
    std::uint64_t before = 0;

    do {
        before = m_categories.version();
        m_places.assign(m_items.size() + 1, ItemPlace{});
        m_items.for_each([this](const Item& item) { m_places[item.handle()] = m_categories.place(item); });
    } while (m_categories.version() != before);

    m_places_version = m_items.version() * VERSION_MIX + m_categories.version();
}

} // namespace voxelspire
