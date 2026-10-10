#ifndef VOXELSPIRE_UI_MENU_PAINTER_HPP
#define VOXELSPIRE_UI_MENU_PAINTER_HPP

#include <cmath>
#include <cstdint>
#include <string>
#include <unordered_map>
#include "../core/types.hpp"

namespace voxelspire {

enum class HitKind : std::uint8_t {
    Tab = 0, Toggle, Slider, Minus, Plus, Value, Prev, Next, Swatch, Hex, Channel, Change, AddKey, Unbind, Button, Reset, ResetTab, Close,
    PickerPanel, Wheel, ValueBar, AlphaBar, PickerHex, PickerChannel, PickerDone, PickerCancel, TextValue, Cancel, Group
};

struct Hit {
    int     x = 0, y = 0, w = 0, h = 0;
    HitKind kind = HitKind::Close;
    int     control = -1;
    int     part = 0;

    bool contains(int px_, int py_) const noexcept { return px_ >= x && px_ < x + w && py_ >= y && py_ < y + h; }
};

struct FieldLook {
    Color       fill;
    Color       text;
    Color       muted;
    Color       accent;
    Color       error;
    int         size         = 0;
    bool        editing      = false;
    bool        all_selected = false;
    std::size_t caret        = 0;
    bool        caret_on     = false;
    bool        failed       = false;
    bool        hot          = false;
    bool        centered     = false;
};

class MenuPainter {
public:
    static constexpr std::size_t MAX_CACHED   = 4096;
    static constexpr int         MAX_ALPHA    = 255;
    static constexpr int         LIGHTEN      = 22;
    static constexpr double      FIELD_PAD    = 6.0;
    static constexpr double      CARET_W      = 1.5;
    static constexpr double      BORDER       = 1.5;
    static constexpr int         SELECT_ALPHA = 110;

    void begin(fizmo::windows::Renderer& r, double scale) noexcept { m_r = &r; m_s = scale; }

    fizmo::windows::Renderer& renderer() noexcept { return *m_r; }
    double scale() const noexcept { return m_s; }
    int px(double v) const noexcept { return static_cast<int>(std::lround(v * m_s)); }

    static fizmo::graphics::Paint fill(const Color& c) { return fizmo::graphics::Paint::fill(c); }
    static Color faded(const Color& c, int alpha);

    static Color lighter(const Color& c);

    static fizmo::text::TextStyle style_of(int size, const Color& c, bool bold) {
        fizmo::text::TextStyle s(static_cast<double>(size), c);
        if (bold) s.set_bold();
        return s;
    }

    unsigned int width(const std::string& s, int size, bool bold = false);

    int line_height(int size);

    void rect(int x, int y, int w, int h, const Color& c);

    void outline(int x, int y, int w, int h, const Color& c);

    void checker(int x, int y, int w, int h, int cell, const Color& a, const Color& b);

    void text(int x, int center_y, const std::string& s, int size, const Color& c, bool bold = false);

    void text_centered(int cx, int cy, const std::string& s, int size, const Color& c, bool bold = false) {
        text(cx - static_cast<int>(width(s, size, bold)) / 2, cy, s, size, c, bold);
    }

    void button(int x, int y, int w, int h, const std::string& label, const Color& bg, const Color& fg, bool hot, int size) {
        rect(x, y, w, h, hot ? lighter(bg) : bg);
        text_centered(x + w / 2, y + h / 2, label, size, fg);
    }

    void field(int x, int y, int w, int h, const std::string& shown, const std::string& suffix, const FieldLook& look);

private:
    fizmo::windows::Renderer*                     m_r = nullptr;
    double                                        m_s = 1.0;
    std::unordered_map<std::string, unsigned int> m_widths;
    std::unordered_map<int, int>                  m_heights;
};

} // namespace voxelspire

#endif // VOXELSPIRE_UI_MENU_PAINTER_HPP