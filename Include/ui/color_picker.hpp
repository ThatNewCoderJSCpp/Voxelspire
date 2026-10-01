#ifndef VOXELSPIRE_UI_COLOR_PICKER_HPP
#define VOXELSPIRE_UI_COLOR_PICKER_HPP

#include <cmath>
#include <string>
#include <vector>
#include "color_math.hpp"
#include "menu_painter.hpp"
#include "settings_model.hpp"
#include "text_field.hpp"

namespace voxelspire {

struct PickerEdit {
    static constexpr int NONE = -1;

    int              part = NONE;
    const TextField* field = nullptr;
    bool             caret_on = false;
    int              failed_part = NONE;
};

class ColorPicker {
public:
    static constexpr int HEX_PART   = 4;
    static constexpr int CHANNELS   = 4;
    static constexpr int NO_CONTROL = -1;

    bool  is_open() const noexcept { return m_index != NO_CONTROL; }
    int   control() const noexcept { return m_index; }
    Color original() const noexcept { return m_original; }

    void open(const SettingControl& c, int index) {
        m_index    = index;
        m_original = c.get_color();
        m_hsv      = ColorMath::to_hsv(m_original);
        m_alpha    = m_original.alpha();
        m_last     = m_original;
    }

    void close() noexcept { m_index = NO_CONTROL; }

    void sync(const Color& now) {
        if (now == m_last) return;
        const Hsv next = ColorMath::to_hsv(now);
        if (next.v > 0.0) {
            if (next.s > 0.0) m_hsv.h = next.h;
            m_hsv.s = next.s;
        }
        m_hsv.v = next.v;
        m_alpha = now.alpha();
        m_last  = now;
    }

    void produced(const Color& c) noexcept { m_last = c; }

    bool contains(int x, int y) const noexcept { return x >= m_x && x < m_x + m_w && y >= m_y && y < m_y + m_h; }

    bool on_wheel(int x, int y) const noexcept {
        const double dx = x - m_wheel_cx, dy = y - m_wheel_cy;
        return dx * dx + dy * dy <= static_cast<double>(m_wheel_r) * m_wheel_r * WHEEL_GRAB * WHEEL_GRAB;
    }

    Color pick_wheel(int x, int y) {
        const double dx = x - m_wheel_cx, dy = y - m_wheel_cy;
        const double r = std::sqrt(dx * dx + dy * dy) / vmax(1, m_wheel_r);
        double h = std::atan2(dy, dx) / FULL_TURN;
        if (h < 0.0) h += 1.0;
        m_hsv.h = h;
        m_hsv.s = vclamp(r, 0.0, 1.0);
        if (m_hsv.v <= 0.0) m_hsv.v = 1.0;
        return current();
    }

    Color pick_value(int y) {
        m_hsv.v = 1.0 - vclamp(static_cast<double>(y - m_bar_y) / vmax(1, m_bar_h), 0.0, 1.0);
        return current();
    }

    Color pick_alpha(int y) {
        m_alpha = ColorMath::byte(1.0 - vclamp(static_cast<double>(y - m_bar_y) / vmax(1, m_bar_h), 0.0, 1.0));
        return current();
    }

    Color current() const noexcept { return ColorMath::from_hsv(m_hsv, m_alpha); }

