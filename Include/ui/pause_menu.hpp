#ifndef VOXELSPIRE_UI_PAUSE_MENU_HPP
#define VOXELSPIRE_UI_PAUSE_MENU_HPP

#include <array>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>
#include "../core/settings/menu.hpp"
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

    bool on_event(const fizmo::windows::WindowEvent& e);

    void update(const fizmo::windows::InputManager& input, double dt);

    void render(fizmo::windows::Renderer& r, unsigned int w, unsigned int h, const MenuSettings& style);

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

    void click(int x, int y);

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