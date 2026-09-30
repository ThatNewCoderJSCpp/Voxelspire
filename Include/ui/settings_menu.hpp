#ifndef VOXELSPIRE_UI_SETTINGS_MENU_HPP
#define VOXELSPIRE_UI_SETTINGS_MENU_HPP

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <unordered_map>
#include <vector>
#include "settings_model.hpp"

namespace voxelspire {

class SettingsMenu {
public:
    static constexpr unsigned int LEFT_MOUSE = 1;
    static constexpr double       SHIFT_STEPS = 10.0;
    static constexpr int          SCROLL_ROWS = 3;

    void set_tabs(std::vector<SettingsTab> tabs) {
        m_tabs = std::move(tabs);
        m_tab = vmin(m_tab, m_tabs.empty() ? std::size_t(0) : m_tabs.size() - 1);
    }

    std::vector<SettingsTab>&       tabs()       noexcept { return m_tabs; }
    const std::vector<SettingsTab>& tabs() const noexcept { return m_tabs; }

    bool is_open() const noexcept { return m_open; }
    bool capturing() const noexcept { return m_capture >= 0; }

    void open() { m_open = true; m_capture = -1; m_drag = {}; m_close_requested = false; }
    void close() { m_open = false; m_capture = -1; m_drag = {}; }

    bool take_close_request() noexcept { const bool r = m_close_requested; m_close_requested = false; return r; }
    std::uint32_t take_changes() noexcept { const std::uint32_t c = m_changes; m_changes = Apply::Nothing; return c; }

    bool on_event(const fizmo::windows::WindowEvent& e) {
        using fizmo::windows::WindowEventType;
        if (!m_open) return false;

        if (e.type == WindowEventType::MouseScroll) {
            m_scroll_steps += e.scroll_delta;
            return true;
        }

        if (e.type == WindowEventType::KeyPress && capturing()) {
            finish_capture(e.key_name);
            return true;
        }

        return false;
    }

    void update(const fizmo::windows::InputManager& input) {
        if (!m_open) return;
        const int mx = input.mouse_x(), my = input.mouse_y();
        m_mouse_x = mx;
        m_mouse_y = my;
        const bool shift = input.is_key_down("LeftShift") || input.is_key_down("RightShift");
        const Hit* hover = hit_at(mx, my);
        m_hover_control = hover && hover->control >= 0 ? hover->control : (row_at(mx, my));

        if (m_scroll_steps != 0) {
            if (hover && hover->kind == HitKind::Slider && !m_drag.active) nudge(hover->control, m_scroll_steps > 0 ? 1.0 : -1.0, shift);
            else m_scroll -= m_scroll_steps * SCROLL_ROWS * m_row_h;
            m_scroll_steps = 0;
        }

        if (m_drag.active) {
            if (!input.is_mouse_button_down(LEFT_MOUSE)) m_drag.active = false;
            else drag_to(mx);
            return;
        }

        if (!input.is_mouse_button_just_pressed(LEFT_MOUSE)) return;
        if (capturing()) { m_capture = -1; return; }
        if (hover) press(*hover, shift);
    }

