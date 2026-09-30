#ifndef VOXELSPIRE_RENDER_HUD_HPP
#define VOXELSPIRE_RENDER_HUD_HPP

#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include "../core/settings.hpp"
#include "../world/chunk_streamer.hpp"
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
    StreamingStats streaming;
    int         simulation_distance = 0;
    int         detail_distance     = 0;
    int         worker_threads      = 0;
    std::size_t particles           = 0;
    std::size_t particle_emitters   = 0;
    std::uint64_t seed              = 0;
    LightStats    light;
    std::string   lighting_preset;
    double        time_hours        = 0.0;
    std::int64_t  day               = 0;
    bool          day_cycle         = true;
    LightLevel    light_here;
    std::size_t   dynamic_lights    = 0;
    std::size_t   shadow_casters    = 0;
    std::string   fluid_model;
    std::size_t   reflection_planes = 0;
};

struct HudRow {
    std::string label;
    std::string value;
    bool        header = false;
};

class Hud {
public:
    static constexpr double FALLBACK_CHAR_WIDTH = 0.55;
    static constexpr double MINUTES_PER_HOUR    = 60.0;
    static constexpr double SCALE_STEP          = 0.1;
    static constexpr double MIN_SCALE           = 0.5;
    static constexpr double MAX_SCALE           = 3.0;
    static constexpr std::size_t MAX_CACHED_WIDTHS = 4096;
    static constexpr int    CORNER_COUNT        = 4;

    static HudCorner next_corner(HudCorner c) noexcept { return static_cast<HudCorner>((static_cast<int>(c) + 1) % CORNER_COUNT); }

    static double stepped_scale(double scale, int direction) noexcept {
        return vclamp(std::round((scale + SCALE_STEP * direction) / SCALE_STEP) * SCALE_STEP, MIN_SCALE, MAX_SCALE);
    }

    bool due(double dt, const HudSettings& hs) noexcept {
        m_elapsed += dt;
        if (!m_primed || m_elapsed >= hs.refresh_interval) { m_elapsed = 0.0; m_primed = true; return true; }
        return false;
    }

    void set_info(const HudInfo& info, const HudSettings& hs) {
        m_rows.clear();
        const WorldRenderStats& st = info.stats;
        const TerrainStats& t = st.terrain;
        constexpr double MB = 1024.0 * 1024.0;

        if (hs.sections.performance) {
            header("Performance");
            row("FPS", format("%.0f", info.fps_average));
            row("Draws", format("%zu meshes in %zu batches", st.batch_items, st.batches));
            row("Jobs", format("%zu gen, %zu mesh, %zu LOD, %zu uploads (%d threads)", info.streaming.jobs_in_flight, t.mesh_jobs, t.lod_jobs, t.pending_uploads, info.worker_threads));
            row("Mesh memory", format("%.1f MB GPU, %.1f MB RAM", t.gpu_mesh_bytes / MB, t.cpu_mesh_bytes / MB));
        }

        if (hs.sections.player) {
            header("Player");
            row("Position", format("%.2f  %.2f  %.2f", clean(info.position.x), clean(info.position.y), clean(info.position.z)));
            row("Velocity", format("%.2f  %.2f  %.2f", clean(info.velocity.x), clean(info.velocity.y), clean(info.velocity.z)));
            const double hspeed = std::sqrt(info.velocity.x * info.velocity.x + info.velocity.y * info.velocity.y);
            row("Speed", format("%.2f b/s %s", hspeed, info.on_ground ? "(ground)" : "(air)"));
            row("Movement", info.movement_mode);
            row("Camera", info.camera_mode);
            if (info.target) row("Target", format("%s at %d %d %d, %s face", info.target_name.c_str(), info.target->block.x, info.target->block.y, info.target->block.z, face_name(info.target->face)));
            else row("Target", "none");
        }

        if (hs.sections.world) {
            header("World");
            row("Seed", format("%llu", static_cast<unsigned long long>(info.seed)));
            const int hour = static_cast<int>(info.time_hours), minute = static_cast<int>((info.time_hours - hour) * MINUTES_PER_HOUR);
            row("Time", format("%02d:%02d, day %lld%s", hour, minute, static_cast<long long>(info.day), info.day_cycle ? "" : " (paused)"));
            row("Columns", format("%zu loaded, %zu stored edits", info.streaming.loaded_columns, info.streaming.stored_chunks));
            row("Entities", format("%zu (%zu touching)", info.entity_count, info.entity_contacts));
            row("Particles", format("%zu (%zu emitters)", info.particles, info.particle_emitters));
            if (std::isfinite(info.terminal_velocity)) row("Air", format("%s, terminal %.1f b/s", info.air_model.c_str(), info.terminal_velocity));
            else row("Air", info.air_model + ", no terminal velocity");
            row("Fluids", info.fluid_model);
        }

        if (hs.sections.rendering) {
            header("Rendering");
            row("Distance", format("%.0f blocks, detail %d, simulation %d chunks", st.render_distance, info.detail_distance, info.simulation_distance));
            row("Chunks", format("%zu drawn / %zu meshed, %zu cave-culled, %zu see-through", st.chunks_visible, st.chunks_total, st.chunks_cave_culled, st.translucent_visible));
            row("Quads", format("%zu drawn / %zu, %zu facing away", st.quads_drawn, st.quads_total, st.quads_facing_away));
            row("LOD tiles", format("%zu drawn / %zu, %zu quads", st.lod_tiles_drawn, st.lod_tiles_total, st.lod_quads_drawn));
            row("Reflections", format("%zu planes, %zu extra chunks", info.reflection_planes, st.chunks_reflected));
        }

        if (hs.sections.lighting) {
            header("Lighting");
            row("Preset", format("%s, %s light", info.lighting_preset.c_str(), light_format_name(info.light.format)));
            row("Lights", format("%zu point lights, %zu shadow casters", info.dynamic_lights, info.shadow_casters));
            row("Light data", format("%zu sections, %.2f MB, %zu emitters", info.light.sections, info.light.bytes / MB, info.light.emitters));
            row("Updates", format("%zu pending, %.2f ms", info.light.pending_columns, info.light.update_ms));
            const LightLevel& lh = info.light_here;
            row("Light here", format("sky %d, block %d %d %d", lh.sky, lh.red, lh.green, lh.blue));
        }
    }

