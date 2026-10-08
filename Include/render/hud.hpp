#ifndef VOXELSPIRE_RENDER_HUD_HPP
#define VOXELSPIRE_RENDER_HUD_HPP

#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include "../core/process_memory.hpp"
#include "../core/settings.hpp"
#include "../physics/heat.hpp"
#include "../world/chunk_streamer.hpp"
#include "../world/voxel_raycast.hpp"
#include "world_renderer.hpp"
#include "../world/fluid_simulator.hpp"

namespace voxelspire {

struct CpuTimings {
    static constexpr double SMOOTHING = 0.1;

    double update    = 0.0;
    double ticks     = 0.0;
    double fluids    = 0.0;
    double streaming = 0.0;
    double lighting  = 0.0;
    double render    = 0.0;

    static void blend(double& value, double sample) noexcept { value += (sample - value) * SMOOTHING; }

    void blend_update(const CpuTimings& s) noexcept {
        blend(update, s.update);
        blend(ticks, s.ticks);
        blend(fluids, s.fluids);
        blend(streaming, s.streaming);
        blend(lighting, s.lighting);
    }
};

struct HudInfo {
    double   fps_average = 0.0;
    vector3d position{};
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
    std::string   biome;
    LightStats    light;
    std::string   lighting_preset;
    double        time_hours         = 0.0;
    std::int64_t  day                = 0;
    bool          day_cycle          = true;
    LightLevel    light_here;
    std::size_t   dynamic_lights     = 0;
    std::size_t   shadow_casters     = 0;
    std::size_t   shadow_buried      = 0;
    std::string   water_preset;
    std::string   fluid_model;
    std::string   flow_model;
    FluidStats    fluids;
    std::size_t   reflection_planes  = 0;
    int           render_distance    = 0;
    double        daylight           = 1.0;
    std::string   water_weather;
    std::string   date;
    std::string   weather;
    double        temperature        = 0.0;
    TemperatureUnit temperature_unit = TemperatureUnit::Celsius;
    bool          heat_on            = false;
    double        body_temperature   = 0.0;
    double        felt_temperature   = 0.0;
    double        heat_warmth        = 0.0;
    double        heat_surround      = 0.0;
    double        heat_exchange      = 1.0;
    bool          heat_player        = true;
    HeatGear      gear;
    std::string   body_state;
        
    fizmo::windows::GpuTimings gpu;
    CpuTimings                 cpu;
    fizmo::windows::GpuMemory  vram;
    ProcessMemory              ram;
};

struct HudMeter {
    double fraction = 0.0;
    Color  color;
    double width = 1.0;
    double ghost = 0.0;
};

struct HudRow {
    std::string           label;
    std::string           value;
    bool                  header = false;
    std::vector<HudMeter> meters;
};

class Hud {
public:
    static constexpr double      FALLBACK_CHAR_WIDTH = 0.55;
    static constexpr double      SCALE_STEP          = 0.1;
    static constexpr double      METER_HEIGHT        = 0.55;
    static constexpr double      METER_GAP           = 0.35;
    static constexpr double      SWATCH_WIDTH        = 0.2;
    static constexpr double      CHANNEL_WIDTH       = 0.34;
    static constexpr double      FULL_DAYLIGHT       = 0.995;
    static constexpr double      GHOST_ALPHA         = 0.35;
    static constexpr std::size_t MAX_CACHED_WIDTHS   = 4096;
    static constexpr int         CORNER_COUNT        = 4;
    static constexpr double      PERCENT             = 100.0;
    static constexpr double      FAHRENHEIT_SCALE    = 1.8;
    static constexpr double      FAHRENHEIT_OFFSET   = 32.0;
    static constexpr double      GIGABYTE            = 1024.0 * 1024.0 * 1024.0;

    static std::string temperature_text(double celsius, TemperatureUnit unit) {
        const double fahrenheit = celsius * FAHRENHEIT_SCALE + FAHRENHEIT_OFFSET;
        if (unit == TemperatureUnit::Fahrenheit) return format("%.1f F (%.1f C)", fahrenheit, celsius);
        return format("%.1f C (%.1f F)", celsius, fahrenheit);
    }

    static std::string ram_text(const ProcessMemory& m) {
        if (!m.valid) return "not available";
        std::string out = format("%.2f GB used by the game (peak %.2f GB)", m.used / GIGABYTE, m.peak / GIGABYTE);
        if (m.system_total > 0) out += format(", %.1f of %.1f GB free on the computer", m.system_available / GIGABYTE, m.system_total / GIGABYTE);
        return out;
    }