    void render(fizmo::windows::Renderer& r, unsigned int w, unsigned int h, const MenuSettings& style) {
        if (!m_open || m_tabs.empty()) return;
        m_hits.clear();
        m_rows.clear();
        const double fit = vmin(1.0, vmin((w - MARGIN * 2.0) / PANEL_W, (h - MARGIN * 2.0) / PANEL_H));
        m_s = vmax(style.scale * vmax(fit, MIN_FIT), MIN_SCALE);
        m_row_h = px(ROW_H);
        const int pw = vmin(px(PANEL_W), static_cast<int>(w) - px(MARGIN) * 2);
        const int ph = vmin(px(PANEL_H), static_cast<int>(h) - px(MARGIN) * 2);
        const int x0 = (static_cast<int>(w) - pw) / 2, y0 = (static_cast<int>(h) - ph) / 2;
        const int side = px(SIDEBAR_W), head = px(HEADER_H), foot = px(FOOTER_H);

        r.draw_rect(0, 0, w, h, fill(style.backdrop));
        r.draw_rect(x0, y0, static_cast<unsigned int>(pw), static_cast<unsigned int>(ph), fill(style.panel));
        r.draw_rect(x0, y0, static_cast<unsigned int>(side), static_cast<unsigned int>(ph), fill(style.sidebar));
        text(r, x0 + px(PAD), y0 + head / 2, "Settings", px(TITLE_SIZE), style.text, true);
        draw_tabs(r, x0, y0 + head, side, style);

        const SettingsTab& tab = m_tabs[m_tab];
        const int cx = x0 + side + px(PAD), cw = pw - side - px(PAD) * 2;
        text(r, cx, y0 + head / 2 - px(SUMMARY_LIFT), tab.name, px(TAB_TITLE_SIZE), style.text, true);
        text(r, cx, y0 + head / 2 + px(SUMMARY_DROP), tab.summary, px(SMALL_SIZE), style.muted);
        const int top = y0 + head, bottom = y0 + ph - foot;
        draw_content(r, cx, top, cw, bottom - top, style);
        draw_footer(r, x0 + side, bottom, pw - side, foot, style);
    }

private:
    enum class HitKind : std::uint8_t { Tab = 0, Toggle, Slider, Minus, Plus, Prev, Next, Channel, Change, AddKey, Unbind, Button, Reset, ResetTab, Close };

    struct Hit {
        int     x = 0, y = 0, w = 0, h = 0;
        HitKind kind = HitKind::Close;
        int     control = -1;
        int     part = 0;

        bool contains(int px_, int py_) const noexcept { return px_ >= x && px_ < x + w && py_ >= y && py_ < y + h; }
    };

    struct Row { int y = 0, h = 0, control = -1; };

    struct Drag {
        bool    active = false;
        HitKind kind = HitKind::Slider;
        int     control = -1;
        int     part = 0;
        int     x = 0, w = 1;
    };

    static constexpr double PANEL_W         = 1120.0;
    static constexpr double PANEL_H         = 720.0;
    static constexpr double MARGIN          = 20.0;
    static constexpr double MIN_FIT         = 0.55;
    static constexpr double MIN_SCALE       = 0.4;
    static constexpr double SIDEBAR_W       = 220.0;
    static constexpr double HEADER_H        = 64.0;
    static constexpr double FOOTER_H        = 70.0;
    static constexpr double PAD             = 20.0;
    static constexpr double ROW_H           = 38.0;
    static constexpr double HEADER_ROW_H    = 46.0;
    static constexpr double TAB_H           = 38.0;
    static constexpr double TEXT_SIZE       = 15.0;
    static constexpr double SMALL_SIZE      = 13.0;
    static constexpr double TITLE_SIZE      = 22.0;
    static constexpr double TAB_TITLE_SIZE  = 18.0;
    static constexpr double SECTION_SIZE    = 14.0;
    static constexpr double SUMMARY_LIFT    = 10.0;
    static constexpr double SUMMARY_DROP    = 12.0;
    static constexpr double CONTROL_W       = 400.0;
    static constexpr double RESET_W         = 58.0;
    static constexpr double BOX             = 22.0;
    static constexpr double SMALL_BUTTON    = 26.0;
    static constexpr double VALUE_W         = 96.0;
    static constexpr double TRACK_H         = 6.0;
    static constexpr double KNOB            = 14.0;
    static constexpr double KEY_BUTTON_W    = 70.0;
    static constexpr double BUTTON_W        = 110.0;
    static constexpr double BUTTON_H        = 30.0;
    static constexpr double GAP             = 6.0;
    static constexpr double SCROLLBAR_W     = 5.0;
    static constexpr double ACCENT_BAR      = 4.0;
    static constexpr double CHECK_INSET     = 5.0;
    static constexpr double CHANNEL_MAX     = 255.0;
    static constexpr int    DIM_ALPHA       = 90;
    static constexpr int    HALF_ALPHA      = 160;
    static constexpr std::size_t MAX_CACHED = 4096;

