#ifndef VOXELSPIRE_ITEM_ITEM_CATEGORY_HPP
#define VOXELSPIRE_ITEM_ITEM_CATEGORY_HPP

#include <algorithm>
#include <string>
#include <utility>
#include <vector>
#include "item.hpp"

namespace voxelspire {

struct ItemCategory {
    std::string key;
    std::string name;
    int         order = 0;
};

struct ItemSource {
    std::string               owner;
    std::string               name;
    int                       order = 0;
    std::vector<ItemCategory> categories;

    const ItemCategory* find(const std::string& key) const noexcept {
        for (const ItemCategory& c : categories) if (c.key == key) return &c;
        return nullptr;
    }
};

struct ItemPlace {
    const ItemSource*   source   = nullptr;
    const ItemCategory* category = nullptr;
};

class ItemCategoryRegistry {
public:
    static constexpr const char* OTHER         = "other";
    static constexpr const char* OTHER_NAME    = "Other";
    static constexpr const char* UNSORTED      = "uncategorized";
    static constexpr const char* UNSORTED_NAME = "Uncategorized";
    static constexpr int         LAST          = 1 << 20;

    ItemCategoryRegistry() { add_source(OTHER, OTHER_NAME, LAST); }

    ItemSource& add_source(const std::string& owner, const std::string& name, int order);

    ItemCategory& add_category(const std::string& owner, const std::string& key, const std::string& name, int order);

    ItemPlace place(const Item& item);

    const std::vector<ItemSource>& sources() const noexcept { return m_sources; }
    std::uint64_t                  version() const noexcept { return m_version; }

    ItemSource* find_source(const std::string& owner) noexcept {
        for (ItemSource& s : m_sources) if (s.owner == owner) return &s;
        return nullptr;
    }

private:
    static ItemCategory* find_category(ItemSource& s, const std::string& key) noexcept {
        for (ItemCategory& c : s.categories) if (c.key == key) return &c;
        return nullptr;
    }

    void sort_sources();

    static void sort_categories(ItemSource& s);

    std::vector<ItemSource> m_sources;
    std::uint64_t           m_version = 0;
};

} // namespace voxelspire

#endif // VOXELSPIRE_ITEM_ITEM_CATEGORY_HPP