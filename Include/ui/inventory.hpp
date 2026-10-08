#ifndef VOXELSPIRE_UI_INVENTORY_SCREEN_HPP
#define VOXELSPIRE_UI_INVENTORY_SCREEN_HPP

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include "../item/inventory.hpp"
#include "../item/system.hpp"
#include "../render/item_icon.hpp"
#include "../render/vitals_hud.hpp"
#include "text_field.hpp"

namespace voxelspire {

struct UiRect {
    int x = 0, y = 0, w = 0, h = 0;

    bool contains(int px, int py) const noexcept { return px >= x && px < x + w && py >= y && py < y + h; }
};

struct InventoryTabSpec {
    std::string                                                     id;
    std::string                                                     name;
    std::function<void(fizmo::windows::Renderer&, const UiRect&)>   render;
    std::function<bool(int, int, unsigned int)>                     click;
};

namespace InventoryTabs {
    inline constexpr const char* INVENTORY = "inventory";
    inline constexpr const char* STATUS    = "status";
    inline constexpr const char* ITEMS     = "items";
} // namespace InventoryTabs

struct InventoryView {
    Inventory*               inventory     = nullptr;
    CursorStack*             cursor        = nullptr;
    StackRules*              rules         = nullptr;
    ItemSystem*              items         = nullptr;
    const PlayerSurvival*    survival      = nullptr;
    VitalsHud*               vitals        = nullptr;
    VitalsContext            vitals_context;
    const MenuSettings*      menu          = nullptr;
    const InventorySettings* settings      = nullptr;
    const HudSettings*       hud           = nullptr;
    bool                     catalog_gives = false;
    double                   carried       = 0.0;
    double                   carry_limit   = 0.0;
    bool                     weight_on     = false;
    std::string              close_key;
};

class InventoryScreen {
public:
    static constexpr unsigned int LEFT_MOUSE   = 1;
    static constexpr unsigned int RIGHT_MOUSE  = 2;
    static constexpr unsigned int MIDDLE_MOUSE = 3;
    static constexpr int          NO_HIT       = -1;

    InventoryScreen() {
        m_tabs.push_back({ InventoryTabs::INVENTORY, "Inventory", {}, {} });
        m_tabs.push_back({ InventoryTabs::STATUS, "Status", {}, {} });
        m_tabs.push_back({ InventoryTabs::ITEMS, "All items", {}, {} });
    }

    void add_tab(InventoryTabSpec tab) {
        for (InventoryTabSpec& t : m_tabs) if (t.id == tab.id) { t = std::move(tab); return; }
        m_tabs.push_back(std::move(tab));
    }

    bool is_open() const noexcept { return m_open; }

    void open(const InventoryView& v) {
        m_open = true;
        m_menu_open = false;
        m_stats_item = NO_ITEM;
        m_view = v;
    }

    void close(const InventoryView& v) {
        if (v.cursor && v.inventory && v.rules) v.cursor->put_back(*v.inventory, *v.rules);
        m_open = false;
        m_menu_open = false;
        m_stats_item = NO_ITEM;
        m_field.end();
    }

    const std::string& tab() const noexcept { return m_tab; }

    bool slot_rect(int slot, UiRect& out) const {
        for (const Hit& h : m_hits)
            if (h.kind == HitKind::Slot && h.index == slot) { out = h.rect; return true; }
        return false;
    }

    bool catalog_rect(int index, UiRect& out) const {
        for (const Hit& h : m_hits)
            if (h.kind == HitKind::CatalogItem && h.index == index) { out = h.rect; return true; }
        return false;
    }
    void show_tab(const std::string& id) { m_tab = id; m_menu_open = false; m_stats_item = NO_ITEM; }
    void show_stats(ItemHandle item) noexcept { m_stats_item = item; m_menu_open = false; }

    bool on_event(const fizmo::windows::WindowEvent& e, const InventoryView& v) {
        using fizmo::windows::WindowEventType;
        if (!m_open) return false;
        m_view = v;

        if (e.type == WindowEventType::MouseMove) {
            m_mouse_x = static_cast<int>(e.x);
            m_mouse_y = static_cast<int>(e.y);
            return false;
        }

        if (e.type == WindowEventType::KeyPress || e.type == WindowEventType::KeyRelease) {
            const bool down = e.type == WindowEventType::KeyPress;
            if (e.key_name == "LeftShift" || e.key_name == "RightShift") m_shift = down;
            if (e.key_name == "LeftControl" || e.key_name == "RightControl") m_control = down;
        }

        if (e.type == WindowEventType::MouseScroll) {
            scroll(e.scroll_delta);
            return true;
        }

        if (e.type == WindowEventType::MouseClick) {
            m_mouse_x = static_cast<int>(e.x);
            m_mouse_y = static_cast<int>(e.y);
            click(m_mouse_x, m_mouse_y, e.button);
            return true;
        }

        if (e.type != WindowEventType::KeyPress) return false;

        if (m_field.active()) {
            field_key(e.key_name);
            return true;
        }

        if (e.key_name == "Escape" && (m_menu_open || m_stats_item != NO_ITEM)) {
            m_menu_open = false;
            m_stats_item = NO_ITEM;
            return true;
        }

        return false;
    }