    double px_d(double v) const noexcept { return v * m_s; }
    int    px(double v) const noexcept { return static_cast<int>(std::lround(v * m_s)); }

    static fizmo::graphics::Paint fill(const Color& c) { return fizmo::graphics::Paint::fill(c); }
    static Color faded(const Color& c, int alpha) { return Color(c.red(), c.green(), c.blue(), static_cast<std::uint8_t>(vclamp(alpha, 0, 255))); }

    static fizmo::text::TextStyle style_of(int size, const Color& c, bool bold) {
        fizmo::text::TextStyle s(static_cast<double>(size), c);
        if (bold) s.set_bold();
        return s;
    }

    unsigned int width(fizmo::windows::Renderer& r, const std::string& s, int size, bool bold = false) {
        if (m_widths.size() > MAX_CACHED) m_widths.clear();
        const std::string key = std::to_string(size) + (bold ? "b" : "r") + s;
        auto it = m_widths.find(key);
        if (it != m_widths.end()) return it->second;
        const unsigned int wd = r.measure_text(s, style_of(size, Color(), bold)).width;
        m_widths.emplace(key, wd);
        return wd;
    }

    int line_height(fizmo::windows::Renderer& r, int size) {
        auto it = m_heights.find(size);
        if (it != m_heights.end()) return it->second;
        const unsigned int ht = r.measure_text("Hg", style_of(size, Color(), false)).height;
        const int value = ht > 0 ? static_cast<int>(ht) : size;
        m_heights.emplace(size, value);
        return value;
    }

    void text(fizmo::windows::Renderer& r, int x, int center_y, const std::string& s, int size, const Color& c, bool bold = false) {
        if (s.empty()) return;
        r.draw_text(x, center_y - line_height(r, size) / 2, s, style_of(size, c, bold));
    }

    void text_centered(fizmo::windows::Renderer& r, int cx, int cy, const std::string& s, int size, const Color& c, bool bold = false) {
        text(r, cx - static_cast<int>(width(r, s, size, bold)) / 2, cy, s, size, c, bold);
    }

    void button_box(fizmo::windows::Renderer& r, int x, int y, int w, int h, const std::string& label, const Color& bg, const Color& fg, bool hot) {
        r.draw_rect(x, y, static_cast<unsigned int>(w), static_cast<unsigned int>(h), fill(hot ? lighter(bg) : bg));
        text_centered(r, x + w / 2, y + h / 2, label, px(SMALL_SIZE), fg);
    }

    static Color lighter(const Color& c) {
        auto up = [](std::uint8_t v) { return static_cast<std::uint8_t>(vmin(255, v + 22)); };
        return Color(up(c.red()), up(c.green()), up(c.blue()), c.alpha());
    }

    bool hot(int x, int y, int w, int h) const noexcept { return m_mouse_x >= x && m_mouse_x < x + w && m_mouse_y >= y && m_mouse_y < y + h; }

    void add_hit(int x, int y, int w, int h, HitKind kind, int control = -1, int part = 0) {
        m_hits.push_back({ x, y, w, h, kind, control, part });
    }

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

    void draw_tabs(fizmo::windows::Renderer& r, int x, int y, int w, const MenuSettings& style) {
        const int th = px(TAB_H);

        for (std::size_t i = 0; i < m_tabs.size(); ++i) {
            const int ty = y + static_cast<int>(i) * th;
            const bool selected = i == m_tab;
            if (selected) {
                r.draw_rect(x, ty, static_cast<unsigned int>(w), static_cast<unsigned int>(th), fill(style.panel));
                r.draw_rect(x, ty, static_cast<unsigned int>(px(ACCENT_BAR)), static_cast<unsigned int>(th), fill(style.accent));
            } else if (hot(x, ty, w, th)) {
                r.draw_rect(x, ty, static_cast<unsigned int>(w), static_cast<unsigned int>(th), fill(style.row_hover));
            }
            text(r, x + px(PAD), ty + th / 2, m_tabs[i].name, px(TEXT_SIZE), selected ? style.text : style.muted, selected);
            add_hit(x, ty, w, th, HitKind::Tab, -1, static_cast<int>(i));
        }
    }

