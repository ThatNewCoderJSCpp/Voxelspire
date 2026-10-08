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

    ItemSource& add_source(const std::string& owner, const std::string& name, int order) {
        if (ItemSource* s = find_source(owner)) {
            s->name  = name;
            s->order = order;
            return *s;
        }

        m_sources.push_back({ owner, name, order, {} });
        sort_sources();
        ++m_version;
        return *find_source(owner);
    }

    ItemCategory& add_category(const std::string& owner, const std::string& key, const std::string& name, int order) {
        ItemSource& s = find_source(owner) ? *find_source(owner) : add_source(owner, Item::pretty(owner), static_cast<int>(m_sources.size()));
        ++m_version;

        for (ItemCategory& c : s.categories) {
            if (c.key != key) continue;
            c.name  = name;
            c.order = order;
            sort_categories(s);
            return *find_category(s, key);
        }

        s.categories.push_back({ key, name, order });
        sort_categories(s);
        return *find_category(s, key);
    }

    ItemPlace place(const Item& item) {
        ItemSource* s = find_source(item.owner());
        if (!s) s = find_source(OTHER);
        const std::string& key = item.properties().category;
        const ItemCategory* c = key.empty() ? nullptr : s->find(key);
        if (!c) c = s->find(UNSORTED);
        if (!c) c = &add_category(s->owner, UNSORTED, UNSORTED_NAME, LAST);
        return { find_source(s->owner), c };
    }

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

    void sort_sources() {
        std::stable_sort(m_sources.begin(), m_sources.end(), [](const ItemSource& a, const ItemSource& b) { return a.order < b.order; });
    }

    static void sort_categories(ItemSource& s) {
        std::stable_sort(s.categories.begin(), s.categories.end(), [](const ItemCategory& a, const ItemCategory& b) { return a.order < b.order; });
    }

    std::vector<ItemSource> m_sources;
    std::uint64_t           m_version = 0;
};

} // namespace voxelspire

#endif // VOXELSPIRE_ITEM_ITEM_CATEGORY_HPP