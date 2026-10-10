#include "item/category.hpp"

namespace voxelspire {

ItemSource& ItemCategoryRegistry::add_source(const std::string& owner, const std::string& name, int order) {
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

ItemCategory& ItemCategoryRegistry::add_category(const std::string& owner, const std::string& key, const std::string& name, int order) {
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

ItemPlace ItemCategoryRegistry::place(const Item& item) {
    ItemSource* s = find_source(item.owner());
    if (!s) s = find_source(OTHER);
    const std::string& key = item.properties().category;
    const ItemCategory* c = key.empty() ? nullptr : s->find(key);
    if (!c) c = s->find(UNSORTED);
    if (!c) c = &add_category(s->owner, UNSORTED, UNSORTED_NAME, LAST);
    return { find_source(s->owner), c };
}

void ItemCategoryRegistry::sort_sources() {
    std::stable_sort(m_sources.begin(), m_sources.end(), [](const ItemSource& a, const ItemSource& b) { return a.order < b.order; });
}

void ItemCategoryRegistry::sort_categories(ItemSource& s) {
    std::stable_sort(s.categories.begin(), s.categories.end(), [](const ItemCategory& a, const ItemCategory& b) { return a.order < b.order; });
}

} // namespace voxelspire