    void draw_content(fizmo::windows::Renderer& r, int x, int y, int w, int h, const MenuSettings& style) {
        SettingsTab& tab = m_tabs[m_tab];
        int total = 0;
        for (const SettingControl& c : tab.controls) total += c.kind == ControlKind::Header ? px(HEADER_ROW_H) : m_row_h;
        m_scroll = vclamp(m_scroll, 0, vmax(0, total - h));
        m_content_x = x;
        m_content_y = y;
        m_content_w = w;
        m_content_h = h;
        r.set_clip_rect(x, y, static_cast<unsigned int>(w), static_cast<unsigned int>(h));
        int cy = y - m_scroll;
        const int control_w = vmin(px(CONTROL_W), w / 2);
        const int reset_w = px(RESET_W);
        const int control_x = x + w - control_w - reset_w - px(GAP) * 2;
        const std::size_t hits_before = m_hits.size();

        for (std::size_t i = 0; i < tab.controls.size(); ++i) {
            SettingControl& c = tab.controls[i];
            const int rh = c.kind == ControlKind::Header ? px(HEADER_ROW_H) : m_row_h;
            const int index = static_cast<int>(i);

            if (cy + rh >= y && cy <= y + h) {
                if (c.kind == ControlKind::Header) {
                    text(r, x, cy + rh / 2 + px(GAP), upper(c.label), px(SECTION_SIZE), style.accent, true);
                } else {
                    const bool on = c.enabled();
                    if (index == m_hover_control) r.draw_rect(x, cy, static_cast<unsigned int>(w), static_cast<unsigned int>(rh), fill(style.row_hover));
                    text(r, x + px(GAP), cy + rh / 2, c.label, px(TEXT_SIZE), on ? style.text : faded(style.muted, HALF_ALPHA));
                    m_rows.push_back({ cy, rh, index });
                    draw_control(r, c, index, control_x, cy, control_w, rh, on, style);

                    if (c.reset && c.is_default && !c.is_default()) {
                        const int bx = control_x + control_w + px(GAP);
                        const int bh = px(BUTTON_H) * 3 / 4;
                        button_box(r, bx, cy + (rh - bh) / 2, reset_w, bh, "Reset", style.control, style.muted, hot(bx, cy + (rh - bh) / 2, reset_w, bh));
                        add_hit(bx, cy + (rh - bh) / 2, reset_w, bh, HitKind::Reset, index);
                    }
                }
            }

            cy += rh;
        }

        for (std::size_t i = hits_before; i < m_hits.size(); ++i) {
            Hit& hit = m_hits[i];
            const int top = vmax(hit.y, y), end = vmin(hit.y + hit.h, y + h);
            hit.h = vmax(0, end - top);
            hit.y = top;
        }

        r.reset_clip_rect();

        if (total > h) {
            const int bar_h = vmax(px(ROW_H), h * h / total);
            const int bar_y = y + (h - bar_h) * m_scroll / vmax(1, total - h);
            r.draw_rect(x + w + px(PAD) / 2, bar_y, static_cast<unsigned int>(px(SCROLLBAR_W)), static_cast<unsigned int>(bar_h), fill(faded(style.muted, DIM_ALPHA)));
        }
    }

    static std::string upper(std::string s) {
        for (char& ch : s) ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
        return s;
    }