    void render(fizmo::windows::Renderer& r, unsigned int w, unsigned int h, const InventoryView& v) {
        if (!m_open) return;
        m_view = v;
        m_hits.clear();
        const MenuSettings& ms = *v.menu;
        const double scale = ms.scale;
        m_slot = static_cast<int>(std::lround(v.settings->slot_size * scale));
        m_gap = vmax(1, static_cast<int>(std::lround(SLOT_GAP * scale)));
        m_pad = static_cast<int>(std::lround(PADDING * scale));
        m_text = TEXT_SIZE * scale;
        r.draw_rect(0, 0, w, h, fizmo::graphics::Paint::fill(ms.backdrop));
        const int grid_w = Inventory::COLUMNS * (m_slot + m_gap) - m_gap;
        const int panel_w = vmin(static_cast<int>(w) - m_pad * 2, vmax(grid_w + m_pad * 2, static_cast<int>(MIN_PANEL_SLOTS * (m_slot + m_gap))) + static_cast<int>(DETAIL_SHARE * grid_w));
        const int panel_h = vmin(static_cast<int>(h) - m_pad * 2, static_cast<int>(PANEL_ROWS * (m_slot + m_gap)) + m_pad * 3);
        const UiRect panel{ (static_cast<int>(w) - panel_w) / 2, (static_cast<int>(h) - panel_h) / 2, panel_w, panel_h };
        r.draw_rect(panel.x, panel.y, static_cast<unsigned int>(panel.w), static_cast<unsigned int>(panel.h), fizmo::graphics::Paint::fill(ms.panel));
        const int tabs_h = tab_bar(r, panel);
        const UiRect body{ panel.x + m_pad, panel.y + tabs_h + m_pad, panel.w - m_pad * 2, panel.h - tabs_h - m_pad * 2 };
        const InventoryTabSpec* t = current();

        if (t && t->render) t->render(r, body);
        else if (m_tab == InventoryTabs::INVENTORY) inventory_tab(r, body);
        else if (m_tab == InventoryTabs::STATUS) status_tab(r, body);
        else if (m_tab == InventoryTabs::ITEMS) items_tab(r, body);

        if (m_stats_item != NO_ITEM) stats_popup(r, w, h);
        if (m_menu_open) context_menu(r, w, h);
        draw_cursor_stack(r);
    }

private:
    static constexpr double SLOT_GAP        = 4.0;
    static constexpr double PADDING         = 14.0;
    static constexpr double TEXT_SIZE       = 14.0;
    static constexpr double TAB_TEXT        = 15.0;
    static constexpr double TAB_HEIGHT      = 1.6;
    static constexpr double COUNT_TEXT      = 0.32;
    static constexpr double COUNT_SHADOW    = 1.0;
    static constexpr double ICON_INSET      = 0.08;
    static constexpr double MIN_PANEL_SLOTS = 12.0;
    static constexpr double PANEL_ROWS      = 11.0;
    static constexpr double DETAIL_SHARE    = 0.62;
    static constexpr double LINE            = 1.45;
    static constexpr double HOTBAR_GAP      = 0.5;
    static constexpr double CATEGORY_WIDTH  = 3.6;
    static constexpr double MENU_WIDTH      = 22.0;
    static constexpr double POPUP_WIDTH     = 34.0;
    static constexpr int    FIELD_LENGTH    = 3;
    static constexpr int    BIG_STEP        = 10;
    static constexpr std::size_t MAX_WIDTHS = 2048;

    enum class HitKind : std::uint8_t { Tab, Slot, Source, Category, CatalogItem, OptionMinus, OptionPlus, OptionValue, OptionReset, ShowStats, CloseStats, Menu, Popup };

    struct Hit {
        UiRect  rect;
        HitKind kind;
        int     index = 0;
    };

    struct CatalogCache {
        std::uint64_t           version  = ~std::uint64_t(0);
        int                     source   = -1;
        int                     category = -2;
        std::vector<ItemHandle> items;
    };

    struct StatsCache {
        ItemHandle            item    = NO_ITEM;
        std::uint64_t         version = ~std::uint64_t(0);
        std::vector<ItemStat> rows;
        std::vector<std::string> recipes;
    };

    const InventoryTabSpec* current() const {
        for (const InventoryTabSpec& t : m_tabs) if (t.id == m_tab) return &t;
        return nullptr;
    }

    fizmo::text::TextStyle style(double size, const Color& c, bool bold = false) const {
        fizmo::text::TextStyle s(size, c);
        if (bold) s.set_bold();
        return s;
    }

