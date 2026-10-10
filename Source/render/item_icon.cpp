#include "render/item_icon.hpp"

namespace voxelspire {

void ItemIconPainter::draw(fizmo::windows::Renderer& r, const Item& item, int x, int y, int size) {
    const ItemProperties& p = item.properties();
    if (p.look == ItemLook::Cube) cube(r, p, x, y, size);
    else flat(r, item, x, y, size);
}

Color ItemIconPainter::shaded(const Color& c, double k) {
    auto s = [k](std::uint8_t v) { return static_cast<std::uint8_t>(vclamp(std::lround(v * k), 0L, static_cast<long>(MAX_ALPHA))); };
    return Color(s(c.red()), s(c.green()), s(c.blue()), MAX_ALPHA);
}

auto ItemIconPainter::shape(int size) -> const Shape& {
    if (m_shape.size == size) return m_shape;
    const double in = size * INSET, w = size - in * 2.0;
    const double cx = size * HALF, top = in, mid = in + w * TOP_SHARE, low = in + w * TOP_SHARE * 2.0, bottom = size - in;
    const double l = in, rr = size - in;
    m_shape.size  = size;
    m_shape.top   = { { cx, top }, { rr, mid }, { cx, low }, { l, mid } };
    m_shape.left  = { { l, mid }, { cx, low }, { cx, bottom }, { l, bottom - (low - mid) } };
    m_shape.right = { { cx, low }, { rr, mid }, { rr, bottom - (low - mid) }, { cx, bottom } };
    m_shape.edge  = { { cx, top }, { rr, mid }, { rr, bottom - (low - mid) }, { cx, bottom }, { l, bottom - (low - mid) }, { l, mid } };
    return m_shape;
}

std::vector<fizmo::windows::RenderPoint> ItemIconPainter::moved(const std::vector<fizmo::windows::RenderPoint>& pts, int x, int y) {
    std::vector<fizmo::windows::RenderPoint> out = pts;
    for (auto& p : out) { p.x += static_cast<float>(x); p.y += static_cast<float>(y); }
    return out;
}

void ItemIconPainter::cube(fizmo::windows::Renderer& r, const ItemProperties& p, int x, int y, int size) {
    const Shape& s = shape(size);
    r.draw_polygon(moved(s.left, x, y), fizmo::graphics::Paint::fill(shaded(p.side, SIDE_SHADE)));
    r.draw_polygon(moved(s.right, x, y), fizmo::graphics::Paint::fill(shaded(p.front, FRONT_SHADE)));
    r.draw_polygon(moved(s.top, x, y), fizmo::graphics::Paint::fill(shaded(p.color, 1.0)));
    r.draw_polyline(moved(s.edge, x, y), fizmo::graphics::Paint::stroke(Color(0, 0, 0, EDGE_ALPHA), OUTLINE), true);
}

void ItemIconPainter::flat(fizmo::windows::Renderer& r, const Item& item, int x, int y, int size) {
    const int in = static_cast<int>(std::lround(size * FLAT_INSET)), w = size - in * 2;
    const ItemProperties& p = item.properties();
    r.draw_rect(x + in, y + in, static_cast<unsigned int>(w), static_cast<unsigned int>(w), fizmo::graphics::Paint::fill(shaded(p.color, 1.0)));
    r.draw_rect(x + in, y + in, static_cast<unsigned int>(w), static_cast<unsigned int>(w), fizmo::graphics::Paint::stroke(Color(0, 0, 0, EDGE_ALPHA), OUTLINE));
    const std::string letter = item.name().substr(0, 1);
    fizmo::text::TextStyle st(size * LETTER, shaded(p.color, FRONT_SHADE));
    st.set_bold();
    const auto m = r.measure_text(letter, st);
    r.draw_text(x + (size - static_cast<int>(m.width)) / 2, y + (size - static_cast<int>(m.height)) / 2, letter, st);
}

} // namespace voxelspire
