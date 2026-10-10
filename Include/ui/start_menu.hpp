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

    void set_worlds(std::vector<WorldRecord> worlds);

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

    bool on_event(const fizmo::windows::WindowEvent& e);

    void update(const fizmo::windows::InputManager& input, double dt);

    void render(fizmo::windows::Renderer& r, unsigned int w, unsigned int h, const MenuSettings& style);

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

    void backdrop(int w, int h);

    bool hot(int x, int y, int w, int h) const noexcept { return m_mouse_x >= x && m_mouse_x < x + w && m_mouse_y >= y && m_mouse_y < y + h; }

    void draw_list(int x, int y, int w, int h, const MenuSettings& style);

    void draw_buttons(int x, int y, int w, int h, const MenuSettings& style);

    void click(int x, int y, bool twice);

    void press_delete();

    static std::string ago(std::int64_t now, std::int64_t then);

    static std::string plural(std::int64_t n, const char* unit) { return std::to_string(n) + " " + unit + (n == 1 ? "" : "s"); }

    static std::string date(std::int64_t when);

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