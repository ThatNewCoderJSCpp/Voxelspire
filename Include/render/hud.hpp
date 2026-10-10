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

    void blend_update(const CpuTimings& s) noexcept;
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

    static std::string temperature_text(double celsius, TemperatureUnit unit);

    static std::string ram_text(const ProcessMemory& m);

    static std::string vram_text(const fizmo::windows::GpuMemory& m);

    static std::string change_text(double celsius, TemperatureUnit unit);

    static HudCorner next_corner(HudCorner c) noexcept { return static_cast<HudCorner>((static_cast<int>(c) + 1) % CORNER_COUNT); }

    static double stepped_scale(double scale, int direction) noexcept;

    bool due(double dt, const HudSettings& hs) noexcept;

    void set_info(const HudInfo& info, const HudSettings& hs);

    void render(fizmo::windows::Renderer& r, unsigned int w, unsigned int h, const HudSettings& hs);

    const std::vector<HudRow>& rows() const noexcept { return m_rows; }

private:
    struct Box { int x, y, w, h; };

    void header(const char* name) { m_rows.push_back({ name, std::string(), true, {} }); }
    void row(const char* label, std::string value, std::vector<HudMeter> meters = {}) { m_rows.push_back({ label, std::move(value), false, std::move(meters) }); }

    void light_rows(const HudInfo& info, const HudSettings& hs);

    int meters_width(const HudRow& row, double size, const HudSettings& hs) const noexcept;

    void draw_meters(
        fizmo::windows::Renderer& r, 
        const HudRow& row, 
        int x, int y, double size, 
        const HudSettings& hs
    ) const;

    static double clean(double v) noexcept { return std::fabs(v) < 0.005 ? 0.0 : v; }

    static std::string format(const char* fmt, ...);

    static int px(double v) noexcept { return static_cast<int>(std::lround(v)); }

    static fizmo::text::TextStyle style(
        const HudSettings& hs, 
        double size, 
        const Color& c, 
        bool bold = false
    );

    unsigned int width_of(
        fizmo::windows::Renderer& r, 
        const std::string& text, 
        const fizmo::text::TextStyle& st, 
        double size, bool bold
    );

    static Box place(
        HudCorner corner, 
        int w, int h, 
        unsigned int screen_w, unsigned int screen_h, 
        int margin
    ) noexcept;

    void draw_panel(
        fizmo::windows::Renderer& r, 
        unsigned int w, unsigned int h, 
        const HudSettings& hs
    );

    double panel_height(const HudSettings& hs, double scale) const noexcept;

    void draw_panel_at(
        fizmo::windows::Renderer& r, 
        unsigned int w, unsigned int h, 
        const HudSettings& hs
    );

    static void draw_crosshair(fizmo::windows::Renderer& r, unsigned int w, unsigned int h, const HudSettings& hs);

    std::vector<HudRow>                           m_rows;
    std::unordered_map<std::string, unsigned int> m_widths;
    double                                        m_cached_size = 0.0;
    double                                        m_elapsed = 0.0;
    bool                                          m_primed  = false;
};

} // namespace voxelspire

#endif // VOXELSPIRE_RENDER_HUD_HPP