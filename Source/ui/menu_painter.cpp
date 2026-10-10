#include "ui/menu_painter.hpp"

namespace voxelspire {

Color MenuPainter::faded(const Color& c, int alpha) { return Color(c.red(), c.green(), c.blue(), static_cast<std::uint8_t>(vclamp(alpha, 0, MAX_ALPHA))); }

Color MenuPainter::lighter(const Color& c) {
    auto up = [](std::uint8_t v) { return static_cast<std::uint8_t>(vmin(MAX_ALPHA, v + LIGHTEN)); };
    return Color(up(c.red()), up(c.green()), up(c.blue()), c.alpha());
}

unsigned int MenuPainter::width(const std::string& s, int size, bool bold) {
    if (s.empty()) return 0;
    if (m_widths.size() > MAX_CACHED) m_widths.clear();
    const std::string key = std::to_string(size) + (bold ? "b" : "r") + s;
    auto it = m_widths.find(key);
    if (it != m_widths.end()) return it->second;
    const unsigned int wd = m_r->measure_text(s, style_of(size, Color(), bold)).width;
    m_widths.emplace(key, wd);
    return wd;
}

int MenuPainter::line_height(int size) {
    auto it = m_heights.find(size);
    if (it != m_heights.end()) return it->second;
    const unsigned int ht = m_r->measure_text("Hg", style_of(size, Color(), false)).height;
    const int value = ht > 0 ? static_cast<int>(ht) : size;
    m_heights.emplace(size, value);
    return value;
}

void MenuPainter::rect(int x, int y, int w, int h, const Color& c) {
    if (w <= 0 || h <= 0) return;
    m_r->draw_rect(x, y, static_cast<unsigned int>(w), static_cast<unsigned int>(h), fill(c));
}

void MenuPainter::outline(int x, int y, int w, int h, const Color& c) {
    const int t = vmax(1, px(BORDER));
    rect(x, y, w, t, c);
    rect(x, y + h - t, w, t, c);
    rect(x, y, t, h, c);
    rect(x + w - t, y, t, h, c);
}

void MenuPainter::checker(int x, int y, int w, int h, int cell, const Color& a, const Color& b) {
    cell = vmax(cell, 1);

    for (int cy = 0; cy < h; cy += cell)
        for (int cx = 0; cx < w; cx += cell)
            rect(x + cx, y + cy, vmin(cell, w - cx), vmin(cell, h - cy), ((cx / cell + cy / cell) % 2 == 0) ? a : b);
}

void MenuPainter::text(int x, int center_y, const std::string& s, int size, const Color& c, bool bold) {
    if (s.empty()) return;
    m_r->draw_text(x, center_y - line_height(size) / 2, s, style_of(size, c, bold));
}

void MenuPainter::field(int x, int y, int w, int h, const std::string& shown, const std::string& suffix, const FieldLook& look) {
    rect(x, y, w, h, look.hot && !look.editing ? lighter(look.fill) : look.fill);
    if (look.editing) outline(x, y, w, h, look.accent);
    else if (look.failed) outline(x, y, w, h, look.error);
    const int pad = px(FIELD_PAD), mid = y + h / 2;
    const int text_w = static_cast<int>(width(shown, look.size));
    const int suffix_w = suffix.empty() ? 0 : static_cast<int>(width(" " + suffix, look.size));
    const int tx = look.centered ? x + (w - text_w - suffix_w) / 2 : x + pad;
    if (look.editing && look.all_selected && !shown.empty()) rect(tx - 1, mid - line_height(look.size) / 2, text_w + 2, line_height(look.size), faded(look.accent, SELECT_ALPHA));
    text(tx, mid, shown, look.size, look.text);
    if (!suffix.empty() && !look.editing) text(tx + text_w, mid, " " + suffix, look.size, look.muted);

    if (look.editing && look.caret_on && !look.all_selected) {
        const int cx = tx + static_cast<int>(width(shown.substr(0, look.caret), look.size));
        const int ch = line_height(look.size);
        rect(cx, mid - ch / 2, vmax(1, px(CARET_W)), ch, look.text);
    }
}

} // namespace voxelspire