    void draw_control(fizmo::windows::Renderer& r, SettingControl& c, int index, int x, int y, int w, int h, bool on, const MenuSettings& style) {
        const Color fg = on ? style.text : faded(style.muted, HALF_ALPHA);
        const int mid = y + h / 2;
        const HitKind none = HitKind::Close;
        auto hit = [&](int hx, int hy, int hw, int hh, HitKind kind, int part = 0) { if (on && kind != none) add_hit(hx, hy, hw, hh, kind, index, part); };

        switch (c.kind) {
            case ControlKind::Toggle: {
                const bool v = c.get_bool();
                const int b = px(BOX), bx = x, by = mid - b / 2;
                r.draw_rect(bx, by, static_cast<unsigned int>(b), static_cast<unsigned int>(b), fill(v ? (on ? style.toggle_on : faded(style.toggle_on, DIM_ALPHA)) : style.control));
                if (v) draw_check(r, bx, by, b, Color(255, 255, 255));
                text(r, bx + b + px(GAP) * 2, mid, v ? "On" : "Off", px(SMALL_SIZE), v ? fg : style.muted);
                hit(bx, y, w, h, HitKind::Toggle);
                break;
            }

            case ControlKind::Number: {
                const int sb = px(SMALL_BUTTON), vw = px(VALUE_W), gap = px(GAP);
                const int track_x = x + sb + gap, track_w = w - sb * 2 - gap * 3 - vw;
                const double t = fraction(c, c.get_number());
                button_box(r, x, mid - sb / 2, sb, sb, "-", style.control, fg, on && hot(x, mid - sb / 2, sb, sb));
                r.draw_rect(track_x, mid - px(TRACK_H) / 2, static_cast<unsigned int>(track_w), static_cast<unsigned int>(vmax(1, px(TRACK_H))), fill(style.control));
                r.draw_rect(track_x, mid - px(TRACK_H) / 2, static_cast<unsigned int>(std::lround(track_w * t)), static_cast<unsigned int>(vmax(1, px(TRACK_H))), fill(on ? style.accent : style.muted));
                const int knob = px(KNOB);
                r.draw_rect(track_x + static_cast<int>(std::lround(track_w * t)) - knob / 2, mid - knob / 2, static_cast<unsigned int>(knob), static_cast<unsigned int>(knob), fill(fg));
                const int plus_x = track_x + track_w + gap;
                button_box(r, plus_x, mid - sb / 2, sb, sb, "+", style.control, fg, on && hot(plus_x, mid - sb / 2, sb, sb));
                const std::string value = format_number(c.get_number(), c.range);
                text(r, plus_x + sb + gap * 2, mid, value, px(SMALL_SIZE), fg);
                hit(x, mid - sb / 2, sb, sb, HitKind::Minus);
                hit(track_x - knob / 2, y, track_w + knob, h, HitKind::Slider);
                hit(plus_x, mid - sb / 2, sb, sb, HitKind::Plus);
                m_track[index] = { track_x, track_w };
                break;
            }

            case ControlKind::Choice: {
                const int sb = px(SMALL_BUTTON);
                const int idx = c.get_choice();
                const std::string value = idx >= 0 && static_cast<std::size_t>(idx) < c.options.size() ? c.options[static_cast<std::size_t>(idx)] : std::string("Custom");
                button_box(r, x, mid - sb / 2, sb, sb, "<", style.control, fg, on && hot(x, mid - sb / 2, sb, sb));
                r.draw_rect(x + sb, mid - sb / 2, static_cast<unsigned int>(w - sb * 2), static_cast<unsigned int>(sb), fill(faded(style.control, HALF_ALPHA)));
                text_centered(r, x + w / 2, mid, value, px(SMALL_SIZE), fg);
                button_box(r, x + w - sb, mid - sb / 2, sb, sb, ">", style.control, fg, on && hot(x + w - sb, mid - sb / 2, sb, sb));
                hit(x, mid - sb / 2, sb, sb, HitKind::Prev);
                hit(x + sb, mid - sb / 2, w - sb * 2, sb, HitKind::Next);
                hit(x + w - sb, mid - sb / 2, sb, sb, HitKind::Next);
                break;
            }

            case ControlKind::Color: {
                const Color v = c.get_color();
                const int sw = px(BOX), gap = px(GAP);
                r.draw_rect(x, mid - sw / 2, static_cast<unsigned int>(sw), static_cast<unsigned int>(sw), fill(Color(v.red(), v.green(), v.blue())));
                const int channels = c.with_alpha ? 4 : 3;
                const int cw = (w - sw - gap * (channels + 1)) / channels;
                const Color tints[4] = { Color(220, 80, 80), Color(80, 200, 110), Color(90, 140, 240), Color(200, 200, 200) };
                const char* names[4] = { "R", "G", "B", "A" };

                for (int k = 0; k < channels; ++k) {
                    const int cx = x + sw + gap + k * (cw + gap);
                    const int value = channel(v, k);
                    const int bh = px(SMALL_BUTTON);
                    r.draw_rect(cx, mid - bh / 2, static_cast<unsigned int>(cw), static_cast<unsigned int>(bh), fill(style.control));
                    r.draw_rect(cx, mid - bh / 2, static_cast<unsigned int>(std::lround(cw * value / CHANNEL_MAX)), static_cast<unsigned int>(bh), fill(faded(tints[k], on ? HALF_ALPHA : DIM_ALPHA)));
                    text_centered(r, cx + cw / 2, mid, std::string(names[k]) + " " + std::to_string(value), px(SMALL_SIZE), fg);
                    hit(cx, mid - bh / 2, cw, bh, HitKind::Channel, k);
                    m_channel[index * 4 + k] = { cx, cw };
                }
                break;
            }

            case ControlKind::Binding: {
                const int bw = px(KEY_BUTTON_W), bh = px(BUTTON_H) * 3 / 4, gap = px(GAP);
                const bool waiting = m_capture == index;
                const std::string keys = waiting ? std::string("Press a key...") : (c.bindings ? c.bindings->describe(c.action) : std::string());
                const int text_w = w - bw * 3 - gap * 3;
                r.draw_rect(x, mid - bh / 2, static_cast<unsigned int>(vmax(0, text_w)), static_cast<unsigned int>(bh), fill(waiting ? faded(style.accent, HALF_ALPHA) : faded(style.control, HALF_ALPHA)));
                text(r, x + gap, mid, keys, px(SMALL_SIZE), keys == "Unbound" ? style.muted : fg);
                const int bx = x + text_w + gap;
                button_box(r, bx, mid - bh / 2, bw, bh, "Change", style.control, fg, on && hot(bx, mid - bh / 2, bw, bh));
                button_box(r, bx + bw + gap, mid - bh / 2, bw, bh, "Add", style.control, fg, on && hot(bx + bw + gap, mid - bh / 2, bw, bh));
                button_box(r, bx + (bw + gap) * 2, mid - bh / 2, bw, bh, "Unbind", faded(style.danger, HALF_ALPHA), fg, on && hot(bx + (bw + gap) * 2, mid - bh / 2, bw, bh));
                hit(x, mid - bh / 2, text_w, bh, HitKind::Change);
                hit(bx, mid - bh / 2, bw, bh, HitKind::Change);
                hit(bx + bw + gap, mid - bh / 2, bw, bh, HitKind::AddKey);
                hit(bx + (bw + gap) * 2, mid - bh / 2, bw, bh, HitKind::Unbind);
                break;
            }

            case ControlKind::Button: {
                const int bh = px(BUTTON_H) * 3 / 4, bw = vmin(w, px(BUTTON_W) * 2);
                button_box(r, x, mid - bh / 2, bw, bh, c.label, style.control, fg, on && hot(x, mid - bh / 2, bw, bh));
                hit(x, mid - bh / 2, bw, bh, HitKind::Button);
                break;
            }

            case ControlKind::Header: break;
        }
    }

