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

    InventoryScreen();

    void add_tab(InventoryTabSpec tab);

    bool is_open() const noexcept { return m_open; }

    void open(const InventoryView& v) {
        m_open = true;
        m_menu_open = false;
        m_stats_item = NO_ITEM;
        m_view = v;
    }

    void close(const InventoryView& v);

    const std::string& tab() const noexcept { return m_tab; }

    bool slot_rect(int slot, UiRect& out) const;

    bool catalog_rect(int index, UiRect& out) const;
    void show_tab(const std::string& id) { m_tab = id; m_menu_open = false; m_stats_item = NO_ITEM; }
    void show_stats(ItemHandle item) noexcept { m_stats_item = item; m_menu_open = false; }

    bool on_event(const fizmo::windows::WindowEvent& e, const InventoryView& v);

    void render(fizmo::windows::Renderer& r, unsigned int w, unsigned int h, const InventoryView& v);

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

    int width_of(fizmo::windows::Renderer& r, const std::string& text, const fizmo::text::TextStyle& st, double size, bool bold);

    int line_height() const noexcept { return static_cast<int>(std::lround(m_text * LINE)); }

    void hit(const UiRect& rect, HitKind kind, int index = 0) { m_hits.push_back({ rect, kind, index }); }

    const Hit* hit_at(int x, int y) const;

    bool hovered(const UiRect& rect) const noexcept { return rect.contains(m_mouse_x, m_mouse_y); }

    int tab_bar(fizmo::windows::Renderer& r, const UiRect& panel);

    void slot_box(fizmo::windows::Renderer& r, const UiRect& rect, const ItemStack& stack, bool selected, bool highlight);

    void draw_stack(fizmo::windows::Renderer& r, const UiRect& rect, const ItemStack& stack);

    void inventory_tab(fizmo::windows::Renderer& r, const UiRect& body);

    ItemHandle hovered_item() const;

    void status_tab(fizmo::windows::Renderer& r, const UiRect& body);

    void refresh_catalog();

    void items_tab(fizmo::windows::Renderer& r, const UiRect& body);

    const StatsCache& stats_for(ItemHandle h);

    int text_lines(fizmo::windows::Renderer& r, int x, int y, int width, const std::string& text, const fizmo::text::TextStyle& st);

    void detail_pane(fizmo::windows::Renderer& r, const UiRect& area, ItemHandle h);

    void context_menu(fizmo::windows::Renderer& r, unsigned int w, unsigned int h);

    void draw_button(fizmo::windows::Renderer& r, const UiRect& rect, const std::string& label, bool enabled = true);

    void stats_popup(fizmo::windows::Renderer& r, unsigned int w, unsigned int h);

    void draw_cursor_stack(fizmo::windows::Renderer& r);

    void scroll(int delta) {
        if (m_tab != InventoryTabs::ITEMS) return;
        m_scroll = vmax(0, m_scroll - delta);
    }

    void open_menu(ItemHandle item, int slot);

    const ItemOption* option(int index) const;

    void set_option(const ItemOption& o, int v) {
        if (!o.set) return;
        o.set(m_menu_ctx, static_cast<int>(o.limits.clamp(v)));
    }

    void field_key(const std::string& key);

    void click(int x, int y, unsigned int button);

    static bool in_menu(HitKind k) noexcept;

    void slot_click(int slot, unsigned int button);

    void catalog_click(int index, unsigned int button);

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