#ifndef VOXELSPIRE_ITEM_INVENTORY_HPP
#define VOXELSPIRE_ITEM_INVENTORY_HPP

#include <array>
#include <cstdint>
#include "stack.hpp"

namespace voxelspire {

class Inventory {
public:
    static constexpr int HOTBAR  = 10;
    static constexpr int COLUMNS = 12;
    static constexpr int ROWS    = 10;
    static constexpr int MAIN    = COLUMNS * ROWS;
    static constexpr int SLOTS   = HOTBAR + MAIN;
    static constexpr int NONE    = -1;

    static constexpr bool in_hotbar(int slot) noexcept { return slot >= 0 && slot < HOTBAR; }
    static constexpr bool valid(int slot) noexcept { return slot >= 0 && slot < SLOTS; }
    static constexpr int  main_slot(int column, int row) noexcept { return HOTBAR + row * COLUMNS + column; }

    const ItemStack& at(int slot) const noexcept;

    void set(int slot, const ItemStack& stack) noexcept;

    int  selected() const noexcept { return m_selected; }
    void select(int slot) noexcept { if (in_hotbar(slot) && slot != m_selected) { m_selected = slot; ++m_revision; } }

    void scroll(int steps, bool wraps) noexcept;

    const ItemStack& held() const noexcept { return at(m_selected); }

    int add(ItemStack stack, const StackRules& rules);

    int count(ItemHandle item) const noexcept {
        int n = 0;
        for (const ItemStack& s : m_slots) if (s.item == item) n += s.count;
        return n;
    }

    int spread(ItemHandle item, const StackRules& rules);

    void clear() noexcept {
        for (ItemStack& s : m_slots) s.clear();
        m_selected = 0;
        ++m_revision;
    }

    std::uint64_t revision() const noexcept { return m_revision; }

private:
    std::array<ItemStack, SLOTS> m_slots{};
    int                          m_selected = 0;
    std::uint64_t                m_revision = 0;
};

class CursorStack {
public:
    const ItemStack& stack() const noexcept { return m_stack; }
    bool empty() const noexcept { return m_stack.empty(); }
    void clear() noexcept { m_stack.clear(); }

    void click(Inventory& inv, int slot, const StackRules& rules);

    void place_one(Inventory& inv, int slot, const StackRules& rules);

    void take_half(Inventory& inv, int slot);

    void quick_move(Inventory& inv, int slot, const StackRules& rules);

    void put_back(Inventory& inv, const StackRules& rules);

    void set(const ItemStack& s) noexcept { m_stack = s; }

private:
    ItemStack m_stack;
};

} // namespace voxelspire

#endif // VOXELSPIRE_ITEM_INVENTORY_HPP