    int width_of(fizmo::windows::Renderer& r, const std::string& text, const fizmo::text::TextStyle& st, double size, bool bold) {
        if (m_widths.size() > MAX_WIDTHS) m_widths.clear();
        const std::string key = std::to_string(static_cast<int>(size * 10.0)) + (bold ? "b" : "r") + text;
        auto it = m_widths.find(key);
        if (it != m_widths.end()) return it->second;
        const int wv = static_cast<int>(r.measure_text(text, st).width);
        m_widths.emplace(key, wv);
        return wv;
    }

    int line_height() const noexcept { return static_cast<int>(std::lround(m_text * LINE)); }

    void hit(const UiRect& rect, HitKind kind, int index = 0) { m_hits.push_back({ rect, kind, index }); }

    const Hit* hit_at(int x, int y) const {
        for (auto it = m_hits.rbegin(); it != m_hits.rend(); ++it) if (it->rect.contains(x, y)) return &*it;
        return nullptr;
    }

    bool hovered(const UiRect& rect) const noexcept { return rect.contains(m_mouse_x, m_mouse_y); }

    int tab_bar(fizmo::windows::Renderer& r, const UiRect& panel) {
        const MenuSettings& ms = *m_view.menu;
        const double size = TAB_TEXT * ms.scale;
        const int h = static_cast<int>(std::lround(size * LINE * TAB_HEIGHT));
        r.draw_rect(panel.x, panel.y, static_cast<unsigned int>(panel.w), static_cast<unsigned int>(h), fizmo::graphics::Paint::fill(ms.sidebar));
        int x = panel.x + m_pad;

        for (std::size_t i = 0; i < m_tabs.size(); ++i) {
            const bool on = m_tabs[i].id == m_tab;
            const fizmo::text::TextStyle st = style(size, on ? ms.text : ms.muted, on);
            const int tw = width_of(r, m_tabs[i].name, st, size, on) + m_pad * 2;
            const UiRect rect{ x, panel.y, tw, h };
            if (on) r.draw_rect(rect.x, rect.y + h - m_gap, static_cast<unsigned int>(rect.w), static_cast<unsigned int>(m_gap), fizmo::graphics::Paint::fill(ms.accent));
            else if (hovered(rect)) r.draw_rect(rect.x, rect.y, static_cast<unsigned int>(rect.w), static_cast<unsigned int>(rect.h), fizmo::graphics::Paint::fill(ms.row_hover));
            r.draw_text(x + m_pad, panel.y + (h - static_cast<int>(size)) / 2, m_tabs[i].name, st);
            hit(rect, HitKind::Tab, static_cast<int>(i));
            x += tw;
        }

        if (!m_view.close_key.empty()) {
            const std::string hint = m_view.close_key + " to close";
            const fizmo::text::TextStyle st = style(m_text, ms.muted);
            r.draw_text(panel.x + panel.w - m_pad - width_of(r, hint, st, m_text, false), panel.y + (h - static_cast<int>(m_text)) / 2, hint, st);
        }

        return h;
    }

    void slot_box(fizmo::windows::Renderer& r, const UiRect& rect, const ItemStack& stack, bool selected, bool highlight) {
        const InventorySettings& is = *m_view.settings;
        r.draw_rect(rect.x, rect.y, static_cast<unsigned int>(rect.w), static_cast<unsigned int>(rect.h), fizmo::graphics::Paint::fill(highlight ? is.slot_hover : is.slot_color));
        if (selected) r.draw_rect(rect.x, rect.y, static_cast<unsigned int>(rect.w), static_cast<unsigned int>(rect.h), fizmo::graphics::Paint::stroke(is.selected_color, static_cast<unsigned int>(m_gap)));
        draw_stack(r, rect, stack);
    }

    void draw_stack(fizmo::windows::Renderer& r, const UiRect& rect, const ItemStack& stack) {
        if (stack.empty()) return;
        const Item* item = m_view.items->items().get(stack.item);
        if (!item) return;
        const int inset = static_cast<int>(std::lround(rect.w * ICON_INSET));
        m_icons.draw(r, *item, rect.x + inset, rect.y + inset, rect.w - inset * 2);
        if (stack.count <= 1 || !m_view.settings->show_counts) return;
        const std::string n = std::to_string(stack.count);
        const double size = rect.h * COUNT_TEXT;
        const InventorySettings& is = *m_view.settings;
        fizmo::text::TextStyle st = style(size, is.count_color, true);
        st.set_shadow(COUNT_SHADOW, COUNT_SHADOW, 0.0, is.count_shadow);
        r.draw_text(rect.x + rect.w - width_of(r, n, st, size, true) - inset, rect.y + rect.h - static_cast<int>(size * LINE), n, st);
    }

