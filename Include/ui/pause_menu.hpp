#ifndef VOXELSPIRE_UI_PAUSE_MENU_HPP
#define VOXELSPIRE_UI_PAUSE_MENU_HPP

#include <array>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>
#include "menu_painter.hpp"

namespace voxelspire {

enum class PauseAction : std::uint8_t { None = 0, Resume, Options, SaveAndExit };

class PauseMenu {
public:
    static constexpr unsigned int LEFT_MOUSE     = 1;
    static constexpr double       STATUS_SECONDS = 4.0;

    void open() noexcept { m_open = true; m_focus = 0; m_action = PauseAction::None; }
    void close() noexcept { m_open = false; }
    bool is_open() const noexcept { return m_open; }

    void set_subtitle(std::string text) { m_subtitle = std::move(text); }

    void set_status(std::string message, bool problem = false) {
        m_status = std::move(message);
        m_status_problem = problem;
        m_status_time = STATUS_SECONDS;
    }

    PauseAction take_action() noexcept { const PauseAction a = m_action; m_action = PauseAction::None; return a; }

    bool on_event(const fizmo::windows::WindowEvent& e) {
        using fizmo::windows::WindowEventType;
        if (!m_open) return false;

        if (e.button == LEFT_MOUSE && e.type == WindowEventType::MouseClick) {
            click(static_cast<int>(e.x), static_cast<int>(e.y));
            return true;
        }

        if (e.type != WindowEventType::KeyPress) return false;
        const int count = static_cast<int>(ENTRIES.size());
        if (e.key_name == "UpArrow")   { m_focus = (m_focus + count - 1) % count; m_keyboard = true; return true; }
        if (e.key_name == "DownArrow") { m_focus = (m_focus + 1) % count; m_keyboard = true; return true; }
        if (e.key_name == "Enter")     { m_action = ENTRIES[static_cast<std::size_t>(m_focus)].action; return true; }
        return false;
    }

    void update(const fizmo::windows::InputManager& input, double dt) {
        if (input.mouse_x() != m_mouse_x || input.mouse_y() != m_mouse_y) m_keyboard = false;
        m_mouse_x = input.mouse_x();
        m_mouse_y = input.mouse_y();
        if (m_status_time > 0.0) m_status_time -= dt;
    }

    void render(fizmo::windows::Renderer& r, unsigned int w, unsigned int h, const MenuSettings& style) {
        m_hits.clear();
        if (!m_open) return;
        m_paint.begin(r, vmax(style.scale, MIN_SCALE));
        MenuPainter& p = m_paint;
        const int sw = static_cast<int>(w), sh = static_cast<int>(h);
        p.rect(0, 0, sw, sh, style.backdrop);

        const int count = static_cast<int>(ENTRIES.size());
        const int pad = p.px(PAD), bh = p.px(BUTTON_H), gap = p.px(GAP), head = p.px(HEADER_H);
        const int pw = vmin(p.px(PANEL_W), sw - p.px(MARGIN) * 2);
        const int ph = head + count * bh + (count - 1) * gap + pad * 2;
        const int x0 = (sw - pw) / 2, y0 = (sh - ph) / 2;
        p.rect(x0, y0, pw, ph, style.panel);
        p.rect(x0, y0, pw, p.px(ACCENT_BAR), style.accent);
        p.text_centered(sw / 2, y0 + pad + p.px(TITLE_DROP), TITLE, p.px(TITLE_SIZE), style.text, true);
        if (!m_subtitle.empty()) p.text_centered(sw / 2, y0 + pad + p.px(SUBTITLE_DROP), m_subtitle, p.px(SMALL_SIZE), style.muted);

        const Color white(255, 255, 255);
        int by = y0 + pad + head;

        for (int i = 0; i < count; ++i) {
            const Entry& entry = ENTRIES[static_cast<std::size_t>(i)];
            const int bx = x0 + pad, bw = pw - pad * 2;
            const bool hot = m_keyboard ? i == m_focus : inside(bx, by, bw, bh);
            const bool primary = entry.action == PauseAction::Resume;
            p.button(bx, by, bw, bh, entry.label, primary ? style.accent : style.control, primary ? white : style.text, hot, p.px(TEXT_SIZE));
            m_hits.push_back({ bx, by, bw, bh, entry.action });
            by += bh + gap;
        }

        if (m_status_time > 0.0 && !m_status.empty())
            p.text_centered(sw / 2, y0 + ph + p.px(STATUS_DROP), m_status, p.px(SMALL_SIZE), m_status_problem ? style.error : style.muted);
    }

private:
    struct Entry {
        PauseAction action;
        const char* label;
    };

    struct Hit {
        int         x = 0, y = 0, w = 0, h = 0;
        PauseAction action = PauseAction::None;
    };

    static constexpr std::array<Entry, 3> ENTRIES{ {
        { PauseAction::Resume,      "Resume" },
        { PauseAction::Options,     "Options" },
        { PauseAction::SaveAndExit, "Save and Exit" }
    } };

    static constexpr const char* TITLE         = "Paused";
    static constexpr double      PANEL_W       = 380.0;
    static constexpr double      MARGIN        = 24.0;
    static constexpr double      PAD           = 24.0;
    static constexpr double      HEADER_H      = 84.0;
    static constexpr double      BUTTON_H      = 44.0;
    static constexpr double      GAP           = 12.0;
    static constexpr double      ACCENT_BAR    = 4.0;
    static constexpr double      TITLE_DROP    = 18.0;
    static constexpr double      SUBTITLE_DROP = 50.0;
    static constexpr double      STATUS_DROP   = 22.0;
    static constexpr double      TITLE_SIZE    = 30.0;
    static constexpr double      TEXT_SIZE     = 17.0;
    static constexpr double      SMALL_SIZE    = 13.0;
    static constexpr double      MIN_SCALE     = 0.4;

    bool inside(int x, int y, int w, int h) const noexcept { return m_mouse_x >= x && m_mouse_x < x + w && m_mouse_y >= y && m_mouse_y < y + h; }

    void click(int x, int y) {
        m_mouse_x = x;
        m_mouse_y = y;
        m_keyboard = false;

        for (const Hit& hit : m_hits) {
            if (x < hit.x || x >= hit.x + hit.w || y < hit.y || y >= hit.y + hit.h) continue;
            m_action = hit.action;
            return;
        }
    }

    std::vector<Hit> m_hits;
    MenuPainter      m_paint;
    PauseAction      m_action         = PauseAction::None;
    std::string      m_subtitle;
    std::string      m_status;
    bool             m_status_problem = false;
    double           m_status_time    = 0.0;
    bool             m_open           = false;
    bool             m_keyboard       = false;
    int              m_focus          = 0;
    int              m_mouse_x        = 0;
    int              m_mouse_y        = 0;
};

} // namespace voxelspire

#endif // VOXELSPIRE_UI_PAUSE_MENU_HPP