#ifndef VOXELSPIRE_UI_START_MENU_HPP
#define VOXELSPIRE_UI_START_MENU_HPP

#include <algorithm>
#include <cstdint>
#include <ctime>
#include <string>
#include <vector>
#include "menu_painter.hpp"
#include "world_store.hpp"

namespace voxelspire {

enum class StartAction : std::uint8_t { None = 0, Play, Create, Edit, Delete, Settings, Quit };

class StartMenu {
public:
    static constexpr unsigned int LEFT_MOUSE      = 1;
    static constexpr double       CONFIRM_SECONDS = 4.0;
    static constexpr double       STATUS_SECONDS  = 6.0;
    static constexpr int          SCROLL_ROWS     = 1;

    void set_worlds(std::vector<WorldRecord> worlds) {
        const std::string keep = m_selected < m_worlds.size() ? m_worlds[m_selected].folder : std::string();
        m_worlds = std::move(worlds);
        m_selected = 0;
        select(keep);
        m_confirm = 0.0;
    }

    void select(const std::string& folder) {
        for (std::size_t i = 0; i < m_worlds.size(); ++i) if (m_worlds[i].folder == folder) m_selected = i;
    }

    const WorldRecord* selected() const noexcept { return m_selected < m_worlds.size() ? &m_worlds[m_selected] : nullptr; }

    void set_status(std::string message, bool problem = false) {
        m_status = std::move(message);
        m_status_problem = problem;
        m_status_time = STATUS_SECONDS;
    }

    StartAction take_action() noexcept { const StartAction a = m_action; m_action = StartAction::None; return a; }

    bool on_event(const fizmo::windows::WindowEvent& e) {
        using fizmo::windows::WindowEventType;

        if (e.type == WindowEventType::MouseScroll) {
            m_scroll_steps += e.scroll_delta;
            return true;
        }

        if (e.button == LEFT_MOUSE && (e.type == WindowEventType::MouseClick || e.type == WindowEventType::MouseDoubleClick)) {
            click(static_cast<int>(e.x), static_cast<int>(e.y), e.type == WindowEventType::MouseDoubleClick);
            return true;
        }

        if (e.type != WindowEventType::KeyPress) return false;
        if (e.key_name == "Enter") { if (selected()) m_action = StartAction::Play; return true; }
        if (e.key_name == "UpArrow" && m_selected > 0) { --m_selected; m_reveal = true; return true; }
        if (e.key_name == "DownArrow" && m_selected + 1 < m_worlds.size()) { ++m_selected; m_reveal = true; return true; }
        if (e.key_name == "Delete") { press_delete(); return true; }
        return false;
    }

    void update(const fizmo::windows::InputManager& input, double dt) {
        m_mouse_x = input.mouse_x();
        m_mouse_y = input.mouse_y();
        if (m_confirm > 0.0) m_confirm -= dt;
        if (m_status_time > 0.0) m_status_time -= dt;
        m_scroll -= m_scroll_steps * SCROLL_ROWS * m_row_h;
        m_scroll_steps = 0;
    }

