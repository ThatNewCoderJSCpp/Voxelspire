#ifndef VOXELSPIRE_ITEM_ITEM_SYSTEM_HPP
#define VOXELSPIRE_ITEM_ITEM_SYSTEM_HPP

#include <vector>
#include "options.hpp"
#include "stats.hpp"
#include "items.hpp"

namespace voxelspire {

class ItemSystem {
public:
    ItemSystem(const BlockRegistry& blocks, const DefaultBlocks& ids) : m_blocks(&blocks) {
        VoxelspireItems::register_categories(m_categories);
        VoxelspireItems::register_items(m_items, blocks, ids);
        VoxelspireItems::register_recipes(m_recipes, m_items);
        ItemOptionRegistry::register_defaults(m_options);
        ItemStatRegistry::register_defaults(m_stats);
    }

    ItemSystem(const ItemSystem&) = delete;
    ItemSystem& operator=(const ItemSystem&) = delete;

    ItemRegistry&               items()            noexcept { return m_items; }
    const ItemRegistry&         items()      const noexcept { return m_items; }
    ItemCategoryRegistry&       categories()       noexcept { return m_categories; }
    const ItemCategoryRegistry& categories() const noexcept { return m_categories; }
    RecipeRegistry&             recipes()          noexcept { return m_recipes; }
    const RecipeRegistry&       recipes()    const noexcept { return m_recipes; }
    ItemOptionRegistry&         options()          noexcept { return m_options; }
    const ItemOptionRegistry&   options()    const noexcept { return m_options; }
    ItemStatRegistry&           stats()            noexcept { return m_stats; }
    const ItemStatRegistry&     stats()      const noexcept { return m_stats; }
    const BlockRegistry&        blocks()     const noexcept { return *m_blocks; }

    void lock() noexcept { m_items.lock(); }

    ItemStatContext stat_context(const StackRules* rules) const noexcept {
        ItemStatContext c;
        c.items      = &m_items;
        c.blocks     = m_blocks;
        c.rules      = rules;
        c.recipes    = &m_recipes;
        c.categories = &m_categories;
        return c;
    }

    const ItemPlace& place(ItemHandle h) {
        refresh_places();
        static const ItemPlace none;
        return h < m_places.size() ? m_places[h] : none;
    }

private:
    void refresh_places() {
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

    static constexpr std::uint64_t VERSION_MIX = 1000003;

    const BlockRegistry*   m_blocks;
    ItemRegistry           m_items;
    ItemCategoryRegistry   m_categories;
    RecipeRegistry         m_recipes;
    ItemOptionRegistry     m_options;
    ItemStatRegistry       m_stats;
    std::vector<ItemPlace> m_places;
    std::uint64_t          m_places_version = ~std::uint64_t(0);
};

} // namespace voxelspire

#endif // VOXELSPIRE_ITEM_ITEM_SYSTEM_HPP