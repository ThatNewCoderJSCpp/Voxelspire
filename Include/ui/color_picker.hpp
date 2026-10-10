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

    void open(const SettingControl& c, int index);

    void close() noexcept { m_index = NO_CONTROL; }

    void sync(const Color& now);

    void produced(const Color& c) noexcept { m_last = c; }

    bool contains(int x, int y) const noexcept { return x >= m_x && x < m_x + m_w && y >= m_y && y < m_y + m_h; }

    bool on_wheel(int x, int y) const noexcept;

    Color pick_wheel(int x, int y);

    Color pick_value(int y);

    Color pick_alpha(int y);

    Color current() const noexcept { return ColorMath::from_hsv(m_hsv, m_alpha); }

    void render(
        MenuPainter& p, 
        const MenuSettings& style, 
        int ax, int ay, int aw, int ah, 
        const SettingControl& c,
        const PickerEdit& edit, 
        std::vector<Hit>& hits, 
        int mouse_x, int mouse_y
    );

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

    void ensure_textures();

    static void marker(MenuPainter& p, int x, int y);

    static void bar_marker(MenuPainter& p, int x, int y, int w, const MenuSettings& style);

    static void swatch(MenuPainter& p, int x, int y, int w, int h, const Color& c, const MenuSettings& style);

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