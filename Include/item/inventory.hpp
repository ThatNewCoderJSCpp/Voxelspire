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

    const ItemStack& at(int slot) const noexcept { static const ItemStack none; return valid(slot) ? m_slots[static_cast<std::size_t>(slot)] : none; }

    void set(int slot, const ItemStack& stack) noexcept {
        if (!valid(slot)) return;
        ItemStack& s = m_slots[static_cast<std::size_t>(slot)];
        s = stack.empty() ? ItemStack{} : stack;
        ++m_revision;
    }

    int  selected() const noexcept { return m_selected; }
    void select(int slot) noexcept { if (in_hotbar(slot) && slot != m_selected) { m_selected = slot; ++m_revision; } }

    void scroll(int steps, bool wraps) noexcept {
        if (steps == 0) return;
        int next = m_selected + steps;
        next = wraps ? ((next % HOTBAR) + HOTBAR) % HOTBAR : static_cast<int>(vclamp(next, 0, HOTBAR - 1));
        select(next);
    }

    const ItemStack& held() const noexcept { return at(m_selected); }

    int add(ItemStack stack, const StackRules& rules) {
        if (stack.empty()) return 0;
        const int limit = rules.limit(stack.item);

        for (int pass = 0; pass < 2 && stack.count > 0; ++pass)
            for (int slot = 0; slot < SLOTS && stack.count > 0; ++slot) {
                ItemStack& s = m_slots[static_cast<std::size_t>(slot)];
                const bool fill = pass == 0 ? (!s.empty() && s.item == stack.item) : s.empty();
                if (!fill) continue;
                if (s.empty()) s = ItemStack{ stack.item, 0 };
                const int moved = vmin(limit - s.count, stack.count);
                if (moved <= 0) continue;
                s.count += moved;
                stack.count -= moved;
                ++m_revision;
            }

        return stack.count;
    }

    int count(ItemHandle item) const noexcept {
        int n = 0;
        for (const ItemStack& s : m_slots) if (s.item == item) n += s.count;
        return n;
    }

    int spread(ItemHandle item, const StackRules& rules) {
        const int limit = rules.limit(item);
        int extra = 0;

        for (ItemStack& s : m_slots) {
            if (s.item != item || s.count <= limit) continue;
            extra += s.count - limit;
            s.count = limit;
            ++m_revision;
        }

        if (extra == 0) return 0;
        const int left = add(ItemStack{ item, extra }, rules);
        if (left == 0) return 0;

        for (ItemStack& s : m_slots) {
            if (s.item != item) continue;
            s.count += left;
            break;
        }

        return left;
    }

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

    void click(Inventory& inv, int slot, const StackRules& rules) {
        ItemStack s = inv.at(slot);

        if (m_stack.empty()) {
            m_stack = s;
            inv.set(slot, {});
            return;
        }

        if (s.empty() || s.item != m_stack.item) {
            inv.set(slot, m_stack);
            m_stack = s;
            return;
        }

        const int moved = vmin(rules.limit(s.item) - s.count, m_stack.count);
        if (moved <= 0) return;
        s.count += moved;
        m_stack.count -= moved;
        inv.set(slot, s);
        if (m_stack.count <= 0) m_stack.clear();
    }

    void place_one(Inventory& inv, int slot, const StackRules& rules) {
        if (m_stack.empty()) return;
        ItemStack s = inv.at(slot);
        if (!s.empty() && s.item != m_stack.item) return;
        if (!s.empty() && s.count >= rules.limit(s.item)) return;
        if (s.empty()) s = ItemStack{ m_stack.item, 0 };
        ++s.count;
        --m_stack.count;
        inv.set(slot, s);
        if (m_stack.count <= 0) m_stack.clear();
    }

    void take_half(Inventory& inv, int slot) {
        ItemStack s = inv.at(slot);
        if (s.empty() || !m_stack.empty()) return;
        const int half = (s.count + 1) / 2;
        m_stack = ItemStack{ s.item, half };
        s.count -= half;
        inv.set(slot, s);
    }

    void quick_move(Inventory& inv, int slot, const StackRules& rules) {
        const ItemStack s = inv.at(slot);
        if (s.empty()) return;
        inv.set(slot, {});
        const int from = Inventory::in_hotbar(slot) ? Inventory::HOTBAR : 0;
        const int to = Inventory::in_hotbar(slot) ? Inventory::SLOTS : Inventory::HOTBAR;
        int left = s.count;
        const int limit = rules.limit(s.item);

        for (int pass = 0; pass < 2 && left > 0; ++pass)
            for (int i = from; i < to && left > 0; ++i) {
                ItemStack t = inv.at(i);
                const bool fill = pass == 0 ? (!t.empty() && t.item == s.item) : t.empty();
                if (!fill) continue;
                if (t.empty()) t = ItemStack{ s.item, 0 };
                const int moved = vmin(limit - t.count, left);
                if (moved <= 0) continue;
                t.count += moved;
                left -= moved;
                inv.set(i, t);
            }

        if (left > 0) inv.set(slot, ItemStack{ s.item, left });
    }

    void put_back(Inventory& inv, const StackRules& rules) {
        if (m_stack.empty()) return;
        m_stack.count = inv.add(m_stack, rules);
        if (m_stack.count <= 0) m_stack.clear();
    }

    void set(const ItemStack& s) noexcept { m_stack = s; }

private:
    ItemStack m_stack;
};

} // namespace voxelspire

#endif // VOXELSPIRE_ITEM_INVENTORY_HPP