    void draw_check(fizmo::windows::Renderer& r, int x, int y, int size, const Color& c) {
        const auto pen = fizmo::graphics::Paint::stroke(c, static_cast<unsigned int>(vmax(2, px(2.0))));
        const int inset = px(CHECK_INSET);
        const int ax = x + inset, ay = y + size / 2;
        const int bx = x + size * 2 / 5, by = y + size - inset;
        const int cx = x + size - inset, cy = y + inset;
        r.draw_line(ax, ay, bx, by, pen);
        r.draw_line(bx, by, cx, cy, pen);
    }

    void draw_footer(fizmo::windows::Renderer& r, int x, int y, int w, int h, const MenuSettings& style) {
        r.draw_line(x, y, x + w, y, fizmo::graphics::Paint::stroke(faded(style.muted, DIM_ALPHA), 1));
        const int bw = px(BUTTON_W), bh = px(BUTTON_H), gap = px(GAP) * 2;
        const int done_x = x + w - px(PAD) - bw, by = y + (h - bh) / 2;
        const int reset_x = done_x - gap - bw;
        button_box(r, done_x, by, bw, bh, "Done", style.accent, Color(255, 255, 255), hot(done_x, by, bw, bh));
        button_box(r, reset_x, by, bw, bh, "Reset tab", style.control, style.text, hot(reset_x, by, bw, bh));
        add_hit(done_x, by, bw, bh, HitKind::Close);
        add_hit(reset_x, by, bw, bh, HitKind::ResetTab);
        const int text_x = x + px(PAD);
        const int text_w = reset_x - gap - text_x;
        std::string title, body;

        if (capturing()) {
            const SettingControl* c = control(m_capture);
            title = c ? "Binding: " + c->label : std::string("Binding");
            body = "Press the key to use. Esc cancels, clicking anywhere also cancels.";
        } else if (const SettingControl* c = control(m_hover_control)) {
            title = c->label;
            body = c->description;
            if (c->kind == ControlKind::Number) body += "  (Shift = 10x steps, scroll the slider to nudge)";
        } else {
            title = "Hover a setting to see what it does.";
            body = "Changes apply immediately" + std::string(m_save_note);
        }

        r.set_clip_rect(text_x, y, static_cast<unsigned int>(vmax(0, text_w)), static_cast<unsigned int>(h));
        text(r, text_x, y + h / 3, title, px(TEXT_SIZE), style.text, true);
        text(r, text_x, y + h * 2 / 3, body, px(SMALL_SIZE), style.muted);
        r.reset_clip_rect();
    }

public:
    void set_save_note(const char* note) noexcept { m_save_note = note; }

private:
    static int channel(const Color& c, int k) noexcept {
        switch (k) {
            case 0: return c.red();
            case 1: return c.green();
            case 2: return c.blue();
            default: return c.alpha();
        }
    }