    void inventory_tab(fizmo::windows::Renderer& r, const UiRect& body) {
        const MenuSettings& ms = *m_view.menu;
        const Inventory& inv = *m_view.inventory;
        const int step = m_slot + m_gap;
        const int x0 = body.x, y0 = body.y + line_height();
        const fizmo::text::TextStyle head = style(m_text, ms.muted, true);
        r.draw_text(x0, body.y, "Backpack", head);

        for (int row = 0; row < Inventory::ROWS; ++row)
            for (int col = 0; col < Inventory::COLUMNS; ++col) {
                const int slot = Inventory::main_slot(col, row);
                const UiRect rect{ x0 + col * step, y0 + row * step, m_slot, m_slot };
                slot_box(r, rect, inv.at(slot), false, hovered(rect));
                hit(rect, HitKind::Slot, slot);
            }

        const int hy = y0 + Inventory::ROWS * step + static_cast<int>(step * HOTBAR_GAP);
        r.draw_text(x0, hy, "Hotbar", head);

        for (int i = 0; i < Inventory::HOTBAR; ++i) {
            const UiRect rect{ x0 + i * step, hy + line_height(), m_slot, m_slot };
            slot_box(r, rect, inv.at(i), i == inv.selected(), hovered(rect));
            hit(rect, HitKind::Slot, i);
        }

        const int by = hy + line_height() + step + m_gap;
        const fizmo::text::TextStyle body_text = style(m_text, ms.text);
        char buf[TEXT_BUFFER];
        if (m_view.weight_on) std::snprintf(buf, sizeof(buf), "Carrying %.1f kg of %.0f kg", m_view.carried, m_view.carry_limit);
        else std::snprintf(buf, sizeof(buf), "Carrying %.1f kg", m_view.carried);
        r.draw_text(x0, by, buf, body_text);
        r.draw_text(x0, by + line_height(), "Click to pick up or drop. Shift-click moves between backpack and hotbar. Right-click for options.", style(m_text, ms.muted));
        detail_pane(r, { x0 + Inventory::COLUMNS * step + m_pad, body.y, body.x + body.w - (x0 + Inventory::COLUMNS * step + m_pad), body.h }, hovered_item());
    }

    ItemHandle hovered_item() const {
        const Hit* h = hit_at(m_mouse_x, m_mouse_y);
        if (!h) return m_detail;
        if (h->kind == HitKind::Slot) { const ItemStack& s = m_view.inventory->at(h->index); return s.empty() ? m_detail : s.item; }
        if (h->kind == HitKind::CatalogItem && h->index >= 0 && h->index < static_cast<int>(m_catalog.items.size())) return m_catalog.items[static_cast<std::size_t>(h->index)];
        return m_detail;
    }

    void status_tab(fizmo::windows::Renderer& r, const UiRect& body) {
        if (!m_view.survival || !m_view.vitals || !m_view.hud) return;
        const VitalsArea area{ body.x, body.y, body.w, body.h };
        m_view.vitals->render_panel(r, static_cast<unsigned int>(body.w), static_cast<unsigned int>(body.h), *m_view.hud, *m_view.survival, m_view.vitals_context, &area);
    }

    void refresh_catalog() {
        ItemSystem& sys = *m_view.items;
        const auto& sources = sys.categories().sources();
        if (sources.empty()) return;
        m_source = vclamp(m_source, 0, static_cast<int>(sources.size()) - 1);
        const ItemSource& src = sources[static_cast<std::size_t>(m_source)];
        m_category = vclamp(m_category, -1, static_cast<int>(src.categories.size()) - 1);
        const std::uint64_t version = sys.items().version() * VERSION_MIX + sys.categories().version();
        if (m_catalog.version == version && m_catalog.source == m_source && m_catalog.category == m_category) return;
        m_catalog.items.clear();
        const ItemCategory* want = m_category >= 0 ? &src.categories[static_cast<std::size_t>(m_category)] : nullptr;

        sys.items().for_each([&](const Item& item) {
            const ItemPlace& p = sys.place(item.handle());
            if (!p.source || p.source->owner != src.owner) return;
            if (want && (!p.category || p.category->key != want->key)) return;
            m_catalog.items.push_back(item.handle());
        });

        m_catalog.version  = sys.items().version() * VERSION_MIX + sys.categories().version();
        m_catalog.source   = m_source;
        m_catalog.category = m_category;
        m_scroll = 0;
    }