    static std::string vram_text(const fizmo::windows::GpuMemory& m) {
        if (!m.valid) return "not available (software renderer)";
        if (!m.measured) return format("%.2f GB on the card, usage not reported by the driver", m.total / GIGABYTE);
        std::string out = format("%.2f GB used of %.2f GB the game may use (%.2f GB card)", m.used / GIGABYTE, m.budget / GIGABYTE, m.total / GIGABYTE);
        if (m.shared_used > 0) out += format(", %.2f GB of shared system memory", m.shared_used / GIGABYTE);
        return out;
    }

    static std::string change_text(double celsius, TemperatureUnit unit) {
        if (unit == TemperatureUnit::Fahrenheit) return format("%.1f F", celsius * FAHRENHEIT_SCALE);
        return format("%.1f C", celsius);
    }

    static HudCorner next_corner(HudCorner c) noexcept { return static_cast<HudCorner>((static_cast<int>(c) + 1) % CORNER_COUNT); }

    static double stepped_scale(double scale, int direction) noexcept {
        return HudLimits::scale.clamp(std::round((scale + SCALE_STEP * direction) / SCALE_STEP) * SCALE_STEP);
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
            row("CPU", format("update %.2f ms (ticks %.2f, fluids %.2f, streaming %.2f, light %.2f), render %.2f ms", info.cpu.update, info.cpu.ticks, info.cpu.fluids, info.cpu.streaming, info.cpu.lighting, info.cpu.render));
            row("Draws", format("%zu meshes in %zu batches: %zu on screen, %zu shadow, %zu reflection only", st.batch_items, st.batches, st.batch_items - st.shadow_casters - st.chunks_reflected, st.shadow_casters, st.chunks_reflected));
            row("Jobs", format("%zu gen, %zu mesh, %zu LOD, %zu uploads (%d threads)", info.streaming.jobs_in_flight, t.mesh_jobs, t.lod_jobs, t.pending_uploads, info.worker_threads));
            row("Remeshes", format("%zu total, %zu kept their solid mesh", t.uploads, t.reused));
            row("Mesh memory", format("%.1f MB GPU, %.1f MB RAM", t.gpu_mesh_bytes / MB, t.cpu_mesh_bytes / MB));
            row("RAM", ram_text(info.ram));
            row("VRAM", vram_text(info.vram));
        }

        if (hs.sections.gpu) {
            header("GPU");

            if (!info.gpu.valid) {
                row("Timing", "not available");
            } else {
                row("Frame", format("%.2f ms", info.gpu.total_ms));
                const double total = info.gpu.total_ms > 0.0 ? info.gpu.total_ms : 1.0;

                for (std::size_t i = 0; i < fizmo::windows::GpuTimings::PASSES; ++i) {
                    const double ms = info.gpu.pass_ms[i];
                    row(fizmo::windows::gpu_pass_name(static_cast<fizmo::windows::GpuPass>(i)), format("%.2f ms (%.0f%%)", ms, PERCENT * ms / total));
                }
            }
        }

        if (hs.sections.player) {
            header("Player");
            row("Position", format("%.2f  %.2f  %.2f", clean(info.position.x), clean(info.position.y), clean(info.position.z)));
            row("Velocity", format("%.2f  %.2f  %.2f", clean(info.velocity.x), clean(info.velocity.y), clean(info.velocity.z)));
            const double hspeed = std::sqrt(info.velocity.x * info.velocity.x + info.velocity.y * info.velocity.y);
            row("Speed", format("%.2f b/s %s", hspeed, info.on_ground ? "(ground)" : "(air)"));
            row("Movement", info.movement_mode);
            row("Camera", info.camera_mode);

            if (info.heat_on) {
                if (info.heat_player) row("Body", temperature_text(info.body_temperature, info.temperature_unit) + ", " + info.body_state);
                else row("Body", "not affected by heat");
                row("Feels like", temperature_text(info.felt_temperature, info.temperature_unit) + (info.gear.any() ? ", worn: stands " + change_text(info.gear.cold_degrees, info.temperature_unit) + " more cold, " + change_text(info.gear.hot_degrees, info.temperature_unit) + " hotter" : std::string()));
                row("Around you", temperature_text(info.heat_surround, info.temperature_unit) + ", +" + change_text(info.heat_warmth, info.temperature_unit)
                    + format(" from heat sources, losing heat %.1fx as fast as in still air", info.heat_exchange));
            }

            if (info.target) row("Target", format("%s at %d %d %d, %s face", info.target_name.c_str(), info.target->block.x, info.target->block.y, info.target->block.z, face_name(info.target->face)));
            else row("Target", "none");
        }

