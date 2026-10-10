#ifndef VOXELSPIRE_ITEM_ITEM_HPP
#define VOXELSPIRE_ITEM_ITEM_HPP

#include <cctype>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include "../block/block_registry.hpp"
#include "../core/identifier.hpp"
#include "../core/settings.hpp"

namespace voxelspire {

using ItemHandle = std::uint32_t;
constexpr ItemHandle NO_ITEM = 0;

enum class ItemLook : std::uint8_t { Cube = 0, Flat };

struct ItemProperties {
    static constexpr int USE_DEFAULT = 0;

    std::string name;
    std::string description;
    std::string category;
    int         max_stack = USE_DEFAULT;
    double      weight    = ItemDefaults::weight;
    BlockId     block     = AIR_ID;
    ItemLook    look      = ItemLook::Flat;
    Color       color     { 200, 200, 200 };
    Color       side      { 160, 160, 160 };
    Color       front     { 130, 130, 130 };

    ItemProperties& named(std::string n) { name = std::move(n); return *this; }
    ItemProperties& described(std::string d) { description = std::move(d); return *this; }
    ItemProperties& in(std::string c) { category = std::move(c); return *this; }
    ItemProperties& stacks_to(int n) noexcept { max_stack = n; return *this; }
    ItemProperties& weighing(double kilograms) noexcept { weight = kilograms; return *this; }
    ItemProperties& colored(const Color& c) noexcept { color = c; side = c; front = c; return *this; }
};

class Item {
public:
    Item(Identifier id, ItemProperties props);

    virtual ~Item() = default;

    Item(const Item&) = delete;
    Item& operator=(const Item&) = delete;

    ItemHandle            handle()     const noexcept { return m_handle; }
    const Identifier&     id()         const noexcept { return m_id; }
    const ItemProperties& properties() const noexcept { return m_props; }
    const std::string&    name()       const noexcept { return m_props.name; }
    const std::string&    owner()      const { return m_id.owner(); }
    bool                  places_block() const noexcept { return m_props.block != AIR_ID; }

    static std::string pretty(const std::string& raw);

private:
    friend class ItemRegistry;

    Identifier     m_id;
    ItemProperties m_props;
    ItemHandle     m_handle = NO_ITEM;
};

class ItemRegistry {
public:
    static constexpr const char* BLOCKS = "blocks";

    ItemRegistry() { m_items.emplace_back(); }
    ItemRegistry(const ItemRegistry&) = delete;
    ItemRegistry& operator=(const ItemRegistry&) = delete;

    template <typename T = Item, typename... Args>
    const Item& add(Args&&... args) {
        if (m_locked) throw std::runtime_error("items must be registered before the world starts");
        auto item = std::make_unique<T>(std::forward<Args>(args)...);
        if (m_by_id.contains(item->id())) throw std::runtime_error("item already registered: " + item->id().str());
        const ItemHandle h = static_cast<ItemHandle>(m_items.size());
        item->m_handle = h;
        m_by_id.emplace(item->id(), h);
        const BlockId block = item->properties().block;

        if (block != AIR_ID) {
            if (m_by_block.size() <= block) m_by_block.resize(static_cast<std::size_t>(block) + 1, NO_ITEM);
            m_by_block[block] = h;
        }

        m_items.push_back(std::move(item));
        ++m_version;
        return *m_items.back();
    }

    const Item& block_item(const BlockRegistry& blocks, BlockId block, std::string category);

    const Item* get(ItemHandle h) const noexcept { return h != NO_ITEM && h < m_items.size() ? m_items[h].get() : nullptr; }

    ItemHandle find(const Identifier& id) const noexcept {
        const ItemHandle* h = m_by_id.find(id);
        return h ? *h : NO_ITEM;
    }

    ItemHandle find(std::string_view text) const {
        const Identifier id = Identifier::find(Kind::Item, text);
        return id ? find(id) : NO_ITEM;
    }

    ItemHandle for_block(BlockId block) const noexcept { return block < m_by_block.size() ? m_by_block[block] : NO_ITEM; }

    template <typename Fn>
    void for_each(Fn&& fn) const {
        for (std::size_t i = 1; i < m_items.size(); ++i) fn(*m_items[i]);
    }

    double weight(ItemHandle h) const noexcept {
        const Item* i = get(h);
        return i ? i->properties().weight : 0.0;
    }

    std::size_t   size()    const noexcept { return m_items.size() - 1; }
    std::uint64_t version() const noexcept { return m_version; }
    void          lock()          noexcept { m_locked = true; }

private:
    std::vector<std::unique_ptr<Item>> m_items;
    IdentifierTable<ItemHandle>        m_by_id;
    std::vector<ItemHandle>            m_by_block;
    std::uint64_t                      m_version = 0;
    bool                               m_locked  = false;
};

} // namespace voxelspire

#endif // VOXELSPIRE_ITEM_ITEM_HPP