    void items_tab(fizmo::windows::Renderer& r, const UiRect& body) {
        const MenuSettings& ms = *m_view.menu;
        refresh_catalog();
        const auto& sources = m_view.items->categories().sources();
        if (sources.empty()) return;
        int x = body.x;
        const int lh = line_height();

        for (std::size_t i = 0; i < sources.size(); ++i) {
            const bool on = static_cast<int>(i) == m_source;
            const fizmo::text::TextStyle st = style(m_text, on ? ms.accent : ms.muted, on);
            const int tw = width_of(r, sources[i].name, st, m_text, on) + m_pad;
            const UiRect rect{ x, body.y, tw, lh };
            if (hovered(rect) && !on) r.draw_rect(rect.x, rect.y, static_cast<unsigned int>(rect.w), static_cast<unsigned int>(rect.h), fizmo::graphics::Paint::fill(ms.row_hover));
            r.draw_text(x + m_pad / 2, body.y, sources[i].name, st);
            hit(rect, HitKind::Source, static_cast<int>(i));
            x += tw + m_gap;
        }

        const ItemSource& src = sources[static_cast<std::size_t>(m_source)];
        const int cat_w = static_cast<int>(m_slot * CATEGORY_WIDTH);
        int y = body.y + lh + m_gap;

        for (int i = -1; i < static_cast<int>(src.categories.size()); ++i) {
            const std::string name = i < 0 ? std::string("Everything") : src.categories[static_cast<std::size_t>(i)].name;
            const bool on = i == m_category;
            const UiRect rect{ body.x, y, cat_w, lh };
            if (on) r.draw_rect(rect.x, rect.y, static_cast<unsigned int>(rect.w), static_cast<unsigned int>(rect.h), fizmo::graphics::Paint::fill(ms.control));
            else if (hovered(rect)) r.draw_rect(rect.x, rect.y, static_cast<unsigned int>(rect.w), static_cast<unsigned int>(rect.h), fizmo::graphics::Paint::fill(ms.row_hover));
            r.draw_text(body.x + m_gap * 2, y, name, style(m_text, on ? ms.text : ms.muted, on));
            hit(rect, HitKind::Category, i);
            y += lh;
        }

        const int step = m_slot + m_gap;
        const int gx = body.x + cat_w + m_pad, gy = body.y + lh + m_gap;
        const int cols = vmax(1, vmin(Inventory::COLUMNS - static_cast<int>(CATEGORY_WIDTH), (body.w - cat_w - m_pad) / step));
        const int rows = vmax(1, (body.y + body.h - gy) / step);
        m_catalog_rows = rows;
        m_catalog_cols = cols;
        const int total_rows = (static_cast<int>(m_catalog.items.size()) + cols - 1) / cols;
        m_scroll = vclamp(m_scroll, 0, vmax(0, total_rows - rows));

        if (m_catalog.items.empty()) r.draw_text(gx, gy, "Nothing here yet.", style(m_text, ms.muted));

        for (int row = 0; row < rows; ++row)
            for (int col = 0; col < cols; ++col) {
                const int index = (row + m_scroll) * cols + col;
                if (index >= static_cast<int>(m_catalog.items.size())) break;
                const UiRect rect{ gx + col * step, gy + row * step, m_slot, m_slot };
                const ItemHandle h = m_catalog.items[static_cast<std::size_t>(index)];
                slot_box(r, rect, ItemStack{ h, 1 }, h == m_detail, hovered(rect));
                hit(rect, HitKind::CatalogItem, index);
            }

        if (total_rows > rows) {
            char buf[TEXT_BUFFER];
            std::snprintf(buf, sizeof(buf), "Scroll for more (%d of %d rows)", m_scroll + rows, total_rows);
            r.draw_text(gx, gy + rows * step, buf, style(m_text, ms.muted));
        }

        const int dx = gx + cols * step + m_pad;
        detail_pane(r, { dx, body.y, body.x + body.w - dx, body.h }, hovered_item());
    }

    const StatsCache& stats_for(ItemHandle h) {
        const std::uint64_t version = m_view.rules ? m_view.rules->version() : 0;
        if (m_stats.item == h && m_stats.version == version) return m_stats;
        m_stats.item = h;
        m_stats.version = version;
        m_stats.rows.clear();
        m_stats.recipes.clear();
        const Item* item = m_view.items->items().get(h);
        if (!item) return m_stats;
        m_stats.rows = m_view.items->stats().collect(*item, m_view.items->stat_context(m_view.rules));
        for (const Recipe* rec : m_view.items->recipes().making(h)) m_stats.recipes.push_back(ItemStatRegistry::recipe_text(*rec, m_view.items->items()));
        return m_stats;
    }

    int text_lines(fizmo::windows::Renderer& r, int x, int y, int width, const std::string& text, const fizmo::text::TextStyle& st) {
        std::string line, word;
        int lines = 0;
        auto flush = [&]() { r.draw_text(x, y + lines * line_height(), line, st); ++lines; line.clear(); };

        for (std::size_t i = 0; i <= text.size(); ++i) {
            if (i < text.size() && text[i] != ' ') { word += text[i]; continue; }
            const std::string next = line.empty() ? word : line + " " + word;
            if (!line.empty() && width_of(r, next, st, m_text, false) > width) { flush(); line = word; }
            else line = next;
            word.clear();
        }

        if (!line.empty()) flush();
        return lines;
    }