    static Color with_channel(const Color& c, int k, int v) noexcept {
        const auto b = static_cast<std::uint8_t>(vclamp(v, 0, 255));
        switch (k) {
            case 0: return Color(b, c.green(), c.blue(), c.alpha());
            case 1: return Color(c.red(), b, c.blue(), c.alpha());
            case 2: return Color(c.red(), c.green(), b, c.alpha());
            default: return Color(c.red(), c.green(), c.blue(), b);
        }
    }

    static double fraction(const SettingControl& c, double v) {
        const NumberRange r = c.limits();
        if (r.max <= r.min) return 0.0;
        if (r.log && r.min > 0.0) return vclamp(std::log(vmax(v, r.min) / r.min) / std::log(r.max / r.min), 0.0, 1.0);
        return vclamp((v - r.min) / (r.max - r.min), 0.0, 1.0);
    }

    static double from_fraction(const SettingControl& c, double t) {
        const NumberRange r = c.limits();
        t = vclamp(t, 0.0, 1.0);
        if (r.log && r.min > 0.0) return r.min * std::pow(r.max / r.min, t);
        return r.min + (r.max - r.min) * t;
    }

    static double snapped(const SettingControl& c, double v) {
        const NumberRange r = c.limits();
        if (r.step > 0.0) v = std::round(v / r.step) * r.step;
        return vclamp(v, r.min, r.max);
    }

