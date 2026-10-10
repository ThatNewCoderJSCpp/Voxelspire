#ifndef VOXELSPIRE_UI_SETTINGS_MENU_HPP
#define VOXELSPIRE_UI_SETTINGS_MENU_HPP

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>
#include "color_math.hpp"
#include "color_picker.hpp"
#include "menu_painter.hpp"
#include "settings_model.hpp"
#include "text_field.hpp"

namespace voxelspire {

class SettingsMenu {
public:
    static constexpr unsigned int LEFT_MOUSE    = 1;
    static constexpr double       SHIFT_STEPS   = 10.0;
    static constexpr double       FINE_DRAG     = 0.1;
    static constexpr int          SCROLL_ROWS   = 3;
    static constexpr double       ERROR_SECONDS = 6.0;
    static constexpr double       BLINK_SECONDS = 0.5;
    static constexpr int          DRAG_START_PX = 3;

    void set_tabs(std::vector<SettingsTab> tabs);

    std::vector<SettingsTab>&       tabs()       noexcept { return m_tabs; }
    const std::vector<SettingsTab>& tabs() const noexcept { return m_tabs; }

    bool is_open()   const noexcept { return m_open; }
    bool capturing() const noexcept { return m_capture >= 0; }
    bool editing()   const noexcept { return m_field.active(); }

    void open() { m_open = true; m_capture = -1; m_drag = {}; m_close_requested = false; m_cancel_requested = false; }

    void close() {
        stop_editing(true);
        m_picker.close();
        m_open = false;
        m_capture = -1;
        m_drag = {};
    }

    bool take_close_request() noexcept { const bool r = m_close_requested; m_close_requested = false; return r; }
    bool take_cancel_request() noexcept { const bool r = m_cancel_requested; m_cancel_requested = false; return r; }

    void set_title(std::string title) { m_title = std::move(title); }
    void set_hint(std::string hint) { m_hint = std::move(hint); }

    void set_actions(std::string done, std::string cancel = std::string()) {
        m_done_label   = std::move(done);
        m_cancel_label = std::move(cancel);
    }

    void select_tab(std::size_t index) noexcept { m_tab = m_tabs.empty() ? 0 : vmin(index, m_tabs.size() - 1); m_scroll = 0; }
    std::uint32_t take_changes() noexcept { const std::uint32_t c = m_changes; m_changes = Apply::Nothing; return c; }

    void set_save_note(const char* note) noexcept { m_save_note = note; }

    void report_problems(const std::vector<std::string>& problems);

    bool on_event(const fizmo::windows::WindowEvent& e);

    void update(const fizmo::windows::InputManager& input, double dt);

    void render(fizmo::windows::Renderer& r, unsigned int w, unsigned int h, const MenuSettings& style);

private:
    enum class Target : std::uint8_t { None = 0, Number, Hex, Channel, PickerHex, PickerChannel, Text };

    struct Row { int y = 0, h = 0, control = -1; };
    struct Span { int x = 0, w = 1; };

    struct Drag {
        bool    active = false;
        HitKind kind = HitKind::Slider;
        int     control = -1;
        int     part = 0;
        int     x = 0, w = 1;
        int     press_x = 0, press_y = 0;
        int     anchor_x = 0;
        double  anchor_t = 0.0;
        bool    fine = false;
        bool    moved = false;
    };

    struct Edit {
        Target target = Target::None;
        int    control = -1;
        int    part = 0;
    };

    struct Problem {
        std::string message;
        int         control = -1;
        Target      target = Target::None;
        int         part = 0;
        double      seconds = 0.0;
    };

    static constexpr double      PANEL_W        = 1120.0;
    static constexpr double      PANEL_H        = 720.0;
    static constexpr double      MARGIN         = 20.0;
    static constexpr double      MIN_FIT        = 0.55;
    static constexpr double      MIN_SCALE      = 0.4;
    static constexpr double      SIDEBAR_W      = 220.0;
    static constexpr double      HEADER_H       = 64.0;
    static constexpr double      FOOTER_H       = 70.0;
    static constexpr double      PAD            = 20.0;
    static constexpr double      ROW_H          = 38.0;
    static constexpr double      HEADER_ROW_H   = 46.0;
    static constexpr double      TAB_H          = 36.0;
    static constexpr double      GROUP_H        = 30.0;
    static constexpr double      GROUP_SIZE     = 12.0;
    static constexpr double      GROUP_DROP     = 3.0;
    static constexpr double      GROUP_INDENT   = 8.0;
    static constexpr double      TEXT_SIZE      = 15.0;
    static constexpr double      SMALL_SIZE     = 13.0;
    static constexpr double      TITLE_SIZE     = 22.0;
    static constexpr double      TAB_TITLE_SIZE = 18.0;
    static constexpr double      SECTION_SIZE   = 14.0;
    static constexpr double      SUMMARY_LIFT   = 10.0;
    static constexpr double      SUMMARY_DROP   = 12.0;
    static constexpr double      CONTROL_W      = 420.0;
    static constexpr double      RESET_W        = 58.0;
    static constexpr double      BOX            = 22.0;
    static constexpr double      SMALL_BUTTON   = 26.0;
    static constexpr double      VALUE_W        = 116.0;
    static constexpr int         ALT_PART       = 1;
    static constexpr double      HEX_W          = 96.0;
    static constexpr double      TRACK_H        = 6.0;
    static constexpr double      KNOB           = 14.0;
    static constexpr double      KEY_BUTTON_W   = 70.0;
    static constexpr double      BUTTON_W       = 110.0;
    static constexpr double      BUTTON_H       = 30.0;
    static constexpr double      GAP            = 6.0;
    static constexpr double      SCROLLBAR_W    = 5.0;
    static constexpr double      ACCENT_BAR     = 4.0;
    static constexpr double      CHECK_INSET    = 5.0;
    static constexpr double      CHECK_STROKE   = 2.0;
    static constexpr double      BUBBLE_PAD     = 8.0;
    static constexpr double      BUBBLE_GAP     = 4.0;
    static constexpr int         DIM_ALPHA      = 90;
    static constexpr int         HALF_ALPHA     = 160;
    static constexpr int         BUBBLE_ALPHA   = 245;
    static constexpr int         CHANNEL_MAX    = 255;
    static constexpr std::size_t NUMBER_LENGTH  = 16;
    static constexpr std::size_t CHANNEL_LENGTH = 3;
    static constexpr std::size_t HEX_LENGTH     = 9;
    static constexpr std::size_t TEXT_LENGTH    = 64;
    static constexpr const char* CANCEL_KEY     = "Escape";