    void detail_pane(fizmo::windows::Renderer& r, const UiRect& area, ItemHandle h) {
        if (area.w < m_slot * 2) return;
        const MenuSettings& ms = *m_view.menu;
        const Item* item = m_view.items->items().get(h);

        if (!item) {
            text_lines(r, area.x, area.y, area.w, "Point at an item to see what it is.", style(m_text, ms.muted));
            return;
        }

        const ItemPlace& place = m_view.items->place(h);
        m_icons.draw(r, *item, area.x, area.y, m_slot);
        const int tx = area.x + m_slot + m_pad;
        r.draw_text(tx, area.y, item->name(), style(m_text * TITLE_SCALE, ms.text, true));
        const std::string where = (place.source ? place.source->name : std::string("Other")) + (place.category ? " / " + place.category->name : std::string());
        r.draw_text(tx, area.y + static_cast<int>(m_text * TITLE_SCALE * LINE), where, style(m_text, ms.muted));
        int y = area.y + m_slot + m_pad;
        const StatsCache& st = stats_for(h);
        const fizmo::text::TextStyle label = style(m_text, ms.muted), value = style(m_text, ms.text);
        r.draw_text(area.x, y, "Crafting", style(m_text, ms.accent, true));
        y += line_height();

        if (st.recipes.empty()) {
            r.draw_text(area.x, y, "NO CRAFTING RECIPE", style(m_text, ms.danger, true));
            y += line_height();
        } else {
            for (const std::string& rec : st.recipes) y += text_lines(r, area.x, y, area.w, rec, value) * line_height();
        }

        y += m_gap * 2;
        int label_w = 0;
        for (const ItemStat& s : st.rows) label_w = vmax(label_w, width_of(r, s.label, label, m_text, false));
        label_w = vmin(label_w + m_pad, static_cast<int>(area.w * LABEL_SHARE));

        for (const ItemStat& s : st.rows) {
            if (y + line_height() > area.y + area.h) break;
            r.draw_text(area.x, y, s.label, label);
            y += text_lines(r, area.x + label_w, y, area.w - label_w, s.value, value) * line_height();
        }
    }

    void context_menu(fizmo::windows::Renderer& r, unsigned int w, unsigned int h) {
        const MenuSettings& ms = *m_view.menu;
        const Item* item = m_view.items->items().get(m_menu_ctx.item);
        if (!item) { m_menu_open = false; return; }
        const std::vector<const ItemOption*> options = m_view.items->options().for_item(m_menu_ctx);
        const int lh = line_height();
        const int mw = static_cast<int>(m_text * MENU_WIDTH);
        const int mh = lh * (static_cast<int>(options.size()) + 2) + m_pad * 2;
        const int x = vmin(m_menu_x, static_cast<int>(w) - mw - m_pad), y = vmin(m_menu_y, static_cast<int>(h) - mh - m_pad);
        const UiRect box{ x, y, mw, mh };
        r.draw_rect(box.x, box.y, static_cast<unsigned int>(box.w), static_cast<unsigned int>(box.h), fizmo::graphics::Paint::fill(ms.sidebar));
        r.draw_rect(box.x, box.y, static_cast<unsigned int>(box.w), static_cast<unsigned int>(box.h), fizmo::graphics::Paint::stroke(ms.accent, 1));
        hit(box, HitKind::Menu);
        r.draw_text(x + m_pad, y + m_pad, item->name(), style(m_text, ms.text, true));
        int ry = y + m_pad + lh;
        const int button = lh;

        for (std::size_t i = 0; i < options.size(); ++i) {
            const ItemOption& o = *options[i];
            r.draw_text(x + m_pad, ry, o.label, style(m_text, ms.muted));

            if (o.kind == ItemOptionKind::Number && o.get) {
                const int value_w = static_cast<int>(m_text * VALUE_WIDTH);
                const int vx = x + mw - m_pad - button * RESET_BUTTONS - button * 2 - value_w - m_gap * 3;
                const UiRect minus{ vx, ry, button, button - m_gap }, value{ vx + button + m_gap, ry, value_w, button - m_gap };
                const UiRect plus{ value.x + value_w + m_gap, ry, button, button - m_gap }, reset{ plus.x + button + m_gap, ry, button * RESET_BUTTONS, button - m_gap };
                draw_button(r, minus, "-");
                draw_button(r, plus, "+");
                r.draw_rect(value.x, value.y, static_cast<unsigned int>(value.w), static_cast<unsigned int>(value.h), fizmo::graphics::Paint::fill(ms.control));
                const bool editing = m_field.active() && m_field_option == static_cast<int>(i);
                const std::string shown = editing ? m_field.text() + "|" : std::to_string(o.get(m_menu_ctx));
                r.draw_text(value.x + m_gap * 2, value.y, shown, style(m_text, editing ? ms.accent : ms.text));
                const bool changed = o.changed && o.changed(m_menu_ctx);
                if (changed) draw_button(r, reset, "Reset");
                hit(minus, HitKind::OptionMinus, static_cast<int>(i));
                hit(plus, HitKind::OptionPlus, static_cast<int>(i));
                hit(value, HitKind::OptionValue, static_cast<int>(i));
                if (changed) hit(reset, HitKind::OptionReset, static_cast<int>(i));
            } else if (o.kind == ItemOptionKind::Action) {
                const UiRect run{ x + mw - m_pad - button * 3, ry, button * 3, button - m_gap };
                draw_button(r, run, "Do");
                hit(run, HitKind::OptionValue, static_cast<int>(i));
            }

            ry += lh;
        }

        const UiRect stats{ x + m_pad, ry, mw - m_pad * 2, lh - m_gap };
        draw_button(r, stats, "View stats");
        hit(stats, HitKind::ShowStats);
    }