    void render(fizmo::windows::Renderer& r, unsigned int w, unsigned int h, const MenuSettings& style) {
        m_hits.clear();
        const double fit = vmin(1.0, vmin((w - MARGIN * 2.0) / PANEL_W, (h - MARGIN * 2.0 - TITLE_SPACE) / PANEL_H));
        m_paint.begin(r, vmax(style.scale * vmax(fit, MIN_FIT), MIN_SCALE));
        MenuPainter& p = m_paint;
        backdrop(static_cast<int>(w), static_cast<int>(h));

        const int pw = vmin(p.px(PANEL_W), static_cast<int>(w) - p.px(MARGIN) * 2);
        const int ph = vmin(p.px(PANEL_H), static_cast<int>(h) - p.px(MARGIN) * 2 - p.px(TITLE_SPACE));
        const int x0 = (static_cast<int>(w) - pw) / 2;
        const int y0 = (static_cast<int>(h) - ph + p.px(TITLE_SPACE)) / 2;
        p.text_centered(static_cast<int>(w) / 2, y0 - p.px(TITLE_SPACE) / 2 - p.px(SUBTITLE_GAP), TITLE, p.px(TITLE_SIZE), style.text, true);
        p.text_centered(static_cast<int>(w) / 2, y0 - p.px(TITLE_SPACE) / 2 + p.px(SUBTITLE_DROP), SUBTITLE, p.px(SMALL_SIZE), style.muted);
        p.rect(x0, y0, pw, ph, style.panel);

        const int pad = p.px(PAD), foot = p.px(FOOTER_H);
        const int list_x = x0 + pad, list_y = y0 + pad, list_w = pw - pad * 2, list_h = ph - pad * 2 - foot;
        draw_list(list_x, list_y, list_w, list_h, style);
        draw_buttons(x0 + pad, y0 + ph - foot, pw - pad * 2, foot, style);
    }

private:
    enum class Button : std::uint8_t { Row = 0, Play, Create, Edit, Delete, Settings, Quit };

    struct Hit {
        int         x = 0, y = 0, w = 0, h = 0;
        Button      button = Button::Row;
        std::size_t row = 0;

        bool contains(int px_, int py_) const noexcept { return px_ >= x && px_ < x + w && py_ >= y && py_ < y + h; }
    };

    static constexpr const char* TITLE         = "VOXELSPIRE";
    static constexpr const char* SUBTITLE      = "Pick a world to play, or make a new one.";
    static constexpr double      PANEL_W       = 820.0;
    static constexpr double      PANEL_H       = 560.0;
    static constexpr double      MARGIN        = 24.0;
    static constexpr double      TITLE_SPACE   = 130.0;
    static constexpr double      SUBTITLE_GAP  = 14.0;
    static constexpr double      SUBTITLE_DROP = 30.0;
    static constexpr double      MIN_FIT       = 0.55;
    static constexpr double      MIN_SCALE     = 0.4;
    static constexpr double      PAD           = 20.0;
    static constexpr double      ROW_H         = 64.0;
    static constexpr double      FOOTER_H      = 112.0;
    static constexpr double      BUTTON_H      = 38.0;
    static constexpr double      GAP           = 10.0;
    static constexpr double      ACCENT_BAR    = 4.0;
    static constexpr double      TITLE_SIZE    = 46.0;
    static constexpr double      NAME_SIZE     = 18.0;
    static constexpr double      TEXT_SIZE     = 15.0;
    static constexpr double      SMALL_SIZE    = 13.0;
    static constexpr double      LINE_LIFT     = 11.0;
    static constexpr double      LINE_DROP     = 12.0;
    static constexpr double      SCROLLBAR_W   = 5.0;
    static constexpr int         BANDS         = 48;
    static constexpr int         DIM_ALPHA     = 90;
    static constexpr int         HALF_ALPHA    = 160;
    static constexpr int         MINUTE        = 60;
    static constexpr int         HOUR          = 60 * MINUTE;
    static constexpr int         DAY           = 24 * HOUR;
    static constexpr int         WEEK          = 7 * DAY;
    static constexpr std::size_t DATE_LENGTH   = 32;

    inline static const Color SKY_TOP    { 18, 30, 54 };
    inline static const Color SKY_BOTTOM { 54, 92, 120 };

    void backdrop(int w, int h) {
        for (int i = 0; i < BANDS; ++i) {
            const double t = static_cast<double>(i) / (BANDS - 1);
            auto mix = [t](std::uint8_t a, std::uint8_t b) { return static_cast<std::uint8_t>(a + (b - a) * t); };
            const Color c(mix(SKY_TOP.red(), SKY_BOTTOM.red()), mix(SKY_TOP.green(), SKY_BOTTOM.green()), mix(SKY_TOP.blue(), SKY_BOTTOM.blue()));
            const int y = h * i / BANDS, next = h * (i + 1) / BANDS;
            m_paint.rect(0, y, w, next - y, c);
        }
    }