        if (hs.sections.world) {
            header("World");
            if (!info.biome.empty()) row("Biome", info.biome);
            const double minutes = (info.time_hours - std::floor(info.time_hours)) * DayCycleSettings::MINUTES_PER_HOUR;
            const int hour = static_cast<int>(info.time_hours), minute = static_cast<int>(minutes);
            const int second = static_cast<int>((minutes - minute) * DayCycleSettings::SECONDS_PER_MINUTE);
            row("Time", format("%02d:%02d:%02d, day %lld%s", hour, minute, second, static_cast<long long>(info.day), info.day_cycle ? "" : " (paused)"));
            if (!info.date.empty()) row("Date", info.date);
            if (!info.weather.empty()) row("Weather", info.weather);
            row("Temperature", temperature_text(info.temperature, info.temperature_unit));
            row("Columns", format("%zu loaded, %zu stored edits", info.streaming.loaded_columns, info.streaming.stored_chunks));
            row("Entities", format("%zu (%zu touching)", info.entity_count, info.entity_contacts));
            row("Particles", format("%zu (%zu emitters)", info.particles, info.particle_emitters));
            if (std::isfinite(info.terminal_velocity)) row("Air", format("%s, terminal %.1f b/s", info.air_model.c_str(), info.terminal_velocity));
            else row("Air", info.air_model + ", no terminal velocity");
            row("Fluids", format("%s preset, %s", info.water_preset.c_str(), info.fluid_model.c_str()));
            row("Water flow", format("%s, %zu waiting, %zu updated, %zu far away", info.flow_model.c_str(), info.fluids.pending, info.fluids.updated, info.fluids.dormant));
            if (!info.water_weather.empty()) row("Waves and rain", info.water_weather);
        }

        if (hs.sections.rendering) {
            header("Rendering");
            row("Distance", format("%d chunks, detail %d, simulation %d", info.render_distance, info.detail_distance, info.simulation_distance));
            row("Chunks", format("%zu drawn / %zu meshed, %zu cave-culled, %zu see-through", st.chunks_visible, st.chunks_total, st.chunks_cave_culled, st.translucent_visible));
            row("Quads", format("%zu drawn / %zu, %zu facing away", st.quads_drawn, st.quads_total, st.quads_facing_away));
            row("LOD tiles", format("%zu drawn / %zu, %zu quads", st.lod_tiles_drawn, st.lod_tiles_total, st.lod_quads_drawn));
            row("Reflections", format("%zu planes, %zu extra chunks", info.reflection_planes, st.chunks_reflected));
        }

        if (hs.sections.lighting) {
            header("Lighting");
            row("Preset", format("%s, %s light", info.lighting_preset.c_str(), light_format_name(info.light.format)));
            row("Lights", format("%zu point lights, %zu shadow casters, %zu buried skipped", info.dynamic_lights, info.shadow_casters, info.shadow_buried));
            row("Light data", format("%zu sections, %.2f MB, %zu emitters", info.light.sections, info.light.bytes / MB, info.light.emitters));
            row("Updates", format("%zu pending, %.2f ms", info.light.pending_columns, info.light.update_ms));
            light_rows(info, hs);
        }
    }

    void render(fizmo::windows::Renderer& r, unsigned int w, unsigned int h, const HudSettings& hs) {
        if (hs.show_crosshair) draw_crosshair(r, w, h, hs);
        if (hs.show_debug && !m_rows.empty()) draw_panel(r, w, h, hs);
    }

    const std::vector<HudRow>& rows() const noexcept { return m_rows; }

private:
    struct Box { int x, y, w, h; };

    void header(const char* name) { m_rows.push_back({ name, std::string(), true, {} }); }
    void row(const char* label, std::string value, std::vector<HudMeter> meters = {}) { m_rows.push_back({ label, std::move(value), false, std::move(meters) }); }