    void draw_button(fizmo::windows::Renderer& r, const UiRect& rect, const std::string& label, bool enabled = true) {
        const MenuSettings& ms = *m_view.menu;
        r.draw_rect(rect.x, rect.y, static_cast<unsigned int>(rect.w), static_cast<unsigned int>(rect.h), fizmo::graphics::Paint::fill(hovered(rect) && enabled ? ms.accent : ms.control));
        if (label.empty()) return;
        const fizmo::text::TextStyle st = style(m_text, enabled ? ms.text : ms.muted);
        r.draw_text(rect.x + (rect.w - width_of(r, label, st, m_text, false)) / 2, rect.y, label, st);
    }

    void stats_popup(fizmo::windows::Renderer& r, unsigned int w, unsigned int h) {
        const MenuSettings& ms = *m_view.menu;
        const int pw = vmin(static_cast<int>(m_text * POPUP_WIDTH), static_cast<int>(w) - m_pad * 2);
        const int ph = vmin(static_cast<int>(h) - m_pad * 2, static_cast<int>(PANEL_ROWS * (m_slot + m_gap)));
        const UiRect box{ (static_cast<int>(w) - pw) / 2, (static_cast<int>(h) - ph) / 2, pw, ph };
        r.draw_rect(box.x, box.y, static_cast<unsigned int>(box.w), static_cast<unsigned int>(box.h), fizmo::graphics::Paint::fill(ms.sidebar));
        r.draw_rect(box.x, box.y, static_cast<unsigned int>(box.w), static_cast<unsigned int>(box.h), fizmo::graphics::Paint::stroke(ms.accent, 1));
        hit(box, HitKind::Popup);
        const UiRect close{ box.x + box.w - m_pad - line_height() * 3, box.y + m_pad, line_height() * 3, line_height() - m_gap };
        draw_button(r, close, "Close");
        hit(close, HitKind::CloseStats);
        detail_pane(r, { box.x + m_pad, box.y + m_pad, box.w - m_pad * 3 - close.w, box.h - m_pad * 2 }, m_stats_item);
    }

    void draw_cursor_stack(fizmo::windows::Renderer& r) {
        if (!m_view.cursor || m_view.cursor->empty()) return;
        const UiRect rect{ m_mouse_x - m_slot / 2, m_mouse_y - m_slot / 2, m_slot, m_slot };
        draw_stack(r, rect, m_view.cursor->stack());
    }

    void scroll(int delta) {
        if (m_tab != InventoryTabs::ITEMS) return;
        m_scroll = vmax(0, m_scroll - delta);
    }

    void open_menu(ItemHandle item, int slot) {
        m_menu_open = true;
        m_menu_x = m_mouse_x;
        m_menu_y = m_mouse_y;
        m_menu_ctx = ItemContext{ item, slot, m_view.inventory, m_view.rules, &m_view.items->items() };
        m_field.end();
    }

    const ItemOption* option(int index) const {
        const std::vector<const ItemOption*> options = m_view.items->options().for_item(m_menu_ctx);
        return index >= 0 && index < static_cast<int>(options.size()) ? options[static_cast<std::size_t>(index)] : nullptr;
    }

    void set_option(const ItemOption& o, int v) {
        if (!o.set) return;
        o.set(m_menu_ctx, static_cast<int>(o.limits.clamp(v)));
    }

    void field_key(const std::string& key) {
        const TextResult res = m_field.key(key, m_shift, m_control);
        if (res != TextResult::Commit && res != TextResult::Cancel) return;
        const ItemOption* o = option(m_field_option);
        const std::string text = m_field.text();
        m_field.end();
        if (res != TextResult::Commit || !o || text.empty()) return;
        set_option(*o, std::atoi(text.c_str()));
    }