    bool hot(int x, int y, int w, int h) const noexcept { return m_mouse_x >= x && m_mouse_x < x + w && m_mouse_y >= y && m_mouse_y < y + h; }

    void draw_list(int x, int y, int w, int h, const MenuSettings& style) {
        MenuPainter& p = m_paint;
        m_row_h = p.px(ROW_H);
        const int total = static_cast<int>(m_worlds.size()) * m_row_h;

        if (m_reveal && selected()) {
            const int top = static_cast<int>(m_selected) * m_row_h;
            if (top < m_scroll) m_scroll = top;
            else if (top + m_row_h > m_scroll + h) m_scroll = top + m_row_h - h;
            m_reveal = false;
        }

        m_scroll = vclamp(m_scroll, 0, vmax(0, total - h));
        p.rect(x, y, w, h, MenuPainter::faded(style.sidebar, HALF_ALPHA));

        if (m_worlds.empty()) {
            p.text_centered(x + w / 2, y + h / 2 - p.px(LINE_LIFT), "No worlds yet.", p.px(NAME_SIZE), style.text, true);
            p.text_centered(x + w / 2, y + h / 2 + p.px(LINE_DROP), "Press Create new world to make your first one.", p.px(SMALL_SIZE), style.muted);
            return;
        }

        p.renderer().set_clip_rect(x, y, static_cast<unsigned int>(w), static_cast<unsigned int>(h));
        const std::int64_t now = WorldStore::now();

        for (std::size_t i = 0; i < m_worlds.size(); ++i) {
            const int ry = y - m_scroll + static_cast<int>(i) * m_row_h;
            if (ry + m_row_h < y || ry > y + h) continue;
            const WorldRecord& world = m_worlds[i];
            const bool on = i == m_selected;
            
            if (on) p.rect(x, ry, w, m_row_h, MenuPainter::faded(style.accent, DIM_ALPHA));
            else if (hot(x, ry, w, m_row_h)) p.rect(x, ry, w, m_row_h, style.row_hover);
            
            if (on) p.rect(x, ry, p.px(ACCENT_BAR), m_row_h, style.accent);
            const int tx = x + p.px(PAD);
            p.text(tx, ry + m_row_h / 2 - p.px(LINE_LIFT), world.name, p.px(NAME_SIZE), style.text, true);
            const std::string details = "Played " + ago(now, world.played) + "   -   Created " + date(world.created) + "   -   " + world.folder;
            p.text(tx, ry + m_row_h / 2 + p.px(LINE_DROP), details, p.px(SMALL_SIZE), style.muted);
            const int clip_top = vmax(ry, y), clip_bottom = vmin(ry + m_row_h, y + h);
            if (clip_bottom > clip_top) m_hits.push_back({ x, clip_top, w, clip_bottom - clip_top, Button::Row, i });
        }

        p.renderer().reset_clip_rect();

        if (total > h) {
            const int bar_h = vmax(m_row_h / 2, h * h / total);
            const int bar_y = y + (h - bar_h) * m_scroll / vmax(1, total - h);
            p.rect(x + w - p.px(SCROLLBAR_W), bar_y, p.px(SCROLLBAR_W), bar_h, MenuPainter::faded(style.muted, HALF_ALPHA));
        }
    }

