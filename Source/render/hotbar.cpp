#include "render/hotbar.hpp"

namespace voxelspire {

void HotbarHud::render(fizmo::windows::Renderer& r, unsigned int w, unsigned int h, const InventorySettings& s, double scale, const Inventory& inv, const ItemRegistry& items) {
    if (!s.show_hotbar) return;
    const int slot = static_cast<int>(std::lround(s.slot_size * scale));
    const int gap = vmax(1, static_cast<int>(std::lround(GAP * scale)));
    const int total = Inventory::HOTBAR * (slot + gap) - gap;
    const int x0 = (static_cast<int>(w) - total) / 2;
    const int y0 = static_cast<int>(h) - static_cast<int>(std::lround(s.hotbar_bottom * scale)) - slot;
    const int outline = vmax(1, static_cast<int>(std::lround(OUTLINE * scale)));

    for (int i = 0; i < Inventory::HOTBAR; ++i) {
        const int x = x0 + i * (slot + gap);
        r.draw_rect(x, y0, static_cast<unsigned int>(slot), static_cast<unsigned int>(slot), fizmo::graphics::Paint::fill(s.slot_color));
        if (i == inv.selected()) r.draw_rect(x - outline, y0 - outline, static_cast<unsigned int>(slot + outline * 2), static_cast<unsigned int>(slot + outline * 2), fizmo::graphics::Paint::stroke(s.selected_color, static_cast<unsigned int>(outline)));
        number(r, s, x, y0, slot, i);
        const ItemStack& st = inv.at(i);
        if (st.empty()) continue;
        const Item* item = items.get(st.item);
        if (!item) continue;
        const int inset = static_cast<int>(std::lround(slot * ICON_INSET));
        m_icons.draw(r, *item, x + inset, y0 + inset, slot - inset * 2);
        if (st.count <= 1 || !s.show_counts) continue;
        const double size = slot * COUNT_TEXT;
        fizmo::text::TextStyle ts(size, s.count_color);
        ts.set_bold();
        ts.set_shadow(COUNT_SHADOW, COUNT_SHADOW, 0.0, s.count_shadow);
        const std::string n = std::to_string(st.count);
        r.draw_text(x + slot - static_cast<int>(r.measure_text(n, ts).width) - inset, y0 + slot - static_cast<int>(size * LINE), n, ts);
    }
}

void HotbarHud::number(fizmo::windows::Renderer& r, const InventorySettings& s, int x, int y, int slot, int i) const {
    if (!s.show_numbers) return;
    fizmo::text::TextStyle ts(slot * NUMBER_TEXT, s.number_color);
    r.draw_text(x + static_cast<int>(slot * ICON_INSET * HALF), y, std::to_string((i + 1) % KEYS), ts);
}

} // namespace voxelspire
