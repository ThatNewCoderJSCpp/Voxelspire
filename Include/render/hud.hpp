#ifndef VOXELSPIRE_RENDER_HUD_HPP
#define VOXELSPIRE_RENDER_HUD_HPP

#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <optional>
#include <string>
#include <vector>
#include "../core/settings.hpp"
#include "../world/voxel_raycast.hpp"
#include "world_renderer.hpp"

namespace voxelspire {

struct HudInfo {
    double      fps_average = 0.0;
    vector3d    position{};
    vector3d    velocity{};
    bool        on_ground = false;
    std::string camera_mode;
    WorldRenderStats stats;
    std::optional<RaycastHit> target;
    std::string target_name;
    bool        mouse_captured = false;
    std::string last_key;
    std::string movement_mode;
    std::size_t entity_count   = 0;
    std::size_t entity_contacts = 0;
    std::string air_model;
    double      terminal_velocity = 0.0;
};

class Hud {
public:
    void render(fizmo::windows::Renderer& r, unsigned int w, unsigned int h, const HudInfo& info, const RenderSettings& rs) const {
        draw_crosshair(r, w, h, rs);

        if (rs.show_debug_hud) {
            std::vector<std::string> lines;
            lines.push_back(format("FPS (avg): %.0f", info.fps_average));
            lines.push_back(format("Pos: %.2f / %.2f / %.2f", clean(info.position.x), clean(info.position.y), clean(info.position.z)));
            const double hspeed = std::sqrt(info.velocity.x * info.velocity.x + info.velocity.y * info.velocity.y);
            lines.push_back(format("Vel: %.2f / %.2f / %.2f", clean(info.velocity.x), clean(info.velocity.y), clean(info.velocity.z)));
            lines.push_back(format("Speed: %.2f b/s%s", hspeed, info.on_ground ? "  (ground)" : "  (air)"));
            lines.push_back("Camera: " + info.camera_mode);
            lines.push_back(format("Faces: %zu drawn / %zu exposed", info.stats.faces_drawn, info.stats.faces_total));
            lines.push_back(format("Quads: %zu drawn / %zu total", info.stats.quads_drawn, info.stats.quads_total));
            lines.push_back(format("Chunks: %zu / %zu in view", info.stats.chunks_visible, info.stats.chunks_total));
            lines.push_back(format("Render distance: %.0f blocks", info.stats.render_distance));
            lines.push_back("Movement: " + info.movement_mode);
            lines.push_back(format("Entities: %zu (%zu touching)", info.entity_count, info.entity_contacts));
            
            if (std::isfinite(info.terminal_velocity))
                lines.push_back(format("Air: %s (terminal %.1f b/s)", info.air_model.c_str(), info.terminal_velocity));
            else
                lines.push_back(format("Air: %s (no terminal velocity)", info.air_model.c_str()));

            if (info.target)
                lines.push_back(format("Target: %s (%d, %d, %d) %s", info.target_name.c_str(), info.target->block.x, info.target->block.y, info.target->block.z, face_name(info.target->face)));
            else
                lines.push_back("Target: none");

            draw_panel_top_right(r, w, lines, rs);
        }

        std::string hint = info.mouse_captured
            ? "WASD move   Space jump   Shift sprint   Ctrl Crouch   Z prone   F5 camera   R respawn   -/= render distance   F3 HUD   Esc release mouse"
            : "Click to capture the mouse";

        if (rs.show_last_key && !info.last_key.empty()) hint += "      last key: \"" + info.last_key + "\"";
        draw_text_line(r, 10, static_cast<int>(h) - 28, hint, rs, Color(255, 255, 255));
    }

private:
    static double clean(double v) noexcept { return std::fabs(v) < 0.005 ? 0.0 : v; }

    static std::string format(const char* fmt, ...) {
        char buf[256];
        va_list args;
        va_start(args, fmt);
        std::vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);
        return buf;
    }

    static fizmo::text::TextStyle style(const RenderSettings& rs, const Color& c) {
        fizmo::text::TextStyle s(rs.hud_text_size, c);
        s.set_shadow(1.0, 1.0, 0.0, Color(0, 0, 0, 200));
        return s;
    }

    static void draw_text_line(fizmo::windows::Renderer& r, int x, int y, const std::string& text, const RenderSettings& rs, const Color& c) {
        r.draw_text(x, y, text, style(rs, c));
    }

    static void draw_panel_top_right(fizmo::windows::Renderer& r, unsigned int w, const std::vector<std::string>& lines, const RenderSettings& rs) {
        const auto st = style(rs, Color(255, 255, 255));
        const int pad = 8, margin = 10;
        const int line_h = static_cast<int>(std::lround(rs.hud_text_size * 1.35));
        unsigned int max_w = 0;
        std::vector<unsigned int> widths;

        for (const auto& l : lines) {
            unsigned int lw = r.measure_text(l, st).width;
            if (lw == 0) lw = static_cast<unsigned int>(static_cast<double>(l.size()) * rs.hud_text_size * 0.55);
            widths.push_back(lw);
            max_w = vmax(max_w, lw);
        }

        const int box_w = static_cast<int>(max_w) + pad * 2;
        const int box_h = line_h * static_cast<int>(lines.size()) + pad * 2;
        const int box_x = static_cast<int>(w) - box_w - margin;
        r.draw_rect(box_x, margin, static_cast<unsigned int>(box_w), static_cast<unsigned int>(box_h), fizmo::graphics::Paint::fill(Color(0, 0, 0, 120)));

        for (std::size_t i = 0; i < lines.size(); ++i) {
            const int x = box_x + box_w - pad - static_cast<int>(widths[i]);
            r.draw_text(x, margin + pad + static_cast<int>(i) * line_h, lines[i], st);
        }
    }

    static void draw_crosshair(fizmo::windows::Renderer& r, unsigned int w, unsigned int h, const RenderSettings& rs) {
        const int cx = static_cast<int>(w / 2), cy = static_cast<int>(h / 2), s = 8;
        const auto p = fizmo::graphics::Paint::stroke(rs.crosshair_color, 2);
        r.draw_line(cx - s, cy, cx + s, cy, p);
        r.draw_line(cx, cy - s, cx, cy + s, p);
    }
};

} // namespace voxelspire

#endif // VOXELSPIRE_RENDER_HUD_HPP