    void render(
        MenuPainter& p, 
        const MenuSettings& style, 
        int ax, int ay, int aw, int ah, 
        const SettingControl& c,
        const PickerEdit& edit, 
        std::vector<Hit>& hits, 
        int mouse_x, int mouse_y
    ) {
        ensure_textures();
        const Color now = c.get_color();
        sync(now);
        m_w = vmin(p.px(PANEL_W), aw);
        m_h = vmin(p.px(PANEL_H), ah);
        m_x = ax + (aw - m_w) / 2;
        m_y = ay + (ah - m_h) / 2;
        auto hot = [&](int x, int y, int w, int h) { return mouse_x >= x && mouse_x < x + w && mouse_y >= y && mouse_y < y + h; };
        const int pad = p.px(PAD), text_size = p.px(TEXT_SIZE), small = p.px(SMALL_SIZE);
        p.rect(ax, ay, aw, ah, MenuPainter::faded(Color(0, 0, 0), SHADE_ALPHA));
        p.rect(m_x, m_y, m_w, m_h, style.panel);
        p.outline(m_x, m_y, m_w, m_h, style.accent);
        hits.push_back({ m_x, m_y, m_w, m_h, HitKind::PickerPanel, m_index, 0 });
        p.text(m_x + pad, m_y + pad + p.px(TITLE_H) / 2, "Color: " + c.label, text_size, style.text, true);
        const int top = m_y + pad + p.px(TITLE_H);
        const int wheel = p.px(WHEEL);
        const int wx = m_x + pad, wy = top;
        m_wheel_cx = wx + wheel / 2;
        m_wheel_cy = wy + wheel / 2;
        m_wheel_r  = wheel / 2;
        p.renderer().draw_texture(m_wheel, wx, wy, static_cast<unsigned int>(wheel), static_cast<unsigned int>(wheel));
        p.renderer().draw_texture(m_shade, wx, wy, static_cast<unsigned int>(wheel), static_cast<unsigned int>(wheel), static_cast<float>(1.0 - m_hsv.v));
        const double angle = m_hsv.h * FULL_TURN;
        const int mx = m_wheel_cx + static_cast<int>(std::lround(std::cos(angle) * m_hsv.s * m_wheel_r));
        const int my = m_wheel_cy + static_cast<int>(std::lround(std::sin(angle) * m_hsv.s * m_wheel_r));
        marker(p, mx, my);
        hits.push_back({ wx, wy, wheel, wheel, HitKind::Wheel, m_index, 0 });
        const int bar_w = p.px(BAR_W), gap = p.px(BAR_GAP);
        m_bar_y = wy;
        m_bar_h = wheel;
        const int vx = wx + wheel + gap;
        Hsv full = m_hsv;

        for (int i = 0; i < STRIPS; ++i) {
            const int y0 = wy + wheel * i / STRIPS, y1 = wy + wheel * (i + 1) / STRIPS;
            full.v = 1.0 - (i + HALF) / STRIPS;
            p.rect(vx, y0, bar_w, y1 - y0, ColorMath::from_hsv(full, SOLID));
        }

        bar_marker(p, vx, wy + static_cast<int>(std::lround((1.0 - m_hsv.v) * wheel)), bar_w, style);
        hits.push_back({ vx, wy, bar_w, wheel, HitKind::ValueBar, m_index, 0 });
        int column = vx + bar_w + gap;

        if (c.with_alpha) {
            const Color solid = ColorMath::from_hsv(m_hsv, SOLID);
            p.checker(column, wy, bar_w, wheel, p.px(CHECKER), style.control, style.muted);

            for (int i = 0; i < STRIPS; ++i) {
                const int y0 = wy + wheel * i / STRIPS, y1 = wy + wheel * (i + 1) / STRIPS;
                p.rect(column, y0, bar_w, y1 - y0, MenuPainter::faded(solid, ColorMath::byte(1.0 - (i + HALF) / STRIPS)));
            }

            bar_marker(p, column, wy + static_cast<int>(std::lround((1.0 - m_alpha / ColorMath::CHANNEL) * wheel)), bar_w, style);
            hits.push_back({ column, wy, bar_w, wheel, HitKind::AlphaBar, m_index, 0 });
            column += bar_w + gap;
        }

        const int rx = column + p.px(COLUMN_GAP) - gap;
        const int rw = m_x + m_w - pad - rx;
        const int row = p.px(ROW_H), field_h = p.px(FIELD_H);
        const int label_w = p.px(LABEL_W);
        int y = top;
        const int sw = (rw - gap) / 2, sh = p.px(SWATCH_H);
        swatch(p, rx, y, sw, sh, now, style);
        swatch(p, rx + sw + gap, y, sw, sh, m_original, style);
        p.text_centered(rx + sw / 2, y + sh + small, "New", small, style.muted);
        p.text_centered(rx + sw + gap + sw / 2, y + sh + small, "Before", small, style.muted);
        y += sh + small * 2 + gap;
        FieldLook look;
        look.fill = style.control; look.text = style.text; look.muted = style.muted; look.accent = style.accent; look.error = style.error; look.size = small;
        const int fw = rw - label_w;

        auto box = [&](const char* name, const std::string& value, int part, HitKind kind) {
            p.text(rx, y + field_h / 2, name, small, style.muted);
            FieldLook l = look;
            l.editing = edit.part == part && edit.field;
            l.failed = edit.failed_part == part;
            l.hot = hot(rx + label_w, y, fw, field_h);
            if (l.editing) { l.all_selected = edit.field->all_selected(); l.caret = edit.field->caret(); l.caret_on = edit.caret_on; }
            p.field(rx + label_w, y, fw, field_h, l.editing ? edit.field->text() : value, std::string(), l);
            hits.push_back({ rx + label_w, y, fw, field_h, kind, m_index, part });
            y += row;
        };

        box("Hex", ColorMath::to_hex(now, c.with_alpha), HEX_PART, HitKind::PickerHex);
        const char* names[CHANNELS] = { "Red", "Green", "Blue", "Opacity" };
        for (int k = 0; k < (c.with_alpha ? CHANNELS : CHANNELS - 1); ++k) box(names[k], std::to_string(ColorMath::channel(now, k)), k, HitKind::PickerChannel);
        const int bw = (rw - gap) / 2, bh = p.px(BUTTON_H), by = m_y + m_h - pad - bh;
        p.button(rx, by, bw, bh, "Done", style.accent, Color(255, 255, 255), hot(rx, by, bw, bh), small);
        p.button(rx + bw + gap, by, bw, bh, "Cancel", style.control, style.text, hot(rx + bw + gap, by, bw, bh), small);
        hits.push_back({ rx, by, bw, bh, HitKind::PickerDone, m_index, 0 });
        hits.push_back({ rx + bw + gap, by, bw, bh, HitKind::PickerCancel, m_index, 0 });
    }

private:
    static constexpr double       PANEL_W     = 560.0;
    static constexpr double       PANEL_H     = 330.0;
    static constexpr double       PAD         = 18.0;
    static constexpr double       TITLE_H     = 30.0;
    static constexpr double       WHEEL       = 240.0;
    static constexpr double       BAR_W       = 24.0;
    static constexpr double       BAR_GAP     = 12.0;
    static constexpr double       COLUMN_GAP  = 22.0;
    static constexpr double       ROW_H       = 32.0;
    static constexpr double       FIELD_H     = 26.0;
    static constexpr double       LABEL_W     = 70.0;
    static constexpr double       SWATCH_H    = 34.0;
    static constexpr double       BUTTON_H    = 30.0;
    static constexpr double       CHECKER     = 6.0;
    static constexpr double       TEXT_SIZE   = 16.0;
    static constexpr double       SMALL_SIZE  = 13.0;
    static constexpr double       MARKER      = 7.0;
    static constexpr double       MARKER_EDGE = 2.0;
    static constexpr double       BAR_MARK_H  = 4.0;
    static constexpr double       WHEEL_GRAB  = 1.08;
    static constexpr double       FULL_TURN   = 2.0 * PI;
    static constexpr double       HALF        = 0.5;
    static constexpr double       EDGE_SOFT   = 1.5;
    static constexpr int          STRIPS      = 64;
    static constexpr int          SHADE_ALPHA = 120;
    static constexpr int          TEXELS      = 256;
    static constexpr std::uint8_t SOLID       = 255;

