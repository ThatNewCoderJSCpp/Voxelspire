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

    void set_tabs(std::vector<SettingsTab> tabs) {
        stop_editing(false);
        m_picker.close();
        m_tabs = std::move(tabs);
        m_tab = vmin(m_tab, m_tabs.empty() ? std::size_t(0) : m_tabs.size() - 1);
    }

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

    void report_problems(const std::vector<std::string>& problems) {
        if (problems.empty()) return;
        const std::string count = std::to_string(problems.size());
        m_error = { (problems.size() == 1 ? std::string("A saved setting was invalid and was reset: ") : count + " saved settings were invalid and were reset. First: ") + problems.front(),
                    -1, Target::None, 0, ERROR_SECONDS * 2.0 };
    }

    bool on_event(const fizmo::windows::WindowEvent& e) {
        using fizmo::windows::WindowEventType;
        if (!m_open) return false;

        if (e.type == WindowEventType::MouseScroll) {
            m_scroll_steps += e.scroll_delta;
            return true;
        }

        if (e.button == LEFT_MOUSE && (e.type == WindowEventType::MouseClick || (e.type == WindowEventType::MouseDoubleClick && !m_click_seen))) {
            m_click_seen = true;
            click(static_cast<int>(e.x), static_cast<int>(e.y));
            return true;
        }

        if (e.type == WindowEventType::MouseMove && m_drag.active) {
            m_mouse_x = static_cast<int>(e.x);
            m_mouse_y = static_cast<int>(e.y);
            drag_to(m_mouse_x, m_mouse_y);
            return true;
        }

        if (e.type == WindowEventType::MouseRelease && e.button == LEFT_MOUSE) {
            m_click_seen = false;
            if (!m_drag.active) return false;
            drag_to(static_cast<int>(e.x), static_cast<int>(e.y));
            release_drag();
            return true;
        }

        if (e.type == WindowEventType::KeyPress || e.type == WindowEventType::KeyRelease) {
            const bool down = e.type == WindowEventType::KeyPress;
            if (e.key_name == "LeftShift" || e.key_name == "RightShift") m_shift = down;
            if (e.key_name == "LeftControl" || e.key_name == "RightControl") m_control = down;
        }

        if (e.type != WindowEventType::KeyPress) return false;

        if (capturing()) {
            finish_capture(e.key_name);
            return true;
        }

        if (m_field.active()) {
            text_key(e.key_name);
            return true;
        }

        if (m_picker.is_open()) {
            if (e.key_name == "Enter") m_picker.close();
            else if (e.key_name == "Escape") cancel_picker();
            return true;
        }

        if (e.key_name == "LeftArrow" || e.key_name == "RightArrow") {
            SettingControl* c = control(m_hover_control);
            if (!c || !c->is_number() || !c->enabled()) return false;
            nudge(m_hover_control, e.key_name == "RightArrow" ? 1.0 : -1.0, m_shift);
            return true;
        }

        return false;
    }

    void update(const fizmo::windows::InputManager& input, double dt) {
        if (!m_open) return;
        m_clock += dt;
        if (m_error.seconds > 0.0) m_error.seconds -= dt;
        const int mx = input.mouse_x(), my = input.mouse_y();
        m_mouse_x = mx;
        m_mouse_y = my;
        m_shift   = input.is_key_down("LeftShift") || input.is_key_down("RightShift");
        m_control = input.is_key_down("LeftControl") || input.is_key_down("RightControl");
        const Hit* hover = hit_at(mx, my);
        m_hover_control = m_picker.is_open() ? -1 : (hover && hover->control >= 0 ? hover->control : row_at(mx, my));

        if (m_scroll_steps != 0) {
            if (hot(m_side.x, m_side.y, m_side.w, m_side.h)) m_tab_scroll -= m_scroll_steps * SCROLL_ROWS * m_paint.px(TAB_H);
            else if (!m_picker.is_open()) m_scroll -= m_scroll_steps * SCROLL_ROWS * m_row_h;
            m_scroll_steps = 0;
        }

        if (m_drag.active && !input.is_mouse_button_down(LEFT_MOUSE)) release_drag();
    }

    void render(fizmo::windows::Renderer& r, unsigned int w, unsigned int h, const MenuSettings& style) {
        if (!m_open || m_tabs.empty()) return;
        m_hits.clear();
        m_rows.clear();
        const double fit = vmin(1.0, vmin((w - MARGIN * 2.0) / PANEL_W, (h - MARGIN * 2.0) / PANEL_H));
        m_paint.begin(r, vmax(style.scale * vmax(fit, MIN_FIT), MIN_SCALE));
        MenuPainter& p = m_paint;
        m_row_h = p.px(ROW_H);
        const int pw = vmin(p.px(PANEL_W), static_cast<int>(w) - p.px(MARGIN) * 2);
        const int ph = vmin(p.px(PANEL_H), static_cast<int>(h) - p.px(MARGIN) * 2);
        const int x0 = (static_cast<int>(w) - pw) / 2, y0 = (static_cast<int>(h) - ph) / 2;
        const int side = p.px(SIDEBAR_W), head = p.px(HEADER_H), foot = p.px(FOOTER_H);

        p.rect(0, 0, static_cast<int>(w), static_cast<int>(h), style.backdrop);
        p.rect(x0, y0, pw, ph, style.panel);
        p.rect(x0, y0, side, ph, style.sidebar);
        p.text(x0 + p.px(PAD), y0 + head / 2, m_title, p.px(TITLE_SIZE), style.text, true);
        draw_tabs(x0, y0 + head, side, ph - head, style);

        const SettingsTab& tab = m_tabs[m_tab];
        const int cx = x0 + side + p.px(PAD), cw = pw - side - p.px(PAD) * 2;
        p.text(cx, y0 + head / 2 - p.px(SUMMARY_LIFT), tab.name, p.px(TAB_TITLE_SIZE), style.text, true);
        p.text(cx, y0 + head / 2 + p.px(SUMMARY_DROP), tab.summary, p.px(SMALL_SIZE), style.muted);
        const int top = y0 + head, bottom = y0 + ph - foot;
        draw_content(cx, top, cw, bottom - top, style);
        draw_footer(x0 + side, bottom, pw - side, foot, style);
        draw_error_bubble(x0, y0, pw, ph, style);

        if (SettingControl* c = control(m_picker.control())) {
            PickerEdit edit;
            edit.field       = m_field.active() && (m_edit.target == Target::PickerHex || m_edit.target == Target::PickerChannel) ? &m_field : nullptr;
            edit.part        = !edit.field ? PickerEdit::NONE : (m_edit.target == Target::PickerHex ? ColorPicker::HEX_PART : m_edit.part);
            edit.caret_on    = caret_on();
            edit.failed_part = error_on(m_picker.control(), Target::PickerHex) ? ColorPicker::HEX_PART
                             : (m_error.seconds > 0.0 && m_error.target == Target::PickerChannel && m_error.control == m_picker.control() ? m_error.part : PickerEdit::NONE);
            m_picker.render(p, style, x0 + side, top, pw - side, bottom - top, *c, edit, m_hits, m_mouse_x, m_mouse_y);
        }
    }

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

    void click(int x, int y) {
        m_mouse_x = x;
        m_mouse_y = y;
        if (m_drag.active) return;
        if (capturing()) { m_capture = -1; return; }

        if (m_picker.is_open() && !m_picker.contains(x, y)) {
            stop_editing(true);
            m_picker.close();
            return;
        }

        const Hit* hover = hit_at(x, y);
        if (hover && m_field.active() && same_target(*hover)) return;
        stop_editing(true);
        hover = hit_at(x, y);
        if (hover) press(*hover);
    }

    bool caret_on() const noexcept { return std::fmod(m_clock, BLINK_SECONDS * 2.0) < BLINK_SECONDS; }

    bool hot(int x, int y, int w, int h) const noexcept { return m_mouse_x >= x && m_mouse_x < x + w && m_mouse_y >= y && m_mouse_y < y + h; }

    void add_hit(int x, int y, int w, int h, HitKind kind, int control = -1, int part = 0) { m_hits.push_back({ x, y, w, h, kind, control, part }); }

    const Hit* hit_at(int x, int y) const noexcept {
        for (auto it = m_hits.rbegin(); it != m_hits.rend(); ++it) if (it->contains(x, y)) return &*it;
        return nullptr;
    }

    int row_at(int x, int y) const noexcept {
        if (x < m_content_x || x >= m_content_x + m_content_w || y < m_content_y || y >= m_content_y + m_content_h) return -1;
        for (const Row& row : m_rows) if (y >= row.y && y < row.y + row.h) return row.control;
        return -1;
    }

    SettingControl* control(int index) {
        if (m_tabs.empty() || index < 0) return nullptr;
        auto& list = m_tabs[m_tab].controls;
        return static_cast<std::size_t>(index) < list.size() ? &list[static_cast<std::size_t>(index)] : nullptr;
    }

    void changed(const SettingControl& c) { m_changes |= c.apply; }

    bool error_on(int index, Target target, int part = 0) const noexcept {
        const bool by_part = target == Target::Channel || target == Target::Number;
        return m_error.seconds > 0.0 && m_error.control == index && m_error.target == target && (!by_part || m_error.part == part);
    }

    bool editing(int index, Target target, int part = 0) const noexcept {
        return m_field.active() && m_edit.control == index && m_edit.target == target && m_edit.part == part;
    }

    FieldLook look(const MenuSettings& style, bool on) const {
        FieldLook l;
        l.fill   = on ? style.control : MenuPainter::faded(style.control, HALF_ALPHA);
        l.text   = on ? style.text : MenuPainter::faded(style.muted, HALF_ALPHA);
        l.muted  = style.muted;
        l.accent = style.accent;
        l.error  = style.error;
        l.size   = m_paint.px(SMALL_SIZE);
        return l;
    }

    bool group_starts(std::size_t i) const noexcept {
        return !m_tabs[i].group.empty() && (i == 0 || m_tabs[i - 1].group != m_tabs[i].group);
    }

    bool tab_shown(std::size_t i) const {
        return i == m_tab || m_tabs[i].group.empty() || !m_closed_groups.count(m_tabs[i].group);
    }

    void draw_tabs(int x, int y, int w, int h, const MenuSettings& style) {
        MenuPainter& p = m_paint;
        const int th = p.px(TAB_H), gh = p.px(GROUP_H);
        m_side = { x, y, w, h };
        int total = 0;

        for (std::size_t i = 0; i < m_tabs.size(); ++i) {
            if (group_starts(i)) total += gh;
            if (tab_shown(i)) total += th;
        }

        m_tab_scroll = vclamp(m_tab_scroll, 0, vmax(0, total - h));
        p.renderer().set_clip_rect(x, y, static_cast<unsigned int>(w), static_cast<unsigned int>(vmax(h, 0)));
        const std::size_t hits_before = m_hits.size();
        int ty = y - m_tab_scroll;

        for (std::size_t i = 0; i < m_tabs.size(); ++i) {
            if (group_starts(i)) {
                const bool closed = m_closed_groups.count(m_tabs[i].group) > 0;
                if (hot(x, ty, w, gh)) p.rect(x, ty, w, gh, style.row_hover);
                p.text(x + p.px(PAD), ty + gh / 2 + p.px(GROUP_DROP), upper(m_tabs[i].group), p.px(GROUP_SIZE), style.accent, true);
                p.text(x + w - p.px(PAD), ty + gh / 2 + p.px(GROUP_DROP), closed ? "+" : "-", p.px(GROUP_SIZE), style.muted, true);
                add_hit(x, ty, w, gh, HitKind::Group, -1, static_cast<int>(i));
                ty += gh;
            }

            if (!tab_shown(i)) continue;
            const bool selected = i == m_tab;

            if (selected) {
                p.rect(x, ty, w, th, style.panel);
                p.rect(x, ty, p.px(ACCENT_BAR), th, style.accent);
            } else if (hot(x, ty, w, th)) {
                p.rect(x, ty, w, th, style.row_hover);
            }

            const int indent = m_tabs[i].group.empty() ? 0 : p.px(GROUP_INDENT);
            p.text(x + p.px(PAD) + indent, ty + th / 2, m_tabs[i].name, p.px(TEXT_SIZE), selected ? style.text : style.muted, selected);
            add_hit(x, ty, w, th, HitKind::Tab, -1, static_cast<int>(i));
            ty += th;
        }

        for (std::size_t i = hits_before; i < m_hits.size(); ++i) {
            Hit& hit = m_hits[i];
            const int from = vmax(hit.y, y), to = vmin(hit.y + hit.h, y + h);
            hit.h = vmax(0, to - from);
            hit.y = from;
        }

        p.renderer().reset_clip_rect();

        if (total > h) {
            const int bar_h = vmax(th, h * h / total);
            const int bar_y = y + (h - bar_h) * m_tab_scroll / vmax(1, total - h);
            p.rect(x + w - p.px(SCROLLBAR_W), bar_y, p.px(SCROLLBAR_W), bar_h, MenuPainter::faded(style.muted, DIM_ALPHA));
        }
    }

    int row_height(const SettingControl& c) const noexcept { return c.kind == ControlKind::Header ? m_paint.px(HEADER_ROW_H) : m_row_h; }

    void draw_content(int x, int y, int w, int h, const MenuSettings& style) {
        MenuPainter& p = m_paint;
        SettingsTab& tab = m_tabs[m_tab];
        int total = 0;
        m_offsets.assign(tab.controls.size(), 0);

        for (std::size_t i = 0; i < tab.controls.size(); ++i) {
            m_offsets[i] = total;
            total += row_height(tab.controls[i]);
        }

        m_scroll = vclamp(m_scroll, 0, vmax(0, total - h));
        m_content_x = x;
        m_content_y = y;
        m_content_w = w;
        m_content_h = h;
        p.renderer().set_clip_rect(x, y, static_cast<unsigned int>(w), static_cast<unsigned int>(h));
        const int control_w = vmin(p.px(CONTROL_W), w / 2);
        const int reset_w = p.px(RESET_W);
        const int control_x = x + w - control_w - reset_w - p.px(GAP) * 2;
        const std::size_t hits_before = m_hits.size();

        for (std::size_t i = 0; i < tab.controls.size(); ++i) {
            SettingControl& c = tab.controls[i];
            const int rh = row_height(c);
            const int cy = y - m_scroll + m_offsets[i];
            const int index = static_cast<int>(i);
            if (cy + rh < y || cy > y + h) continue;

            if (c.kind == ControlKind::Header) {
                p.text(x, cy + rh / 2 + p.px(GAP), upper(c.label), p.px(SECTION_SIZE), style.accent, true);
                continue;
            }

            const bool on = c.enabled();
            if (index == m_hover_control) p.rect(x, cy, w, rh, style.row_hover);
            p.text(x + p.px(GAP), cy + rh / 2, c.label, p.px(TEXT_SIZE), on ? style.text : MenuPainter::faded(style.muted, HALF_ALPHA));
            m_rows.push_back({ cy, rh, index });
            draw_control(c, index, control_x, cy, control_w, rh, on, style);

            if (c.reset && c.is_default && !c.is_default()) {
                const int bx = control_x + control_w + p.px(GAP);
                const int bh = p.px(BUTTON_H) * 3 / 4;
                p.button(bx, cy + (rh - bh) / 2, reset_w, bh, "Reset", style.control, style.muted, hot(bx, cy + (rh - bh) / 2, reset_w, bh), p.px(SMALL_SIZE));
                add_hit(bx, cy + (rh - bh) / 2, reset_w, bh, HitKind::Reset, index);
            }
        }

        for (std::size_t i = hits_before; i < m_hits.size(); ++i) {
            Hit& hit = m_hits[i];
            const int from = vmax(hit.y, y), to = vmin(hit.y + hit.h, y + h);
            hit.h = vmax(0, to - from);
            hit.y = from;
        }

        p.renderer().reset_clip_rect();

        if (total > h) {
            const int bar_h = vmax(p.px(ROW_H), h * h / total);
            const int bar_y = y + (h - bar_h) * m_scroll / vmax(1, total - h);
            p.rect(x + w + p.px(PAD) / 2, bar_y, p.px(SCROLLBAR_W), bar_h, MenuPainter::faded(style.muted, DIM_ALPHA));
        }
    }

    static std::string upper(std::string s) {
        for (char& ch : s) ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
        return s;
    }

    void draw_control(SettingControl& c, int index, int x, int y, int w, int h, bool on, const MenuSettings& style) {
        MenuPainter& p = m_paint;
        const Color fg = on ? style.text : MenuPainter::faded(style.muted, HALF_ALPHA);
        const int mid = y + h / 2, gap = p.px(GAP), small = p.px(SMALL_SIZE);
        auto hit = [&](int hx, int hy, int hw, int hh, HitKind kind, int part = 0) { if (on) add_hit(hx, hy, hw, hh, kind, index, part); };

        switch (c.kind) {
            case ControlKind::Toggle: {
                const bool v = c.get_bool();
                const int b = p.px(BOX), by = mid - b / 2;
                p.rect(x, by, b, b, v ? (on ? style.toggle_on : MenuPainter::faded(style.toggle_on, DIM_ALPHA)) : style.control);
                if (v) draw_check(x, by, b, Color(255, 255, 255));
                p.text(x + b + gap * 2, mid, v ? "On" : "Off", small, v ? fg : style.muted);
                hit(x, y, w, h, HitKind::Toggle);
                break;
            }

            case ControlKind::Integer:
            case ControlKind::Decimal: {
                const int sb = p.px(SMALL_BUTTON), vw = p.px(VALUE_W);
                const bool alt = c.number.alt.active();
                const int track_x = x + sb + gap, track_w = w - sb * 2 - gap * 3 - vw - (alt ? vw + gap : 0);
                const int th = vmax(1, p.px(TRACK_H)), knob = p.px(KNOB);
                const int filled = static_cast<int>(std::lround(track_w * fraction(c, c.get_number())));
                p.button(x, mid - sb / 2, sb, sb, "-", style.control, fg, on && hot(x, mid - sb / 2, sb, sb), small);
                p.rect(track_x, mid - th / 2, track_w, th, style.control);
                p.rect(track_x, mid - th / 2, filled, th, on ? style.accent : style.muted);
                p.rect(track_x + filled - knob / 2, mid - knob / 2, knob, knob, fg);
                const int plus_x = track_x + track_w + gap, value_x = plus_x + sb + gap;
                p.button(plus_x, mid - sb / 2, sb, sb, "+", style.control, fg, on && hot(plus_x, mid - sb / 2, sb, sb), small);
                FieldLook l = look(style, on);
                l.editing = editing(index, Target::Number);
                l.failed = error_on(index, Target::Number);
                l.hot = on && hot(value_x, mid - sb / 2, vw, sb);
                if (l.editing) { l.all_selected = m_field.all_selected(); l.caret = m_field.caret(); l.caret_on = caret_on(); }
                p.field(value_x, mid - sb / 2, vw, sb, l.editing ? m_field.text() : format_number(c.get_number(), c.decimals()), c.number.unit, l);
                m_boxes[index] = { value_x, mid - sb / 2, vw, sb };
                hit(x, mid - sb / 2, sb, sb, HitKind::Minus);
                hit(track_x - knob / 2, y, track_w + knob, h, HitKind::Slider);
                hit(plus_x, mid - sb / 2, sb, sb, HitKind::Plus);
                hit(value_x, mid - sb / 2, vw, sb, HitKind::Value);

                if (alt) {
                    const int alt_x = value_x + vw + gap;
                    FieldLook al = look(style, on);
                    al.editing = editing(index, Target::Number, ALT_PART);
                    al.failed = error_on(index, Target::Number, ALT_PART);
                    al.hot = on && hot(alt_x, mid - sb / 2, vw, sb);
                    if (al.editing) { al.all_selected = m_field.all_selected(); al.caret = m_field.caret(); al.caret_on = caret_on(); }
                    p.field(alt_x, mid - sb / 2, vw, sb, al.editing ? m_field.text() : format_number(c.number.alt.to(c.get_number()), c.decimals()), c.number.alt.unit, al);
                    if (al.failed) m_boxes[index] = { alt_x, mid - sb / 2, vw, sb };
                    hit(alt_x, mid - sb / 2, vw, sb, HitKind::Value, ALT_PART);
                }
                m_track[index] = { track_x, track_w };
                break;
            }

            case ControlKind::Choice: {
                const int sb = p.px(SMALL_BUTTON);
                const std::vector<std::string> options = c.choices();
                const int idx = c.get_choice();
                const std::string value = idx >= 0 && static_cast<std::size_t>(idx) < options.size() ? options[static_cast<std::size_t>(idx)] : std::string("Custom");
                p.button(x, mid - sb / 2, sb, sb, "<", style.control, fg, on && hot(x, mid - sb / 2, sb, sb), small);
                p.rect(x + sb, mid - sb / 2, w - sb * 2, sb, MenuPainter::faded(style.control, HALF_ALPHA));
                p.text_centered(x + w / 2, mid, value, small, fg);
                p.button(x + w - sb, mid - sb / 2, sb, sb, ">", style.control, fg, on && hot(x + w - sb, mid - sb / 2, sb, sb), small);
                hit(x, mid - sb / 2, sb, sb, HitKind::Prev);
                hit(x + sb, mid - sb / 2, w - sb * 2, sb, HitKind::Next);
                hit(x + w - sb, mid - sb / 2, sb, sb, HitKind::Next);
                break;
            }

            case ControlKind::Color: {
                const Color v = c.get_color();
                const int bh = p.px(SMALL_BUTTON), sw = bh, hw = p.px(HEX_W), top = mid - bh / 2;
                p.checker(x, top, sw, bh, bh / 4, style.control, style.muted);
                p.rect(x, top, sw, bh, v);
                p.outline(x, top, sw, bh, on && hot(x, top, sw, bh) ? style.text : style.muted);
                hit(x, top, sw, bh, HitKind::Swatch);
                const int hex_x = x + sw + gap;
                FieldLook l = look(style, on);
                l.editing = editing(index, Target::Hex);
                l.failed = error_on(index, Target::Hex);
                l.hot = on && hot(hex_x, top, hw, bh);
                l.centered = !l.editing;
                if (l.editing) { l.all_selected = m_field.all_selected(); l.caret = m_field.caret(); l.caret_on = caret_on(); }
                p.field(hex_x, top, hw, bh, l.editing ? m_field.text() : ColorMath::to_hex(v, c.with_alpha), std::string(), l);
                hit(hex_x, top, hw, bh, HitKind::Hex);
                m_boxes[index] = { hex_x, top, hw, bh };

                const int channels = c.with_alpha ? ColorPicker::CHANNELS : ColorPicker::CHANNELS - 1;
                const int start = hex_x + hw + gap;
                const int cw = (x + w - start - gap * (channels - 1)) / channels;
                const Color tints[ColorPicker::CHANNELS] = { Color(220, 80, 80), Color(80, 200, 110), Color(90, 140, 240), Color(200, 200, 200) };
                const char* names[ColorPicker::CHANNELS] = { "R", "G", "B", "A" };

                for (int k = 0; k < channels; ++k) {
                    const int cx = start + k * (cw + gap);
                    const int value = ColorMath::channel(v, k);
                    const bool typing = editing(index, Target::Channel, k);
                    p.rect(cx, top, cw, bh, style.control);
                    if (!typing) p.rect(cx, top, static_cast<int>(std::lround(cw * value / static_cast<double>(CHANNEL_MAX))), bh, MenuPainter::faded(tints[k], on ? HALF_ALPHA : DIM_ALPHA));
                    FieldLook cl = look(style, on);
                    cl.fill = Color(0, 0, 0, 0);
                    cl.editing = typing;
                    cl.failed = error_on(index, Target::Channel, k);
                    cl.centered = !typing;
                    if (typing) { cl.all_selected = m_field.all_selected(); cl.caret = m_field.caret(); cl.caret_on = caret_on(); cl.fill = style.control; }
                    p.field(cx, top, cw, bh, typing ? m_field.text() : std::string(names[k]) + " " + std::to_string(value), std::string(), cl);
                    hit(cx, top, cw, bh, HitKind::Channel, k);
                    m_channel[index * ColorPicker::CHANNELS + k] = { cx, cw };
                    if (cl.failed) m_boxes[index] = { cx, top, cw, bh };
                }
                break;
            }

            case ControlKind::Binding: {
                const int bw = p.px(KEY_BUTTON_W), bh = p.px(BUTTON_H) * 3 / 4;
                const bool waiting = m_capture == index;
                const std::string keys = waiting ? std::string("Press a key...") : (c.bindings ? c.bindings->describe(c.action) : std::string());
                const int text_w = w - bw * 3 - gap * 3;
                p.rect(x, mid - bh / 2, vmax(0, text_w), bh, waiting ? MenuPainter::faded(style.accent, HALF_ALPHA) : MenuPainter::faded(style.control, HALF_ALPHA));
                p.text(x + gap, mid, keys, small, keys == "Unbound" ? style.muted : fg);
                const int bx = x + text_w + gap;
                p.button(bx, mid - bh / 2, bw, bh, "Change", style.control, fg, on && hot(bx, mid - bh / 2, bw, bh), small);
                p.button(bx + bw + gap, mid - bh / 2, bw, bh, "Add", style.control, fg, on && hot(bx + bw + gap, mid - bh / 2, bw, bh), small);
                p.button(bx + (bw + gap) * 2, mid - bh / 2, bw, bh, "Unbind", MenuPainter::faded(style.danger, HALF_ALPHA), fg, on && hot(bx + (bw + gap) * 2, mid - bh / 2, bw, bh), small);
                hit(x, mid - bh / 2, text_w, bh, HitKind::Change);
                hit(bx, mid - bh / 2, bw, bh, HitKind::Change);
                hit(bx + bw + gap, mid - bh / 2, bw, bh, HitKind::AddKey);
                hit(bx + (bw + gap) * 2, mid - bh / 2, bw, bh, HitKind::Unbind);
                break;
            }

            case ControlKind::Text: {
                const int bh = p.px(SMALL_BUTTON);
                FieldLook l = look(style, on);
                l.editing = editing(index, Target::Text);
                l.failed = error_on(index, Target::Text);
                l.hot = on && hot(x, mid - bh / 2, w, bh);
                if (l.editing) { l.all_selected = m_field.all_selected(); l.caret = m_field.caret(); l.caret_on = caret_on(); }
                p.field(x, mid - bh / 2, w, bh, l.editing ? m_field.text() : c.get_text(), std::string(), l);
                m_boxes[index] = { x, mid - bh / 2, w, bh };
                hit(x, mid - bh / 2, w, bh, HitKind::TextValue);
                break;
            }

            case ControlKind::Button: {
                const int bh = p.px(BUTTON_H) * 3 / 4, bw = vmin(w, p.px(BUTTON_W) * 2);
                p.button(x, mid - bh / 2, bw, bh, c.label, style.control, fg, on && hot(x, mid - bh / 2, bw, bh), small);
                hit(x, mid - bh / 2, bw, bh, HitKind::Button);
                break;
            }

            case ControlKind::Header: break;
        }
    }

    void draw_check(int x, int y, int size, const Color& c) {
        MenuPainter& p = m_paint;
        const auto pen = fizmo::graphics::Paint::stroke(c, static_cast<unsigned int>(vmax(2, p.px(CHECK_STROKE))));
        const int inset = p.px(CHECK_INSET);
        const int ax = x + inset, ay = y + size / 2;
        const int bx = x + size * 2 / 5, by = y + size - inset;
        const int cx = x + size - inset, cy = y + inset;
        p.renderer().draw_line(ax, ay, bx, by, pen);
        p.renderer().draw_line(bx, by, cx, cy, pen);
    }

    void draw_error_bubble(int px0, int py0, int pw, int ph, const MenuSettings& style) {
        if (m_error.seconds <= 0.0 || m_error.control < 0 || m_picker.is_open()) return;
        auto it = m_boxes.find(m_error.control);
        if (it == m_boxes.end()) return;
        const Box& b = it->second;
        if (b.y + b.h < m_content_y || b.y > m_content_y + m_content_h) return;
        MenuPainter& p = m_paint;
        const int size = p.px(SMALL_SIZE), pad = p.px(BUBBLE_PAD);
        const int tw = static_cast<int>(p.width(m_error.message, size)) + pad * 2;
        const int th = p.line_height(size) + pad;
        const int bx = vclamp(b.x + b.w - tw, px0, px0 + pw - tw);
        const int by = vmin(b.y + b.h + p.px(BUBBLE_GAP), py0 + ph - th);
        p.rect(bx, by, tw, th, MenuPainter::faded(style.sidebar, BUBBLE_ALPHA));
        p.outline(bx, by, tw, th, style.error);
        p.text(bx + pad, by + th / 2, m_error.message, size, style.error);
    }

    void draw_footer(int x, int y, int w, int h, const MenuSettings& style) {
        MenuPainter& p = m_paint;
        p.renderer().draw_line(x, y, x + w, y, fizmo::graphics::Paint::stroke(MenuPainter::faded(style.muted, DIM_ALPHA), 1));
        const int bw = p.px(BUTTON_W), bh = p.px(BUTTON_H), gap = p.px(GAP) * 2, small = p.px(SMALL_SIZE);
        const int done_x = x + w - p.px(PAD) - bw, by = y + (h - bh) / 2;
        p.button(done_x, by, bw, bh, m_done_label, style.accent, Color(255, 255, 255), hot(done_x, by, bw, bh), small);
        add_hit(done_x, by, bw, bh, HitKind::Close);
        int left = done_x;

        if (!m_cancel_label.empty()) {
            left -= gap + bw;
            p.button(left, by, bw, bh, m_cancel_label, style.control, style.text, hot(left, by, bw, bh), small);
            add_hit(left, by, bw, bh, HitKind::Cancel);
        }

        const int reset_x = left - gap - bw;
        p.button(reset_x, by, bw, bh, "Reset tab", style.control, style.text, hot(reset_x, by, bw, bh), small);
        add_hit(reset_x, by, bw, bh, HitKind::ResetTab);
        const int text_x = x + p.px(PAD);
        const int text_w = reset_x - gap - text_x;
        std::string title, body;
        Color title_color = style.text, body_color = style.muted;

        if (capturing()) {
            const SettingControl* c = control(m_capture);
            title = c ? "Binding: " + c->label : std::string("Binding");
            body = "Press the key to use. Esc cancels, clicking anywhere also cancels.";
        } else if (m_field.active()) {
            const SettingControl* c = control(m_edit.control);
            title = c ? c->label : std::string();
            body = typing_help(c);
        } else if (m_error.seconds > 0.0) {
            title = m_error.control < 0 ? "Settings file" : "Invalid value";
            body = m_error.message;
            title_color = body_color = style.error;
        } else if (m_picker.is_open()) {
            const SettingControl* c = control(m_picker.control());
            title = c ? "Color: " + c->label : std::string("Color");
            body = "Drag the wheel and bars, or type hex or RGB. Enter keeps it, Esc undoes it.";
        } else if (const SettingControl* c = control(m_hover_control)) {
            title = c->label;
            body = c->description + hover_help(*c);
        } else {
            title = "Hover a setting to see what it does.";
            body = m_hint.empty() ? "Changes apply immediately" + std::string(m_save_note) : m_hint;
        }

        p.renderer().set_clip_rect(text_x, y, static_cast<unsigned int>(vmax(0, text_w)), static_cast<unsigned int>(h));
        p.text(text_x, y + h / 3, title, p.px(TEXT_SIZE), title_color, true);
        p.text(text_x, y + h * 2 / 3, body, small, body_color);
        p.renderer().reset_clip_rect();
    }

    static std::string hover_help(const SettingControl& c) {
        switch (c.kind) {
            case ControlKind::Integer:
            case ControlKind::Decimal: return "  Click the number to type one. Shift-drag the slider for fine steps, arrow keys nudge it.";
            case ControlKind::Color:   return "  Click the swatch for a color wheel, or type hex or RGB values.";
            default:                   return std::string();
        }
    }

    std::string typing_help(const SettingControl* c) const {
        const bool channel = m_edit.target == Target::Channel || m_edit.target == Target::PickerChannel;
        const bool hex = m_edit.target == Target::Hex || m_edit.target == Target::PickerHex;
        const std::string keys = " Enter applies, Esc cancels, Tab moves on.";
        if (m_edit.target == Target::Text) return "Type any text" + (c && c->max_length > 0 ? std::string(" up to ") + std::to_string(c->max_length) + " characters." : std::string(".")) + keys;
        if (hex) return std::string("Type a hex color like #FFA040") + (c && c->with_alpha ? " or #FFA040C0." : ".") + keys;
        if (channel) return "Type a whole number from 0 to 255." + keys;
        if (!c) return keys;
        const bool alt = m_edit.target == Target::Number && m_edit.part == ALT_PART && c->number.alt.active();
        const Bounds b = alt ? c->number.alt.bounds(c->limits()) : c->limits();
        const std::string unit = alt ? " " + c->number.alt.unit : std::string();
        const std::string kind = c->integer() && !alt ? "Type a whole number" : "Type a number";
        if (b.open_below() && b.open_above()) return kind + "." + keys;
        if (b.open_above()) return kind + " of " + format_limit(b.min) + unit + " or more." + keys;
        if (b.open_below()) return kind + " of " + format_limit(b.max) + unit + " or less." + keys;
        return kind + " from " + format_limit(b.min) + unit + " to " + format_limit(b.max) + unit + "." + keys;
    }

    static double fraction(const SettingControl& c, double v) {
        const Bounds r = c.slider();
        if (r.max <= r.min) return 0.0;
        if (c.number.log && r.min > 0.0) return vclamp(std::log(vmax(v, r.min) / r.min) / std::log(r.max / r.min), 0.0, 1.0);
        return vclamp((v - r.min) / (r.max - r.min), 0.0, 1.0);
    }

    static double from_fraction(const SettingControl& c, double t) {
        const Bounds r = c.slider();
        t = vclamp(t, 0.0, 1.0);
        if (c.number.log && r.min > 0.0) return r.min * std::pow(r.max / r.min, t);
        return r.min + (r.max - r.min) * t;
    }

    static double snapped(const SettingControl& c, double v) {
        const Bounds r = c.slider();
        const double step = c.step();
        if (step > 0.0) v = r.min + std::round((v - r.min) / step) * step;
        return c.limits().clamp(v);
    }

    void set_number(SettingControl& c, double v) {
        const double before = c.get_number();
        c.set_number(c.limits().clamp(v));
        if (c.get_number() != before) changed(c);
    }

    void set_color(SettingControl& c, const Color& v) {
        const Color before = c.get_color();
        c.set_color(v);
        if (c.get_color() != before) changed(c);
        m_picker.produced(c.get_color());
    }

    void nudge(int index, double direction, bool shift) {
        SettingControl* c = control(index);
        if (!c || !c->is_number() || !c->enabled()) return;
        set_number(*c, snapped(*c, c->get_number() + direction * c->step() * (shift ? SHIFT_STEPS : 1.0)));
    }

    void begin_drag(const Hit& h, int x, int w) {
        m_drag = {};
        m_drag.active  = true;
        m_drag.kind    = h.kind;
        m_drag.control = h.control;
        m_drag.part    = h.part;
        m_drag.x       = x;
        m_drag.w       = vmax(1, w);
        m_drag.press_x = m_mouse_x;
        m_drag.press_y = m_mouse_y;
        m_drag.fine    = m_shift;
    }

    void drag_to(int mx, int my) {
        SettingControl* c = control(m_drag.control);
        if (!c) return;
        if (std::abs(mx - m_drag.press_x) > DRAG_START_PX || std::abs(my - m_drag.press_y) > DRAG_START_PX) m_drag.moved = true;

        switch (m_drag.kind) {
            case HitKind::Slider: {
                if (m_shift != m_drag.fine) {
                    m_drag.fine = m_shift;
                    m_drag.anchor_x = mx;
                    m_drag.anchor_t = fraction(*c, c->get_number());
                }
                const double t = m_drag.anchor_t + static_cast<double>(mx - m_drag.anchor_x) / m_drag.w * (m_drag.fine ? FINE_DRAG : 1.0);
                set_number(*c, snapped(*c, from_fraction(*c, t)));
                break;
            }
            case HitKind::Channel: {
                if (!m_drag.moved) break;
                const double t = static_cast<double>(mx - m_drag.x) / m_drag.w;
                set_color(*c, ColorMath::with_channel(c->get_color(), m_drag.part, static_cast<int>(std::lround(vclamp(t, 0.0, 1.0) * CHANNEL_MAX))));
                break;
            }
            case HitKind::Wheel:    set_color(*c, m_picker.pick_wheel(mx, my)); break;
            case HitKind::ValueBar: set_color(*c, m_picker.pick_value(my)); break;
            case HitKind::AlphaBar: set_color(*c, m_picker.pick_alpha(my)); break;
            default: break;
        }
    }

    void release_drag() {
        const Drag done = m_drag;
        m_drag.active = false;
        if (done.kind != HitKind::Channel || done.moved) return;
        begin_edit(Target::Channel, done.control, done.part);
    }

    void begin_edit(Target target, int index, int part) {
        SettingControl* c = control(index);
        if (!c) return;
        m_edit = { target, index, part };

        switch (target) {
            case Target::Number: {
                const bool alt = part == ALT_PART && c->number.alt.active();
                const double shown = alt ? c->number.alt.to(c->get_number()) : c->get_number();
                const Bounds b = alt ? c->number.alt.bounds(c->limits()) : c->limits();
                m_field.begin(format_number(shown, c->decimals()), c->integer() && !alt ? TextFilter::Integer : TextFilter::Decimal, b.min < 0.0, NUMBER_LENGTH);
                break;
            }
            case Target::Channel:
            case Target::PickerChannel:
                m_field.begin(std::to_string(ColorMath::channel(c->get_color(), part)), TextFilter::Integer, false, CHANNEL_LENGTH);
                break;
            case Target::Hex:
            case Target::PickerHex:
                m_field.begin(ColorMath::to_hex(c->get_color(), c->with_alpha), TextFilter::Hex, false, HEX_LENGTH);
                break;
            case Target::Text:
                m_field.begin(c->get_text(), TextFilter::Text, false, c->max_length > 0 ? c->max_length : TEXT_LENGTH);
                break;
            case Target::None: break;
        }
    }

    bool same_target(const Hit& h) const noexcept {
        if (h.control != m_edit.control) return false;
        switch (h.kind) {
            case HitKind::Value:         return m_edit.target == Target::Number && m_edit.part == h.part;
            case HitKind::Hex:           return m_edit.target == Target::Hex;
            case HitKind::Channel:       return m_edit.target == Target::Channel && m_edit.part == h.part;
            case HitKind::PickerHex:     return m_edit.target == Target::PickerHex;
            case HitKind::PickerChannel: return m_edit.target == Target::PickerChannel && m_edit.part == h.part;
            case HitKind::TextValue:     return m_edit.target == Target::Text;
            default:                     return false;
        }
    }

    void stop_editing(bool apply) {
        if (!m_field.active()) return;
        const Edit edit = m_edit;
        const std::string text = m_field.text();
        m_field.end();
        m_edit = {};
        if (apply) commit(edit, text);
    }

    void commit(const Edit& edit, const std::string& text) {
        SettingControl* c = control(edit.control);
        if (!c) return;
        m_error.seconds = 0.0;

        switch (edit.target) {
            case Target::Number: {
                if (edit.part == ALT_PART && c->number.alt.active()) {
                    const AltUnit& alt = c->number.alt;
                    const NumberCheck check = check_value(text, alt.bounds(c->limits()), false);
                    if (!check.ok) { fail(*c, edit, check.error); break; }
                    const double value = c->limits().clamp(alt.from(check.value));
                    set_number(*c, c->integer() ? std::round(value) : value);
                    break;
                }

                const NumberCheck check = check_number(*c, text);
                if (check.ok) set_number(*c, check.value);
                else fail(*c, edit, check.error);
                break;
            }
            case Target::Channel:
            case Target::PickerChannel: {
                const NumberCheck check = check_value(text, { 0.0, static_cast<double>(CHANNEL_MAX) }, true);
                if (check.ok) set_color(*c, ColorMath::with_channel(c->get_color(), edit.part, static_cast<int>(check.value)));
                else fail(*c, edit, check.error);
                break;
            }
            case Target::Text: {
                const std::string clean = trimmed(text);
                if (clean.empty()) fail(*c, edit, "Type at least one character.");
                else { const std::string before = c->get_text(); c->set_text(clean); if (c->get_text() != before) changed(*c); }
                break;
            }
            case Target::Hex:
            case Target::PickerHex: {
                Color parsed;
                std::string error;
                if (ColorMath::parse_hex(trimmed(text), c->with_alpha, c->get_color(), parsed, error)) set_color(*c, parsed);
                else fail(*c, edit, error);
                break;
            }
            case Target::None: break;
        }
    }

    void fail(SettingControl& c, const Edit& edit, const std::string& message) {
        std::string note;

        if (c.reset) {
            c.reset();
            changed(c);
            if (c.is_number()) note = " Reset to the default (" + format_number(c.get_number(), c.decimals()) + ").";
            else if (c.kind == ControlKind::Color) note = " Reset to the default (" + ColorMath::to_hex(c.get_color(), c.with_alpha) + ").";
            else note = " Reset to the default.";
            if (c.kind == ControlKind::Color) m_picker.produced(c.get_color());
        } else if (c.is_number()) {
            note = " Kept " + format_number(c.get_number(), c.decimals()) + ".";
        }

        m_error = { message + note, edit.control, edit.target, edit.part, ERROR_SECONDS };
    }

    void text_key(const std::string& key) {
        const Edit edit = m_edit;
        switch (m_field.key(key, m_shift, m_control)) {
            case TextResult::Commit: stop_editing(true); break;
            case TextResult::Cancel: m_field.end(); m_edit = {}; break;
            case TextResult::Next:
            case TextResult::Previous: {
                const bool back = m_shift;
                stop_editing(true);
                move_edit(edit, back ? -1 : 1);
                break;
            }
            default: break;
        }
    }

    void move_edit(const Edit& from, int direction) {
        if (from.target == Target::Number) {
            const SettingControl* c = control(from.control);
            const bool alt = c && c->number.alt.active();

            if (alt && from.part == 0 && direction > 0) { begin_edit(Target::Number, from.control, ALT_PART); return; }
            if (alt && from.part == ALT_PART && direction < 0) { begin_edit(Target::Number, from.control, 0); return; }
        }

        if (from.target == Target::PickerHex || from.target == Target::PickerChannel) {
            const SettingControl* c = control(from.control);
            const int last = c && c->with_alpha ? ColorPicker::CHANNELS - 1 : ColorPicker::CHANNELS - 2;
            int part = from.target == Target::PickerHex ? -1 : from.part;
            part += direction;
            if (part < 0) begin_edit(Target::PickerHex, from.control, 0);
            else if (part <= last) begin_edit(Target::PickerChannel, from.control, part);
            return;
        }

        const auto& list = m_tabs[m_tab].controls;
        for (int i = from.control + direction; i >= 0 && i < static_cast<int>(list.size()); i += direction) {
            const SettingControl& c = list[static_cast<std::size_t>(i)];
            if (!c.is_number() || !c.enabled()) continue;
            begin_edit(Target::Number, i, direction < 0 && c.number.alt.active() ? ALT_PART : 0);
            reveal(i);
            return;
        }
    }

    void reveal(int index) {
        if (index < 0 || static_cast<std::size_t>(index) >= m_offsets.size()) return;
        const int top = m_offsets[static_cast<std::size_t>(index)];
        if (top < m_scroll) m_scroll = top;
        else if (top + m_row_h > m_scroll + m_content_h) m_scroll = top + m_row_h - m_content_h;
    }

    void cancel_picker() {
        stop_editing(false);
        if (SettingControl* c = control(m_picker.control())) set_color(*c, m_picker.original());
        m_picker.close();
    }

    void press(const Hit& h) {
        SettingControl* c = control(h.control);

        switch (h.kind) {
            case HitKind::Tab:
                m_picker.close();
                m_tab = static_cast<std::size_t>(h.part);
                m_scroll = 0;
                m_boxes.clear();
                m_error.control = -1;
                return;
            case HitKind::Group: {
                const std::string& group = m_tabs[static_cast<std::size_t>(h.part)].group;
                if (!m_closed_groups.erase(group)) m_closed_groups.insert(group);
                return;
            }
            case HitKind::Close:
                m_close_requested = true;
                return;
            case HitKind::Cancel:
                m_cancel_requested = true;
                return;
            case HitKind::ResetTab:
                for (SettingControl& each : m_tabs[m_tab].controls) {
                    if (!each.reset || !each.is_default || each.is_default()) continue;
                    each.reset();
                    changed(each);
                }
                return;
            default: break;
        }

        if (!c || !c->enabled()) return;

        switch (h.kind) {
            case HitKind::Toggle:
                c->set_bool(!c->get_bool());
                changed(*c);
                break;
            case HitKind::Minus: nudge(h.control, -1.0, m_shift); break;
            case HitKind::Plus:  nudge(h.control, 1.0, m_shift); break;
            case HitKind::Value: begin_edit(Target::Number, h.control, h.part); break;
            case HitKind::Slider: {
                auto it = m_track.find(h.control);
                if (it == m_track.end()) break;
                begin_drag(h, it->second.x, it->second.w);
                set_number(*c, snapped(*c, from_fraction(*c, static_cast<double>(m_mouse_x - it->second.x) / vmax(1, it->second.w))));
                m_drag.anchor_x = m_mouse_x;
                m_drag.anchor_t = fraction(*c, c->get_number());
                break;
            }
            case HitKind::Channel: {
                auto it = m_channel.find(h.control * ColorPicker::CHANNELS + h.part);
                if (it != m_channel.end()) begin_drag(h, it->second.x, it->second.w);
                break;
            }
            case HitKind::Hex: begin_edit(Target::Hex, h.control, 0); break;
            case HitKind::TextValue: begin_edit(Target::Text, h.control, 0); break;
            case HitKind::Swatch: m_picker.open(*c, h.control); break;
            case HitKind::Wheel:
                if (!m_picker.on_wheel(m_mouse_x, m_mouse_y)) break;
                begin_drag(h, 0, 1);
                drag_to(m_mouse_x, m_mouse_y);
                break;
            case HitKind::ValueBar:
            case HitKind::AlphaBar:
                begin_drag(h, 0, 1);
                drag_to(m_mouse_x, m_mouse_y);
                break;
            case HitKind::PickerHex: begin_edit(Target::PickerHex, h.control, 0); break;
            case HitKind::PickerChannel: begin_edit(Target::PickerChannel, h.control, h.part); break;
            case HitKind::PickerDone: m_picker.close(); break;
            case HitKind::PickerCancel: cancel_picker(); break;
            case HitKind::Prev:
            case HitKind::Next: {
                const int n = static_cast<int>(c->choices().size());
                if (n == 0) break;
                const int current = c->get_choice();
                const int step = h.kind == HitKind::Next ? 1 : -1;
                c->set_choice(((current < 0 ? 0 : current) + step + n) % n);
                changed(*c);
                break;
            }
            case HitKind::Change: m_capture = h.control; m_capture_add = false; break;
            case HitKind::AddKey: m_capture = h.control; m_capture_add = true; break;
            case HitKind::Unbind:
                if (c->bindings) c->bindings->unbind(c->action);
                changed(*c);
                break;
            case HitKind::Button:
                if (c->press) c->press();
                changed(*c);
                break;
            case HitKind::Reset:
                if (c->reset) c->reset();
                changed(*c);
                break;
            default: break;
        }
    }

    void finish_capture(const std::string& key) {
        SettingControl* c = control(m_capture);
        const bool add = m_capture_add;
        m_capture = -1;
        if (!c || !c->bindings || key.empty() || key == CANCEL_KEY) return;
        if (add) c->bindings->add_key(c->action, key);
        else c->bindings->set_key(c->action, key);
        changed(*c);
    }

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