    void click(int x, int y, unsigned int button) {
        if (m_field.active()) {
            const ItemOption* o = option(m_field_option);
            const std::string text = m_field.text();
            m_field.end();
            if (o && !text.empty()) set_option(*o, std::atoi(text.c_str()));
        }

        const Hit* h = hit_at(x, y);

        if (m_menu_open && (!h || !in_menu(h->kind))) { m_menu_open = false; return; }
        if (m_stats_item != NO_ITEM && (!h || (h->kind != HitKind::Popup && h->kind != HitKind::CloseStats))) { m_stats_item = NO_ITEM; return; }
        if (!h) return;

        switch (h->kind) {
            case HitKind::Tab:
                if (h->index < static_cast<int>(m_tabs.size())) {
                    const InventoryTabSpec& t = m_tabs[static_cast<std::size_t>(h->index)];
                    show_tab(t.id);
                }
                break;
            case HitKind::Slot:        slot_click(h->index, button); break;
            case HitKind::Source:      m_source = h->index; m_category = -1; break;
            case HitKind::Category:    m_category = h->index; break;
            case HitKind::CatalogItem: catalog_click(h->index, button); break;
            case HitKind::OptionMinus:
            case HitKind::OptionPlus:
                if (const ItemOption* o = option(h->index); o && o->get) set_option(*o, o->get(m_menu_ctx) + (h->kind == HitKind::OptionPlus ? 1 : -1) * (m_shift ? BIG_STEP : 1));
                break;
            case HitKind::OptionValue:
                if (const ItemOption* o = option(h->index)) {
                    if (o->kind == ItemOptionKind::Action && o->run) o->run(m_menu_ctx);
                    else if (o->get) { m_field.begin(std::to_string(o->get(m_menu_ctx)), TextFilter::Integer, false, FIELD_LENGTH); m_field_option = h->index; }
                }
                break;
            case HitKind::OptionReset:
                if (const ItemOption* o = option(h->index); o && o->reset) o->reset(m_menu_ctx);
                break;
            case HitKind::ShowStats:   m_stats_item = m_menu_ctx.item; m_menu_open = false; break;
            case HitKind::CloseStats:  m_stats_item = NO_ITEM; break;
            case HitKind::Menu:
            case HitKind::Popup:       break;
        }

        if (const InventoryTabSpec* t = current(); t && t->click) t->click(x, y, button);
    }

    static bool in_menu(HitKind k) noexcept {
        return k == HitKind::OptionMinus || k == HitKind::OptionPlus || k == HitKind::OptionValue || k == HitKind::OptionReset || k == HitKind::ShowStats || k == HitKind::Menu;
    }

    void slot_click(int slot, unsigned int button) {
        Inventory& inv = *m_view.inventory;
        CursorStack& cur = *m_view.cursor;
        const StackRules& rules = *m_view.rules;

        if (button == LEFT_MOUSE) {
            if (m_shift && cur.empty()) cur.quick_move(inv, slot, rules);
            else cur.click(inv, slot, rules);
            return;
        }

        if (button == RIGHT_MOUSE) {
            if (!cur.empty()) { cur.place_one(inv, slot, rules); return; }
            const ItemStack& s = inv.at(slot);
            if (!s.empty()) open_menu(s.item, slot);
            return;
        }

        if (button == MIDDLE_MOUSE) cur.take_half(inv, slot);
    }

    void catalog_click(int index, unsigned int button) {
        if (index < 0 || index >= static_cast<int>(m_catalog.items.size())) return;
        const ItemHandle h = m_catalog.items[static_cast<std::size_t>(index)];
        if (button == RIGHT_MOUSE) { open_menu(h, Inventory::NONE); return; }
        if (button != LEFT_MOUSE) return;
        m_detail = h;
        if (!m_view.catalog_gives) return;
        CursorStack& cur = *m_view.cursor;
        const int limit = m_view.rules->limit(h);
        if (cur.empty() || cur.stack().item != h) cur.set(ItemStack{ h, m_shift ? limit : 1 });
        else cur.set(ItemStack{ h, vmin(cur.stack().count + 1, limit) });
    }

    static constexpr double TITLE_SCALE  = 1.2;
    static constexpr double LABEL_SHARE  = 0.5;
    static constexpr double VALUE_WIDTH  = 3.2;
    static constexpr int    RESET_BUTTONS = 3;
    static constexpr std::size_t TEXT_BUFFER = 160;
    static constexpr std::uint64_t VERSION_MIX = 1000003;

    std::vector<InventoryTabSpec>          m_tabs;
    std::string                            m_tab = InventoryTabs::INVENTORY;
    InventoryView                          m_view;
    std::vector<Hit>                       m_hits;
    std::unordered_map<std::string, int>   m_widths;
    ItemIconPainter                        m_icons;
    CatalogCache                           m_catalog;
    StatsCache                             m_stats;
    TextField                              m_field;
    ItemContext                            m_menu_ctx;
    ItemHandle                             m_detail       = NO_ITEM;
    ItemHandle                             m_stats_item   = NO_ITEM;
    bool                                   m_open         = false;
    bool                                   m_menu_open    = false;
    bool                                   m_shift        = false;
    bool                                   m_control      = false;
    int                                    m_field_option = -1;
    int                                    m_menu_x       = 0;
    int                                    m_menu_y       = 0;
    int                                    m_mouse_x      = 0;
    int                                    m_mouse_y      = 0;
    int                                    m_source       = 0;
    int                                    m_category     = -1;
    int                                    m_scroll       = 0;
    int                                    m_catalog_rows = 1;
    int                                    m_catalog_cols = 1;
    int                                    m_slot         = 40;
    int                                    m_gap          = 4;
    int                                    m_pad          = 14;
    double                                 m_text         = TEXT_SIZE;
};

} // namespace voxelspire

#endif // VOXELSPIRE_UI_INVENTORY_SCREEN_HPP