    void ensure_textures() {
        if (m_wheel.valid()) return;
        fizmo::images::BitmapImage wheel(TEXELS, TEXELS), shade(TEXELS, TEXELS);
        const double c = (TEXELS - 1) * HALF;

        for (int y = 0; y < TEXELS; ++y)
            for (int x = 0; x < TEXELS; ++x) {
                const double dx = (x - c) / c, dy = (y - c) / c;
                const double r = std::sqrt(dx * dx + dy * dy);
                const double coverage = vclamp((1.0 - r) * c / EDGE_SOFT + HALF, 0.0, 1.0);
                const std::uint8_t a = ColorMath::byte(coverage);
                double h = std::atan2(dy, dx) / FULL_TURN;
                if (h < 0.0) h += 1.0;
                wheel.set_pixel(static_cast<unsigned int>(x), static_cast<unsigned int>(y), ColorMath::from_hsv({ h, vmin(r, 1.0), 1.0 }, a));
                shade.set_pixel(static_cast<unsigned int>(x), static_cast<unsigned int>(y), Color(0, 0, 0, a));
            }

        m_wheel = fizmo::graphics::Texture(std::move(wheel), fizmo::graphics::SampleFilter::Bilinear);
        m_shade = fizmo::graphics::Texture(std::move(shade), fizmo::graphics::SampleFilter::Bilinear);
    }

    static void marker(MenuPainter& p, int x, int y) {
        const unsigned int r = static_cast<unsigned int>(vmax(3, p.px(MARKER)));
        const unsigned int t = static_cast<unsigned int>(vmax(1, p.px(MARKER_EDGE)));
        p.renderer().draw_circle(x, y, r + t, fizmo::graphics::Paint::stroke(Color(0, 0, 0), t));
        p.renderer().draw_circle(x, y, r, fizmo::graphics::Paint::stroke(Color(255, 255, 255), t));
    }

    static void bar_marker(MenuPainter& p, int x, int y, int w, const MenuSettings& style) {
        const int h = vmax(2, p.px(BAR_MARK_H));
        p.rect(x - h / 2, y - h / 2, w + h, h, Color(255, 255, 255));
        p.outline(x - h / 2, y - h / 2, w + h, h, style.panel);
    }

    static void swatch(MenuPainter& p, int x, int y, int w, int h, const Color& c, const MenuSettings& style) {
        p.checker(x, y, w, h, p.px(CHECKER), style.control, style.muted);
        p.rect(x, y, w, h, c);
        p.outline(x, y, w, h, style.muted);
    }

    int                      m_index = NO_CONTROL;
    Color                    m_original;
    Color                    m_last;
    Hsv                      m_hsv;
    std::uint8_t             m_alpha = SOLID;
    fizmo::graphics::Texture m_wheel;
    fizmo::graphics::Texture m_shade;
    int                      m_x = 0, m_y = 0, m_w = 0, m_h = 0;
    int                      m_wheel_cx = 0, m_wheel_cy = 0, m_wheel_r = 1;
    int                      m_bar_y = 0, m_bar_h = 1;
};

} // namespace voxelspire

#endif // VOXELSPIRE_UI_COLOR_PICKER_HPP