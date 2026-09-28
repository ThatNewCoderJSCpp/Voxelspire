#ifndef VOXELSPIRE_RENDER_QUEUE_HPP
#define VOXELSPIRE_RENDER_QUEUE_HPP

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>
#include "projector.hpp"

namespace voxelspire {

namespace detail_rp {
    using namespace fizmo;
    using namespace fizmo::windows;
    using RenderPointType = RenderPoint;
}

using RenderPoint = detail_rp::RenderPointType;

inline RenderPoint to_render_point(const ScreenPoint& p) noexcept {
    const int x = static_cast<int>(std::lround(p.x));
    const int y = static_cast<int>(std::lround(p.y));
    return RenderPoint{ x, y }; 
}

enum class RenderLayer : std::uint8_t { Terrain, Entity };

struct DrawPolygon {
    std::array<ScreenPoint, Projector::MAX_POINTS> points{};
    std::uint8_t count = 0;
    RenderLayer  layer = RenderLayer::Terrain;
    Color        color;
    double       depth = 0.0; 
};

class RenderQueue {
public:
    void clear() noexcept { m_items.clear(); }
    std::size_t size() const noexcept { return m_items.size(); }

    void push(const ScreenPoint* pts, int count, const Color& color, double depth, RenderLayer layer = RenderLayer::Terrain) {
        if (count < 3) return;
        DrawPolygon p;
        p.count = static_cast<std::uint8_t>(count);
        for (int i = 0; i < count; ++i) p.points[i] = pts[i];
        p.layer = layer;
        p.color = color;
        p.depth = depth;
        m_items.push_back(p);
    }

    template <typename Overlay>
    void flush(fizmo::windows::Renderer& renderer, Overlay&& overlay, double overlay_depth) {
        sort();
        bool overlay_done = false;

        for (std::uint32_t idx : m_order) {
            const DrawPolygon& p = m_items[idx];
            if (!overlay_done && p.layer == RenderLayer::Entity && p.depth < overlay_depth) { overlay(); overlay_done = true; }
            draw(renderer, p);
        }

        if (!overlay_done) overlay();
    }

    void flush(fizmo::windows::Renderer& renderer) {
        sort();
        for (std::uint32_t idx : m_order) draw(renderer, m_items[idx]);
    }

private:
    void sort() {
        m_order.resize(m_items.size());
        for (std::size_t i = 0; i < m_order.size(); ++i) m_order[i] = static_cast<std::uint32_t>(i);
        std::sort(m_order.begin(), m_order.end(), [this](std::uint32_t a, std::uint32_t b) { return m_items[a].depth > m_items[b].depth; });
    }

    void draw(fizmo::windows::Renderer& renderer, const DrawPolygon& p) {
        m_scratch.clear();
        for (int i = 0; i < p.count; ++i) m_scratch.push_back(to_render_point(p.points[i]));
        renderer.draw_polygon(m_scratch, fizmo::graphics::Paint::fill_and_stroke(p.color, p.color, 1));
    }

    std::vector<DrawPolygon>   m_items;
    std::vector<std::uint32_t> m_order;
    std::vector<RenderPoint>   m_scratch;
};

struct FaceShading {
    double up = 1.0, down = 0.5, north_south = 0.8, east_west = 0.62;

    double factor(Face f) const noexcept {
        switch (f) {
            case Face::Up:    return up;
            case Face::Down:  return down;
            case Face::North:
            case Face::South: return north_south;
            default:          return east_west;
        }
    }

    static Color apply(const Color& c, double k) noexcept {
        auto ch = [k](std::uint8_t v) { return static_cast<std::uint8_t>(vclamp(v * k, 0.0, 255.0)); };
        return Color(ch(c.red()), ch(c.green()), ch(c.blue()), c.alpha());
    }
};

} // namespace voxelspire

#endif // VOXELSPIRE_RENDER_QUEUE_HPP