    static std::string format_number(double v, const NumberRange& r) {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%.*f", vmax(r.decimals, 0), v);
        std::string s = buf;
        if (r.unit && *r.unit) s += std::string(" ") + r.unit;
        return s;
    }

    void set_number(SettingControl& c, double v) {
        const double before = c.get_number();
        c.set_number(v);
        if (c.get_number() != before) changed(c);
    }

    void nudge(int index, double direction, bool shift) {
        SettingControl* c = control(index);
        if (!c || c->kind != ControlKind::Number || !c->enabled()) return;
        const NumberRange r = c->limits();
        const double step = r.step > 0.0 ? r.step : (r.max - r.min) / 100.0;
        set_number(*c, snapped(*c, c->get_number() + direction * step * (shift ? SHIFT_STEPS : 1.0)));
    }

    void drag_to(int mx) {
        SettingControl* c = control(m_drag.control);
        if (!c) return;
        const double t = static_cast<double>(mx - m_drag.x) / vmax(1, m_drag.w);

        if (m_drag.kind == HitKind::Slider) {
            set_number(*c, snapped(*c, from_fraction(*c, t)));
            return;
        }

        const Color before = c->get_color();
        const Color next = with_channel(before, m_drag.part, static_cast<int>(std::lround(vclamp(t, 0.0, 1.0) * CHANNEL_MAX)));
        if (next == before) return;
        c->set_color(next);
        changed(*c);
    }

    void press(const Hit& h, bool shift) {
        SettingControl* c = control(h.control);

        switch (h.kind) {
            case HitKind::Tab:
                m_tab = static_cast<std::size_t>(h.part);
                m_scroll = 0;
                return;
            case HitKind::Close:
                m_close_requested = true;
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
            case HitKind::Minus: nudge(h.control, -1.0, shift); break;
            case HitKind::Plus:  nudge(h.control, 1.0, shift); break;
            case HitKind::Slider: {
                auto it = m_track.find(h.control);
                if (it == m_track.end()) break;
                m_drag = { true, HitKind::Slider, h.control, 0, it->second.first, it->second.second };
                drag_to(m_mouse_x);
                break;
            }
            case HitKind::Channel: {
                auto it = m_channel.find(h.control * 4 + h.part);
                if (it == m_channel.end()) break;
                m_drag = { true, HitKind::Channel, h.control, h.part, it->second.first, it->second.second };
                drag_to(m_mouse_x);
                break;
            }
            case HitKind::Prev:
            case HitKind::Next: {
                const int n = static_cast<int>(c->options.size());
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

    static constexpr const char* CANCEL_KEY = "Escape";

    std::vector<SettingsTab>              m_tabs;
    std::vector<Hit>                      m_hits;
    std::vector<Row>                      m_rows;
    std::unordered_map<int, std::pair<int, int>> m_track;
    std::unordered_map<int, std::pair<int, int>> m_channel;
    std::unordered_map<std::string, unsigned int> m_widths;
    std::unordered_map<int, int>          m_heights;
    std::size_t   m_tab = 0;
    bool          m_open = false;
    bool          m_close_requested = false;
    int           m_capture = -1;
    bool          m_capture_add = false;
    Drag          m_drag;
    int           m_scroll = 0;
    int           m_scroll_steps = 0;
    int           m_hover_control = -1;
    int           m_mouse_x = 0, m_mouse_y = 0;
    int           m_row_h = 1;
    int           m_content_x = 0, m_content_y = 0, m_content_w = 0, m_content_h = 0;
    double        m_s = 1.0;
    std::uint32_t m_changes = Apply::Nothing;
    const char*   m_save_note = ".";
};

} // namespace voxelspire

#endif // VOXELSPIRE_UI_SETTINGS_MENU_HPP