    void draw_buttons(int x, int y, int w, int h, const MenuSettings& style) {
        MenuPainter& p = m_paint;
        const int gap = p.px(GAP), bh = p.px(BUTTON_H), size = p.px(TEXT_SIZE);
        const int row1 = y + (h / 2 - bh) / 2 + gap / 2, row2 = y + h / 2 + (h / 2 - bh) / 2 - gap / 2;
        const int half = (w - gap) / 2, quarter = (w - gap * 3) / 4;
        const bool any = selected() != nullptr;
        const Color white(255, 255, 255);

        auto button = [&](int bx, int by, int bw, Button which, const std::string& label, const Color& bg, const Color& fg, bool enabled) {
            p.button(bx, by, bw, bh, label, enabled ? bg : MenuPainter::faded(bg, DIM_ALPHA), enabled ? fg : style.muted, enabled && hot(bx, by, bw, bh), size);
            if (enabled) m_hits.push_back({ bx, by, bw, bh, which, 0 });
        };

        button(x, row1, half, Button::Play, any ? "Play " + selected()->name : std::string("Play"), style.accent, white, any);
        button(x + half + gap, row1, half, Button::Create, "Create new world", style.control, style.text, true);
        button(x, row2, quarter, Button::Edit, "Edit world", style.control, style.text, any);
        button(x + (quarter + gap), row2, quarter, Button::Delete, m_confirm > 0.0 ? "Really delete?" : "Delete world", m_confirm > 0.0 ? style.danger : style.control, style.text, any);
        button(x + (quarter + gap) * 2, row2, quarter, Button::Settings, "Settings", style.control, style.text, true);
        button(x + (quarter + gap) * 3, row2, quarter, Button::Quit, "Quit", style.control, style.text, true);

        if (m_status_time > 0.0 && !m_status.empty())
            p.text_centered(x + w / 2, y + h + p.px(PAD) / 2, m_status, p.px(SMALL_SIZE), m_status_problem ? style.error : style.muted);
    }

    void click(int x, int y, bool twice) {
        m_mouse_x = x;
        m_mouse_y = y;

        for (auto it = m_hits.rbegin(); it != m_hits.rend(); ++it) {
            if (!it->contains(x, y)) continue;

            switch (it->button) {
                case Button::Row:
                    if (it->row != m_selected) m_confirm = 0.0;
                    m_selected = it->row;
                    if (twice) m_action = StartAction::Play;
                    break;
                case Button::Play:     m_action = StartAction::Play; break;
                case Button::Create:   m_action = StartAction::Create; break;
                case Button::Edit:     m_action = StartAction::Edit; break;
                case Button::Delete:   press_delete(); break;
                case Button::Settings: m_action = StartAction::Settings; break;
                case Button::Quit:     m_action = StartAction::Quit; break;
            }

            return;
        }
    }

    void press_delete() {
        if (!selected()) return;
        if (m_confirm > 0.0) { m_confirm = 0.0; m_action = StartAction::Delete; return; }
        m_confirm = CONFIRM_SECONDS;
    }

    static std::string ago(std::int64_t now, std::int64_t then) {
        if (then <= 0) return "never";
        const std::int64_t s = vmax<std::int64_t>(0, now - then);
        if (s < MINUTE) return "just now";
        if (s < HOUR) return plural(s / MINUTE, "minute") + " ago";
        if (s < DAY) return plural(s / HOUR, "hour") + " ago";
        if (s < WEEK) return plural(s / DAY, "day") + " ago";
        return "on " + date(then);
    }

    static std::string plural(std::int64_t n, const char* unit) { return std::to_string(n) + " " + unit + (n == 1 ? "" : "s"); }

    static std::string date(std::int64_t when) {
        if (when <= 0) return "unknown";
        const std::time_t t = static_cast<std::time_t>(when);
        std::tm parts{};
    #ifdef OS_WINDOWS
        localtime_s(&parts, &t);
    #elif defined(OS_LINUX)
        localtime_r(&t, &parts);
    #endif
        char buf[DATE_LENGTH];
        std::strftime(buf, sizeof(buf), "%b %d, %Y", &parts);
        return buf;
    }

    std::vector<WorldRecord> m_worlds;
    std::vector<Hit>         m_hits;
    MenuPainter              m_paint;
    std::size_t              m_selected       = 0;
    StartAction              m_action         = StartAction::None;
    int                      m_scroll         = 0;
    int                      m_scroll_steps   = 0;
    int                      m_row_h          = 1;
    int                      m_mouse_x        = 0;
    int                      m_mouse_y        = 0;
    bool                     m_reveal         = false;
    double                   m_confirm        = 0.0;
    std::string              m_status;
    bool                     m_status_problem = false;
    double                   m_status_time    = 0.0;
};

} // namespace voxelspire

#endif // VOXELSPIRE_UI_START_MENU_HPP