    void click(int x, int y);

    bool caret_on() const noexcept { return std::fmod(m_clock, BLINK_SECONDS * 2.0) < BLINK_SECONDS; }

    bool hot(int x, int y, int w, int h) const noexcept { return m_mouse_x >= x && m_mouse_x < x + w && m_mouse_y >= y && m_mouse_y < y + h; }

    void add_hit(int x, int y, int w, int h, HitKind kind, int control = -1, int part = 0) { m_hits.push_back({ x, y, w, h, kind, control, part }); }

    const Hit* hit_at(int x, int y) const noexcept;

    int row_at(int x, int y) const noexcept;

    SettingControl* control(int index);

    void changed(const SettingControl& c) { m_changes |= c.apply; }

    bool error_on(int index, Target target, int part = 0) const noexcept;

    bool editing(int index, Target target, int part = 0) const noexcept;

    FieldLook look(const MenuSettings& style, bool on) const;

    bool group_starts(std::size_t i) const noexcept {
        return !m_tabs[i].group.empty() && (i == 0 || m_tabs[i - 1].group != m_tabs[i].group);
    }

    bool tab_shown(std::size_t i) const {
        return i == m_tab || m_tabs[i].group.empty() || !m_closed_groups.count(m_tabs[i].group);
    }

    void draw_tabs(int x, int y, int w, int h, const MenuSettings& style);

    int row_height(const SettingControl& c) const noexcept { return c.kind == ControlKind::Header ? m_paint.px(HEADER_ROW_H) : m_row_h; }

    void draw_content(int x, int y, int w, int h, const MenuSettings& style);

    static std::string upper(std::string s);

    void draw_control(SettingControl& c, int index, int x, int y, int w, int h, bool on, const MenuSettings& style);

    void draw_check(int x, int y, int size, const Color& c);

    void draw_error_bubble(int px0, int py0, int pw, int ph, const MenuSettings& style);

    void draw_footer(int x, int y, int w, int h, const MenuSettings& style);

    static std::string hover_help(const SettingControl& c);

    std::string typing_help(const SettingControl* c) const;

    static double fraction(const SettingControl& c, double v);

    static double from_fraction(const SettingControl& c, double t);

    static double snapped(const SettingControl& c, double v);

    void set_number(SettingControl& c, double v);

    void set_color(SettingControl& c, const Color& v);

    void nudge(int index, double direction, bool shift);

    void begin_drag(const Hit& h, int x, int w);

    void drag_to(int mx, int my);

    void release_drag();

    void begin_edit(Target target, int index, int part);

    bool same_target(const Hit& h) const noexcept;

    void stop_editing(bool apply);

    void commit(const Edit& edit, const std::string& text);

    void fail(SettingControl& c, const Edit& edit, const std::string& message);

    void text_key(const std::string& key);

    void move_edit(const Edit& from, int direction);

    void reveal(int index);

    void cancel_picker();

    void press(const Hit& h);

    void finish_capture(const std::string& key);

    struct Box { int x = 0, y = 0, w = 0, h = 0; };

    std::vector<SettingsTab>      m_tabs;
    std::vector<Hit>              m_hits;
    std::vector<Row>              m_rows;
    std::vector<int>              m_offsets;
    std::unordered_map<int, Span> m_track;
    std::unordered_map<int, Span> m_channel;
    std::unordered_map<int, Box>  m_boxes;
    MenuPainter                   m_paint;
    ColorPicker                   m_picker;
    TextField                     m_field;
    Edit                          m_edit;
    Problem                       m_error;
    Drag                          m_drag;
    std::size_t                   m_tab = 0;
    int                           m_tab_scroll = 0;
    Box                           m_side;
    std::set<std::string>         m_closed_groups;
    bool                          m_open = false;
    bool                          m_close_requested = false;
    bool                          m_cancel_requested = false;
    std::string                   m_title = "Settings";
    std::string                   m_hint;
    std::string                   m_done_label = "Done";
    std::string                   m_cancel_label;
    int                           m_capture = -1;
    bool                          m_capture_add = false;
    bool                          m_click_seen = false;
    bool                          m_shift = false;
    bool                          m_control = false;
    int                           m_scroll = 0;
    int                           m_scroll_steps = 0;
    int                           m_hover_control = -1;
    int                           m_mouse_x = 0, m_mouse_y = 0;
    int                           m_row_h = 1;
    int                           m_content_x = 0, m_content_y = 0, m_content_w = 0, m_content_h = 0;
    double                        m_clock = 0.0;
    std::uint32_t                 m_changes = Apply::Nothing;
    const char*                   m_save_note = ".";
};

} // namespace voxelspire

#endif // VOXELSPIRE_UI_SETTINGS_MENU_HPP