    void light_rows(const HudInfo& info, const HudSettings& hs) {
        const LightLevel& lh = info.light_here;
        const double max = LightLimits::MAX;
        const int sky_now = static_cast<int>(std::lround(lh.sky * info.daylight));
        const int block = lh.block();
        const int level = vmax(sky_now, block);
        const char* source = level == 0 ? "dark" : (sky_now >= block ? "from the sky" : "from blocks");
        auto byte = [](int v) { return static_cast<std::uint8_t>(v * LightLevel::BYTE_SCALE); };

        const std::string sky_text = info.daylight < FULL_DAYLIGHT ? format("%d / %d, %d right now", lh.sky, LightLimits::MAX, sky_now) : format("%d / %d", lh.sky, LightLimits::MAX);
        row("Light level", format("%d / %d, %s", level, LightLimits::MAX, source), { { level / max, hs.level_meter, 1.0 } });
        row("Sky light", sky_text, { { sky_now / max, hs.sky_meter, 1.0, lh.sky / max } });
        
        row("Block light", format("%d / %d  (R %d, G %d, B %d)", block, LightLimits::MAX, lh.red, lh.green, lh.blue),
            { { 1.0, Color(byte(lh.red), byte(lh.green), byte(lh.blue)), SWATCH_WIDTH },
              { lh.red / max, hs.red_meter, CHANNEL_WIDTH },
              { lh.green / max, hs.green_meter, CHANNEL_WIDTH },
              { lh.blue / max, hs.blue_meter, CHANNEL_WIDTH } });
    }

    int meters_width(const HudRow& row, double size, const HudSettings& hs) const noexcept {
        if (row.meters.empty()) return 0;
        const int gap = px(size * METER_GAP);
        int total = gap;
        for (const HudMeter& m : row.meters) total += px(size * hs.meter_width * m.width) + gap;
        return total;
    }

    void draw_meters(
        fizmo::windows::Renderer& r, 
        const HudRow& row, 
        int x, int y, double size, 
        const HudSettings& hs
    ) const {
        const int gap = px(size * METER_GAP), h = vmax(2, px(size * METER_HEIGHT));
        const int top = y + (px(size) - h) / 2;
        x += gap;

        for (const HudMeter& m : row.meters) {
            const int w = vmax(2, px(size * hs.meter_width * m.width));
            r.draw_rect(x, top, static_cast<unsigned int>(w), static_cast<unsigned int>(h), fizmo::graphics::Paint::fill(hs.meter_background));
            const int ghost = static_cast<int>(std::lround(w * vclamp(m.ghost, 0.0, 1.0)));
            const Color dim(m.color.red(), m.color.green(), m.color.blue(), static_cast<std::uint8_t>(m.color.alpha() * GHOST_ALPHA));
            if (ghost > 0) r.draw_rect(x, top, static_cast<unsigned int>(ghost), static_cast<unsigned int>(h), fizmo::graphics::Paint::fill(dim));
            const int filled = static_cast<int>(std::lround(w * vclamp(m.fraction, 0.0, 1.0)));
            if (filled > 0) r.draw_rect(x, top, static_cast<unsigned int>(filled), static_cast<unsigned int>(h), fizmo::graphics::Paint::fill(m.color));
            x += w + gap;
        }
    }

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

    static fizmo::text::TextStyle style(
        const HudSettings& hs, 
        double size, 
        const Color& c, 
        bool bold = false
    ) {
        fizmo::text::TextStyle s(size, c);
        const double offset = hs.shadow_offset * hs.scale;
        if (offset > 0.0) s.set_shadow(offset, offset, 0.0, hs.text_shadow);
        if (bold) s.set_bold();
        return s;
    }

    unsigned int width_of(
        fizmo::windows::Renderer& r, 
        const std::string& text, 
        const fizmo::text::TextStyle& st, 
        double size, bool bold
    ) {
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

    static Box place(
        HudCorner corner, 
        int w, int h, 
        unsigned int screen_w, unsigned int screen_h, 
        int margin
    ) noexcept {
        const bool right  = corner == HudCorner::TopRight || corner == HudCorner::BottomRight;
        const bool bottom = corner == HudCorner::BottomLeft || corner == HudCorner::BottomRight;
        return { right ? static_cast<int>(screen_w) - w - margin : margin, bottom ? static_cast<int>(screen_h) - h - margin : margin, w, h };
    }

    void draw_panel(
        fizmo::windows::Renderer& r, 
        unsigned int w, unsigned int h, 
        const HudSettings& hs
    ) {
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

    void draw_panel_at(
        fizmo::windows::Renderer& r, 
        unsigned int w, unsigned int h, 
        const HudSettings& hs
    ) {
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
                value_w = vmax(value_w, width_of(r, row.value, value, size, false) + static_cast<unsigned int>(meters_width(row, size, hs)));
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
                if (!row.meters.empty()) draw_meters(r, row, value_x + static_cast<int>(width_of(r, row.value, value, size, false)), y, size, hs);
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