    void render(fizmo::windows::Renderer& r, unsigned int w, unsigned int h, const HudSettings& hs) {
        if (hs.show_crosshair) draw_crosshair(r, w, h, hs);
        if (hs.show_debug && !m_rows.empty()) draw_panel(r, w, h, hs);
    }

    const std::vector<HudRow>& rows() const noexcept { return m_rows; }

private:
    struct Box { int x, y, w, h; };

    void header(const char* name) { m_rows.push_back({ name, std::string(), true }); }
    void row(const char* label, std::string value) { m_rows.push_back({ label, std::move(value), false }); }

    static double clean(double v) noexcept { return std::fabs(v) < 0.005 ? 0.0 : v; }

    static std::string format(const char* fmt, ...) {
        char buf[256];
        va_list args;
        va_start(args, fmt);
        std::vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);
        return buf;
    }

    static int px(double v) noexcept { return static_cast<int>(std::lround(v)); }

    static fizmo::text::TextStyle style(const HudSettings& hs, double size, const Color& c, bool bold = false) {
        fizmo::text::TextStyle s(size, c);
        const double offset = hs.shadow_offset * hs.scale;
        if (offset > 0.0) s.set_shadow(offset, offset, 0.0, hs.text_shadow);
        if (bold) s.set_bold();
        return s;
    }

    unsigned int width_of(fizmo::windows::Renderer& r, const std::string& text, const fizmo::text::TextStyle& st, double size, bool bold) {
        if (m_cached_size != size) { m_widths.clear(); m_cached_size = size; }
        if (m_widths.size() > MAX_CACHED_WIDTHS) m_widths.clear();
        const std::string key = (bold ? "b:" : "r:") + text;
        auto it = m_widths.find(key);
        if (it != m_widths.end()) return it->second;
        unsigned int width = r.measure_text(text, st).width;
        if (width == 0) width = static_cast<unsigned int>(static_cast<double>(text.size()) * size * FALLBACK_CHAR_WIDTH);
        m_widths.emplace(key, width);
        return width;
    }

    static Box place(HudCorner corner, int w, int h, unsigned int screen_w, unsigned int screen_h, int margin) noexcept {
        const bool right  = corner == HudCorner::TopRight || corner == HudCorner::BottomRight;
        const bool bottom = corner == HudCorner::BottomLeft || corner == HudCorner::BottomRight;
        return { right ? static_cast<int>(screen_w) - w - margin : margin, bottom ? static_cast<int>(screen_h) - h - margin : margin, w, h };
    }

    void draw_panel(fizmo::windows::Renderer& r, unsigned int w, unsigned int h, const HudSettings& hs) {
        const double full = panel_height(hs, hs.scale);
        double reserved = hs.margin * hs.scale * 2.0;
        const double room = static_cast<double>(h) - reserved;

        if (hs.fit_to_screen && full > room && room > 0.0) {
            HudSettings fitted = hs;
            fitted.scale = hs.scale * room / full;
            draw_panel_at(r, w, h, fitted);
            return;
        }

        draw_panel_at(r, w, h, hs);
    }

    double panel_height(const HudSettings& hs, double scale) const noexcept {
        const double line = hs.text_size * scale * hs.line_spacing;
        double total = hs.padding * scale * 2.0;
        for (std::size_t i = 0; i < m_rows.size(); ++i) total += line + (m_rows[i].header && i > 0 ? line * hs.section_spacing : 0.0);
        return total;
    }

    void draw_panel_at(fizmo::windows::Renderer& r, unsigned int w, unsigned int h, const HudSettings& hs) {
        const double size = hs.text_size * hs.scale;
        const fizmo::text::TextStyle label = style(hs, size, hs.label_color);
        const fizmo::text::TextStyle value = style(hs, size, hs.text_color);
        const fizmo::text::TextStyle head  = style(hs, size, hs.header_color, hs.bold_headers);
        const int pad = px(hs.padding * hs.scale), margin = px(hs.margin * hs.scale), gap = px(hs.column_gap * hs.scale);
        const int line_h = px(size * hs.line_spacing), section_gap = px(size * hs.line_spacing * hs.section_spacing);
        unsigned int label_w = 0, value_w = 0, header_w = 0;
        int content_h = 0;

        for (std::size_t i = 0; i < m_rows.size(); ++i) {
            const HudRow& row = m_rows[i];

            if (row.header) {
                header_w = vmax(header_w, width_of(r, row.label, head, size, hs.bold_headers));
                if (i > 0) content_h += section_gap;
            } else {
                label_w = vmax(label_w, width_of(r, row.label, label, size, false));
                value_w = vmax(value_w, width_of(r, row.value, value, size, false));
            }

            content_h += line_h;
        }

        const int inner_w = vmax(static_cast<int>(header_w), static_cast<int>(label_w) + gap + static_cast<int>(value_w));
        const Box box = place(hs.corner, inner_w + pad * 2, content_h + pad * 2, w, h, margin);
        r.draw_rect(box.x, box.y, static_cast<unsigned int>(box.w), static_cast<unsigned int>(box.h), fizmo::graphics::Paint::fill(hs.background));
        int y = box.y + pad;
        const int label_x = box.x + pad, value_x = label_x + static_cast<int>(label_w) + gap;

        for (std::size_t i = 0; i < m_rows.size(); ++i) {
            const HudRow& row = m_rows[i];

            if (row.header) {
                if (i > 0) y += section_gap;
                r.draw_text(label_x, y, row.label, head);
            } else {
                r.draw_text(label_x, y, row.label, label);
                r.draw_text(value_x, y, row.value, value);
            }

            y += line_h;
        }
    }

    static void draw_crosshair(fizmo::windows::Renderer& r, unsigned int w, unsigned int h, const HudSettings& hs) {
        const int cx = static_cast<int>(w / 2), cy = static_cast<int>(h / 2);
        const int s = px(hs.crosshair_size * hs.scale), g = px(hs.crosshair_gap * hs.scale);
        const auto p = fizmo::graphics::Paint::stroke(hs.crosshair_color, static_cast<unsigned int>(vmax(1L, std::lround(hs.crosshair_thickness * hs.scale))));

        if (g <= 0) {
            r.draw_line(cx - s, cy, cx + s, cy, p);
            r.draw_line(cx, cy - s, cx, cy + s, p);
            return;
        }

        r.draw_line(cx - s, cy, cx - g, cy, p);
        r.draw_line(cx + g, cy, cx + s, cy, p);
        r.draw_line(cx, cy - s, cx, cy - g, p);
        r.draw_line(cx, cy + g, cx, cy + s, p);
    }

    std::vector<HudRow>                           m_rows;
    std::unordered_map<std::string, unsigned int> m_widths;
    double                                        m_cached_size = 0.0;
    double                                        m_elapsed = 0.0;
    bool                                          m_primed  = false;
};

} // namespace voxelspire

#endif // VOXELSPIRE_RENDER_HUD_HPP