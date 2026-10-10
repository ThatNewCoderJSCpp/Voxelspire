#ifndef VOXELSPIRE_RENDER_ITEM_ICON_HPP
#define VOXELSPIRE_RENDER_ITEM_ICON_HPP

#include <array>
#include <cmath>
#include <string>
#include <vector>
#include "../item/item.hpp"

namespace voxelspire {

class ItemIconPainter {
public:
    static constexpr double INSET       = 0.14;
    static constexpr double TOP_SHARE   = 0.29;
    static constexpr double SIDE_SHADE  = 0.78;
    static constexpr double FRONT_SHADE = 0.6;
    static constexpr double FLAT_INSET  = 0.2;
    static constexpr double LETTER      = 0.45;
    static constexpr int    OUTLINE     = 1;
    static constexpr int    MAX_ALPHA   = 255;
    static constexpr int    EDGE_ALPHA  = 128;
    static constexpr double HALF        = 0.5;

    void draw(fizmo::windows::Renderer& r, const Item& item, int x, int y, int size);

private:
    struct Shape {
        int size = -1;
        std::vector<fizmo::windows::RenderPoint> top, left, right, edge;
    };

    static Color shaded(const Color& c, double k);

    const Shape& shape(int size);

    static std::vector<fizmo::windows::RenderPoint> moved(const std::vector<fizmo::windows::RenderPoint>& pts, int x, int y);

    void cube(fizmo::windows::Renderer& r, const ItemProperties& p, int x, int y, int size);

    void flat(fizmo::windows::Renderer& r, const Item& item, int x, int y, int size);

    Shape m_shape;
};

} // namespace voxelspire

#endif // VOXELSPIRE_RENDER_ITEM_ICON_HPP