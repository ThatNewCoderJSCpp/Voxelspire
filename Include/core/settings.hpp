#ifndef VOXELSPIRE_CORE_SETTINGS_HPP
#define VOXELSPIRE_CORE_SETTINGS_HPP

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include "limits.hpp"
#include "types.hpp"
#include "../input/input_bindings.hpp"
#include "../lighting/dynamic_light.hpp"
#include "../physics/air_resistance.hpp"
#include "../physics/fluid_resistance.hpp"
#include "../physics/fluid_flow.hpp"
#include "../lighting/format.hpp"
#include "../entity/entity_body.hpp"

namespace voxelspire {

struct WorldDefaults {
    static constexpr double gravity           = 32.0;
    static constexpr double terminal_velocity = 78.4;
    static constexpr double void_depth        = 64.0;
    static constexpr int    min_z             = -64;
    static constexpr int    max_z             = 320;
    static constexpr double fluid_buoyancy    = 0.85;
    static constexpr double fluid_sink_speed  = 2.0;
    static constexpr double current_speed     = 2.5;
    static constexpr double current_push      = 12.0;
    static constexpr double wade_slowdown     = 0.5;
    static constexpr double fall_break_depth  = 3.0;
    static constexpr int    fluid_updates     = 4096;

    static std::shared_ptr<const AirResistance> air_resistance() {
        return std::make_shared<QuadraticDrag>(QuadraticDrag::coefficient_for(terminal_velocity, gravity));
    }

    static std::shared_ptr<const FluidResistance> fluid_resistance() {
        return std::make_shared<QuadraticFluidDrag>();
    }

    static std::shared_ptr<const FluidRules> fluid_rules() {
        return std::make_shared<MinecraftFluid>();
    }
};

struct WorldSettings {
    static constexpr std::uint64_t RANDOM_SEED  = 0;
    static constexpr const char*   DEFAULT_NAME = "New World";

    std::uint64_t seed       = RANDOM_SEED;
    std::string   name       = DEFAULT_NAME;
    int    min_z             = WorldDefaults::min_z;
    int    max_z             = WorldDefaults::max_z;
    double gravity           = WorldDefaults::gravity;
    double void_depth        = WorldDefaults::void_depth;
    int    horizontal_limit  = EngineLimits::WORLD_MAX_HORIZONTAL;
    LightFormat light_format = LightFormat::Colored;

    std::shared_ptr<const AirResistance>   air_resistance   = WorldDefaults::air_resistance();
    std::shared_ptr<const FluidResistance> fluid_resistance = WorldDefaults::fluid_resistance();
    std::shared_ptr<const FluidRules>      fluid_rules      = WorldDefaults::fluid_rules();
    double current_speed    = WorldDefaults::current_speed;
    double current_push     = WorldDefaults::current_push;
    double wade_slowdown    = WorldDefaults::wade_slowdown;
    double fall_break_depth = WorldDefaults::fall_break_depth;
    int    fluid_updates    = WorldDefaults::fluid_updates;
    double fluid_buoyancy   = WorldDefaults::fluid_buoyancy;
    double fluid_sink_speed = WorldDefaults::fluid_sink_speed;

    double fall_height() const noexcept { return static_cast<double>(max_z - min_z) + void_depth; }

    WorldSettings validated() const {
        WorldSettings w = *this;
        w.min_z = vclamp(w.min_z, EngineLimits::WORLD_MIN_Z, EngineLimits::WORLD_MAX_Z - 1);
        w.max_z = vclamp(w.max_z, w.min_z + 1, EngineLimits::WORLD_MAX_Z);
        w.horizontal_limit = vclamp(w.horizontal_limit, EngineLimits::CHUNK_SIZE, EngineLimits::WORLD_MAX_HORIZONTAL);
        if (!w.air_resistance) w.air_resistance = std::make_shared<NoAirResistance>();
        if (!w.fluid_resistance) w.fluid_resistance = std::make_shared<NoFluidResistance>();
        if (!w.fluid_rules) w.fluid_rules = std::make_shared<StillFluid>();
        return w;
    }
};

struct CameraSettings {
    double fov_y                 = 70.0;
    double near_plane            = 0.05;
    double third_person_distance = 4.0;
    double collision_margin      = 0.2;
};

struct DisplaySettings {
    bool vsync   = false;
    int  max_fps = 0;
};

struct ControlSettings {
    double mouse_sensitivity = 0.12;
    bool   invert_y          = false;
};

struct SimulationSettings {
    double tick_rate           = 60.0;
    double game_speed          = 1.0;
    int    max_ticks_per_frame = 10;
};

struct SaveSettings {
    double      autosave_minutes = 5.0;
    std::string folder           = "worlds";
};

struct MenuSettings {
    bool        pause_game    = true;
    bool        save_on_close = true;
    double      scale         = 1.0;
    std::string file          = "config/voxelspire_settings.cfg";
    Color       backdrop      { 0, 0, 0, 150 };
    Color       panel         { 20, 23, 31, 245 };
    Color       sidebar       { 14, 16, 22, 255 };
    Color       row_hover     { 255, 255, 255, 14 };
    Color       text          { 232, 236, 244 };
    Color       muted         { 140, 150, 168 };
    Color       accent        { 92, 164, 255 };
    Color       toggle_on     { 64, 186, 104 };
    Color       control       { 42, 47, 60 };
    Color       danger        { 214, 88, 88 };
    Color       error         { 255, 96, 96 };
};

struct StreamingSettings {
    int         simulation_distance    = 8;
    int         detail_distance        = 12;
    int         unload_margin          = 2;
    int         max_column_jobs        = 64;
    int         max_mesh_jobs          = 256;
    std::size_t upload_bytes_per_frame = 16u * 1024u * 1024u;
    double      result_time_budget_ms  = 4.0;
    int         worker_threads         = 0;
};

struct LodSettings {
    bool        enabled            = true;
    int         tile_chunks        = 2;
    int         max_level          = 12;
    int         heightmap_level    = 4;
    double      coverage_threshold = 0.5;
    std::size_t max_tile_jobs      = 32;
    int         exact_levels       = 2;
    int         samples_per_cell   = 4;
};

struct ParticleSettings {
    bool        enabled           = true;
    std::size_t max_particles     = 200000;
    double      emit_distance     = 160.0;
    double      draw_distance     = 160.0;
    double      recenter_distance = 512.0;
};

struct FaceShadingSettings {
    double up = 1.0, down = 0.5, north_south = 0.8, east_west = 0.62;
};

struct RenderSettings {
    int    render_distance      = 32;
    int    render_distance_step = 4;
    bool   merge_faces          = true;
    bool   cave_culling         = true;
    bool   face_culling         = true;
    bool   cull_void_faces      = true;
    double block_color_variation = 1.0;
    int    wave_detail          = 2;
    FaceShadingSettings shading;
    Color  sky_color        { 135, 190, 255 };
    Color  outline_color    { 0, 0, 0, 200 };
    unsigned int outline_width = 2;
    double outline_inflate  = 0.002;
    Color  player_color     { 45, 95, 225 };
    Color  player_visor     { 170, 215, 255 };
    int    capsule_segments = 16;
    int    capsule_rings    = 4;
    bool   first_person_body = true;

    double render_distance_blocks() const noexcept { return static_cast<double>(render_distance) * EngineLimits::CHUNK_SIZE; }
};

enum class HudCorner : std::uint8_t { TopLeft = 0, TopRight, BottomLeft, BottomRight };

struct HudSections {
    bool performance = true;
    bool gpu         = true;
    bool player      = true;
    bool world       = true;
    bool rendering   = true;
    bool lighting    = true;
};

struct HudSettings {
    bool        show_debug       = true;
    bool        show_crosshair   = true;
    bool        show_last_key    = true;
    HudCorner   corner           = HudCorner::TopLeft;
    HudCorner   hint_corner      = HudCorner::BottomLeft;
    double      scale            = 1.0;
    double      text_size        = 15.0;
    double      hint_text_size   = 13.0;
    double      line_spacing     = 1.3;
    double      section_spacing  = 0.5;
    double      column_gap       = 14.0;
    double      margin           = 12.0;
    double      padding          = 10.0;
    double      refresh_interval = 0.25;
    bool        bold_headers     = true;
    bool        fit_to_screen    = true;
    Color       text_color       { 238, 242, 250 };
    Color       label_color      { 150, 172, 205 };
    Color       header_color     { 255, 208, 110 };
    Color       background       { 8, 10, 16, 185 };
    Color       text_shadow      { 0, 0, 0, 210 };
    double      shadow_offset    = 1.0;
    Color       crosshair_color  { 255, 255, 255, 220 };
    double      crosshair_size   = 8.0;
    double      crosshair_gap    = 0.0;
    double      crosshair_thickness = 2.0;
    double      meter_width      = 5.0;
    Color       meter_background { 255, 255, 255, 38 };
    Color       sky_meter        { 125, 195, 255 };
    Color       level_meter      { 255, 212, 96 };
    Color       red_meter        { 235, 80, 80 };
    Color       green_meter      { 90, 210, 110 };
    Color       blue_meter       { 95, 145, 255 };
    HudSections sections;
};

struct EntitySettings {
    std::size_t octree_max_per_node = 8;
    std::size_t octree_max_depth    = 8;
    double      octree_looseness    = 2.0;
    double      octree_margin       = 1.0;
    double      push_acceleration   = 24.0;
    double      max_push_speed      = 4.0;
};

struct CharacterSettings {
    double width = PlayerDefaults::width;
    double reach = PlayerDefaults::reach;
    double mass  = PlayerDefaults::mass;

    double standing_height  = PlayerDefaults::height::standing;
    double crouching_height = PlayerDefaults::height::crouching;
    double crawling_height  = PlayerDefaults::height::crawling;
    double swimming_height  = PlayerDefaults::height::swimming;

    double standing_eye_height  = PlayerDefaults::eye_height::standing;
    double crouching_eye_height = PlayerDefaults::eye_height::crouching;
    double crawling_eye_height  = PlayerDefaults::eye_height::crawling;
    double swimming_eye_height  = PlayerDefaults::eye_height::swimming;

    double walk_speed   = PlayerDefaults::movement::movement_speed;
    double sprint_speed = PlayerDefaults::movement::sprint_speed;
    double crouch_speed = PlayerDefaults::movement::crouch_speed;
    double crawl_speed  = PlayerDefaults::movement::crawl_speed;
    double swim_speed   = PlayerDefaults::movement::swim_speed;
    double stroke_speed = PlayerDefaults::movement::stroke_speed;
    double fly_speed    = PlayerDefaults::movement::fly_speed;

    double alt_walk_speed   = PlayerDefaults::movement::alt_walk_speed;
    double alt_sprint_speed = PlayerDefaults::movement::alt_sprint_speed;
    double alt_crouch_speed = PlayerDefaults::movement::alt_crouch_speed;
    double alt_crawl_speed  = PlayerDefaults::movement::alt_crawl_speed;
    double alt_swim_speed   = PlayerDefaults::movement::alt_swim_speed;
    double alt_stroke_speed = PlayerDefaults::movement::alt_stroke_speed;
    double alt_fly_speed    = PlayerDefaults::movement::alt_fly_speed;

    double jump_velocity       = PlayerDefaults::movement::jump_velocity;
    double ground_acceleration = PlayerDefaults::movement::ground_acceleration;
    double air_acceleration    = PlayerDefaults::movement::air_acceleration;
    double swim_acceleration   = PlayerDefaults::movement::swim_acceleration;
    double stroke_acceleration = PlayerDefaults::movement::stroke_acceleration;

    double swim_rise_speed     = PlayerDefaults::swimming::rise_speed;
    double swim_sink_speed     = PlayerDefaults::swimming::sink_speed;
    double swim_vertical_accel = PlayerDefaults::swimming::vertical_accel;
    double surface_leap        = PlayerDefaults::swimming::surface_leap;
    double stroke_buoyancy     = PlayerDefaults::swimming::stroke_buoyancy;

    double fly_vertical_speed = PlayerDefaults::flying::vertical_speed;
    double fly_vertical_accel = PlayerDefaults::flying::vertical_accel;

    double speed_ratio(double speed) const noexcept { return walk_speed > 0.0 ? speed / walk_speed : 0.0; }
    double acceleration_ratio(double accel) const noexcept { return ground_acceleration > 0.0 ? accel / ground_acceleration : 0.0; }

    EntityBody body() const {
        EntityBody b({ width, standing_height, standing_eye_height });
        b.set(Poses::Crouching, { width, crouching_height, crouching_eye_height });
        b.set(Poses::Prone,     { width, crawling_height,  crawling_eye_height });
        b.set(Poses::Swimming,  { width, swimming_height,  swimming_eye_height });
        return b;
    }
};

struct LightingSettings {
    std::string name = "classic";
 
    bool   baked_light       = true;
    bool   smooth_lighting   = true;
    double ambient_occlusion = 1.0;
    double occlusion_step    = 0.2;
    double face_shading      = 1.0;
 
    double falloff   = 1.6;
    double min_light = 0.05;
    double max_light = 1.0;
    double ambient   = 0.0;
 
    double sky_light   = 1.0;
    double block_light = 1.0;
    Color  block_tint  = Color(255, 238, 214);
 
    bool   sun_lighting = false;
    double sun_strength = 0.55;
    double sun_exposure = 4.0;
 
    bool        dynamic_lights         = false;
    std::size_t max_dynamic_lights     = 32;
    double      dynamic_light_distance = 64.0;
 
    bool        block_point_lights          = false;
    std::size_t max_block_point_lights      = 24;
    double      block_point_light_distance  = 40.0;
    double      block_point_light_intensity = 0.8;
    double      block_point_light_radius    = 12.0;
    double      point_light_fade            = 6.0;
 
    bool         sun_shadows                  = false;
    unsigned int sun_shadow_resolution        = 2048;
    double       sun_shadow_distance          = 64.0;
    double       shadow_strength              = 0.85;
    double       shadow_softness              = 1.5;
    double       shadow_angle_step            = 0.5;
    bool         shadow_crossfade             = true;
    bool         soft_shadows                 = false;
    double       soft_shadow_sun_size         = 1.2;
    double       max_shadow_softness          = 12.0;
    unsigned int shadow_filter_taps           = 4;
    double       sun_shadow_redraw            = 0.125;
    bool         point_shadows                = false;
    unsigned int max_point_shadows            = 4;
    unsigned int point_shadow_resolution      = 512;
    double       point_shadow_fade            = 4.0;
    bool         hide_unshadowed_point_lights = true;
 
    bool   capsule_shadows          = true;
    double capsule_shadow_sun_size  = 1.0;
    double capsule_shadow_lamp_size = 0.2;
 
    bool   atmosphere      = false;
    double fog_density     = 0.0025;
    double fog_start       = 32.0;
    double sky_glow        = 0.35;
    double sun_glow_spread = 8.0;
    double sun_glow_focus  = 400.0;
 
    bool         volumetric_light      = false;
    unsigned int volumetric_steps      = 16;
    double       volumetric_density    = 0.01;
    double       volumetric_anisotropy = 0.5;
    double       volumetric_distance   = 96.0;
    double       volumetric_intensity  = 1.0;
    double       volumetric_near_bias  = 2.0;
    unsigned int volumetric_cell_size  = 8;
 
    bool         light_shafts          = false;
    unsigned int light_shaft_samples   = 48;
    double       light_shaft_strength  = 0.25;
    double       light_shaft_decay     = 0.96;
    double       light_shaft_length    = 0.85;
    double       light_shaft_focus     = 24.0;
 
    bool         reflections         = false;
    double       reflectivity        = 1.0;
    double       wave_strength       = 0.08;
    double       wave_scale          = 0.9;
    double       wave_speed          = 1.2;
    double       specular_power      = 180.0;
    double       specular            = 2.5;
    bool         refraction          = false;
    double       refraction_strength = 1.0;
    double       water_absorption    = 0.1;
    double       water_scattering    = 0.08;
    bool         screen_reflections  = false;
    unsigned int reflection_steps    = 32;
    double       reflection_distance = 64.0;
 
    bool         planar_reflections         = false;
    unsigned int max_reflection_planes      = 1;
    double       reflection_resolution      = 0.5;
    double       mirror_resolution          = 0.5;
    double       reflection_distortion      = 0.02;
    bool         reflect_any_distance       = true;
    double       reflection_plane_distance  = 48.0;
    int          reflection_view_chunks     = 8;
    bool         water_planar_reflections   = true;
 
    bool   underwater_fog     = true;
    double underwater_density = 0.12;
 
    bool   tone_mapping = false;
    double exposure     = 1.0;
    double saturation   = 1.0;
 
    double update_budget_ms = 3.0;
 
    static LightingSettings off() {
        LightingSettings s;
        s.name              = "off";
        s.baked_light       = false;
        s.smooth_lighting   = false;
        s.ambient_occlusion = 0.0;
        return s;
    }
 
    static LightingSettings basic() {
        LightingSettings s;
        s.name              = "basic";
        s.smooth_lighting   = false;
        s.ambient_occlusion = 0.0;
        return s;
    }
 
    static LightingSettings classic() { return {}; }
 
    static LightingSettings dynamic() {
        LightingSettings s;
        s.name           = "dynamic";
        s.dynamic_lights = true;
        return s;
    }
 
    static LightingSettings realistic() {
        LightingSettings s;
        s.name                 = "realistic";
        s.dynamic_lights       = true;
        s.block_point_lights   = true;
        s.max_light            = 2.2;
        s.sun_strength         = 1.0;
        s.sky_light            = 0.58;
        s.block_light          = 0.5;
        s.ambient_occlusion    = 1.1;
        s.face_shading         = 0.25;
        s.sun_lighting         = true;
        s.sun_shadows          = true;
        s.soft_shadows         = true;
        s.point_shadows        = true;
        s.atmosphere           = true;
        s.volumetric_light     = true;
        s.volumetric_steps     = 24;
        s.volumetric_density   = 0.04;
        s.volumetric_intensity = 1.0;
        s.light_shafts         = true;
        s.reflections          = true;
        s.planar_reflections   = true;
        s.refraction           = true;
        s.tone_mapping         = true;
        s.exposure             = 0.9;
        s.saturation           = 1.15;
        return s;
    }
 
    static LightingSettings ultra() {
        LightingSettings s = realistic();
        s.name                    = "ultra";
        s.max_light               = 2.5;
        s.sun_strength            = 1.1;
        s.sky_light               = 0.55;
        s.ambient_occlusion       = 1.2;
        s.face_shading            = 0.2;
        s.volumetric_steps        = 48;
        s.volumetric_cell_size    = 6;
        s.volumetric_density      = 0.06;
        s.volumetric_intensity    = 2.0;
        s.light_shaft_samples     = 64;
        s.light_shaft_strength    = 0.3;
        s.screen_reflections      = true;
        s.max_reflection_planes   = 2;
        s.reflection_resolution   = 1.0;
        s.mirror_resolution       = 1.0;
        s.saturation              = 1.2;
        s.sun_shadow_resolution   = 4096;
        s.sun_shadow_distance     = 96.0;
        s.shadow_strength         = 0.92;
        s.shadow_filter_taps      = 6;
        s.soft_shadow_sun_size    = 1.5;
        s.max_point_shadows       = 8;
        s.point_shadow_resolution = 768;
        s.max_block_point_lights  = 32;
        return s;
    }
};

struct DayCycleSettings {
    static constexpr double REAL_SECONDS_PER_MINUTE = 60.0;
    static constexpr double HOURS_PER_DAY           = 24.0;
    static constexpr double MINUTES_PER_HOUR        = 60.0;
    static constexpr double SECONDS_PER_MINUTE      = 60.0;
 
    bool   enabled          = true;
    double real_day_minutes = 20.0;
    double start_time       = 0.3;
    double sun_tilt         = 0.35;
    double night_brightness = 0.2;
    double horizon_fade     = 0.12;
    double twilight         = 0.2;
    double dusk_sky_mix     = 0.6;
    Color  day_sky          = Color(135, 190, 255);
    Color  night_sky        = Color(8, 11, 28);
    Color  dusk_sky         = Color(250, 140, 80);
    Color  day_light_tint   = Color(255, 255, 255);
    Color  night_light_tint = Color(130, 150, 255);
    Color  day_zenith       = Color(62, 118, 228);
    Color  day_horizon      = Color(172, 208, 250);
    Color  night_zenith     = Color(3, 5, 16);
    Color  night_horizon    = Color(14, 20, 44);
    Color  dusk_horizon     = Color(255, 150, 90);
    Color  sun_glow         = Color(255, 214, 160);
    Color  dusk_glow        = Color(255, 120, 60);
 
    double real_day_seconds()  const noexcept { return real_day_minutes * REAL_SECONDS_PER_MINUTE; }
    double real_hour_seconds() const noexcept { return real_day_seconds() / HOURS_PER_DAY; }
};

enum class AirModel : std::uint8_t { Keep = 0, None, TerminalCap, WorldHeight, Linear, Quadratic };

struct PhysicsSettings {
    static constexpr double DEFAULT_TERMINAL_VELOCITY = 78.4;

    AirModel air_model         = AirModel::Keep;
    double   terminal_velocity = DEFAULT_TERMINAL_VELOCITY;

    std::shared_ptr<const AirResistance> air_resistance(double gravity, const std::shared_ptr<const AirResistance>& current) const {
        switch (air_model) {
            case AirModel::Keep:        return current;
            case AirModel::None:        return std::make_shared<NoAirResistance>();
            case AirModel::TerminalCap: return std::make_shared<TerminalVelocityCap>(terminal_velocity);
            case AirModel::WorldHeight: return std::make_shared<WorldHeightLimit>();
            case AirModel::Linear:      return std::make_shared<LinearDrag>(LinearDrag::coefficient_for(terminal_velocity, gravity));
            case AirModel::Quadratic:   return std::make_shared<QuadraticDrag>(QuadraticDrag::coefficient_for(terminal_velocity, gravity));
        }
        return current;
    }
};

enum class FluidModel : std::uint8_t { None = 0, Linear, TickDamping, Quadratic };
enum class FlowModel  : std::uint8_t { Still = 0, Minecraft, Realistic };
enum class DisplaceBy : std::uint8_t { Weight = 0, BodySize };

struct WaveSettings {
    bool   enabled      = false;
    double calm_height  = 0.03;
    double storm_height = 0.35;
    double wavelength   = 18.0;
    double speed        = 1.0;
    bool   overtop      = true;
    double overtop_rate = 1.0;
};

struct WaterSettings {
    static constexpr const char* STILL     = "still";
    static constexpr const char* MINECRAFT = "minecraft";
    static constexpr const char* FLOWING   = "flowing";
    static constexpr const char* REALISTIC = "realistic";
    static constexpr const char* ULTRA     = "ultra";
 
    static constexpr double DEFAULT_LINEAR_DRAG    = 4.0;
    static constexpr int    DEFAULT_UPDATES        = 4096;
    static constexpr int    ULTRA_UPDATES          = 8192;
    static constexpr double DEFAULT_BUOYANCY       = 0.85;
    static constexpr double DEFAULT_SINK_SPEED     = 2.0;
    static constexpr double DEFAULT_CURRENT_SPEED  = 2.5;
    static constexpr double DEFAULT_CURRENT_PUSH   = 12.0;
    static constexpr double DEFAULT_WADE_SLOWDOWN  = 0.5;
    static constexpr double DEFAULT_FALL_BREAK     = 3.0;
    static constexpr double STILL_WADE_SLOWDOWN    = 0.3;
    static constexpr double MINECRAFT_FALL_BREAK   = 1.0;
    static constexpr double REALISTIC_WADE         = 0.6;
    static constexpr double ULTRA_WADE             = 0.7;
    static constexpr double ULTRA_INTERVAL         = 0.05;
    static constexpr double ULTRA_MIN_DEPTH        = 1.0 / 64.0;
 
    std::string name = REALISTIC;
 
    FlowModel    flow               = FlowModel::Realistic;
    double       minecraft_interval = MinecraftFluid::DEFAULT_INTERVAL;
    int          flow_spread        = MinecraftFluid::DEFAULT_SPREAD;
    int          slope_search       = MinecraftFluid::DEFAULT_SLOPE_SEARCH;
    bool         infinite_sources   = true;
    double       realistic_interval = RealisticFluid::DEFAULT_INTERVAL;
    double       min_depth          = RealisticFluid::DEFAULT_MIN_DEPTH;
    bool         seek_drops         = true;
    int          drop_search        = RealisticFluid::DEFAULT_DROP_SEARCH;
    bool         displacement       = false;
    DisplaceBy   displace_by        = DisplaceBy::Weight;
    bool         splashes           = true;
    double       splash             = 1.0;
    int          updates            = DEFAULT_UPDATES;
 
    WaveSettings waves;
 
    bool         rain_fills         = true;
    double       rain_fill          = 0.12;
    double       soak               = 0.25;
    double       evaporation        = 0.05;
    double       rain_reach         = 64.0;
 
    FluidModel   resistance         = FluidModel::Quadratic;
    double       fluid_density      = QuadraticFluidDrag::WATER_DENSITY;
    double       drag_coefficient   = QuadraticFluidDrag::DRAG_COEFFICIENT;
    double       linear_drag        = DEFAULT_LINEAR_DRAG;
    double       tick_damping       = TickFluidDamping::MINECRAFT_FACTOR;
    double       damping_ticks      = TickFluidDamping::MINECRAFT_TICKS;
 
    double       buoyancy           = DEFAULT_BUOYANCY;
    double       sink_speed         = DEFAULT_SINK_SPEED;
    double       current_speed      = DEFAULT_CURRENT_SPEED;
    double       current_push       = DEFAULT_CURRENT_PUSH;
    double       wade_slowdown      = DEFAULT_WADE_SLOWDOWN;
    double       fall_break_depth   = DEFAULT_FALL_BREAK;
 
    static WaterSettings still() {
        WaterSettings w;
        w.name          = STILL;
        w.flow          = FlowModel::Still;
        w.waves.enabled = false;
        w.rain_fills    = false;
        w.resistance    = FluidModel::Linear;
        w.current_speed = 0.0;
        w.current_push  = 0.0;
        w.wade_slowdown = STILL_WADE_SLOWDOWN;
        return w;
    }
 
    static WaterSettings minecraft() {
        WaterSettings w;
        w.name             = MINECRAFT;
        w.flow             = FlowModel::Minecraft;
        w.waves.enabled    = false;
        w.rain_fills       = false;
        w.resistance       = FluidModel::TickDamping;
        w.fall_break_depth = MINECRAFT_FALL_BREAK;
        return w;
    }
 
    static WaterSettings flowing() {
        WaterSettings w;
        w.name       = FLOWING;
        w.flow       = FlowModel::Minecraft;
        w.rain_fills = false;
        return w;
    }
 
    static WaterSettings realistic() {
        WaterSettings w;
        w.wade_slowdown = REALISTIC_WADE;
        return w;
    }
 
    static WaterSettings ultra() {
        WaterSettings w;
        w.name               = ULTRA;
        w.realistic_interval = ULTRA_INTERVAL;
        w.min_depth          = ULTRA_MIN_DEPTH;
        w.drop_search        = RealisticFluid::MAX_DROP_SEARCH;
        w.displacement       = true;
        w.updates            = ULTRA_UPDATES;
        w.wade_slowdown      = ULTRA_WADE;
        return w;
    }
 
    bool same_flow(const WaterSettings& o) const noexcept {
        if (flow != o.flow) return false;
 
        switch (flow) {
            case FlowModel::Still:     return true;
            case FlowModel::Minecraft: return minecraft_interval == o.minecraft_interval && flow_spread == o.flow_spread && slope_search == o.slope_search && infinite_sources == o.infinite_sources;
            case FlowModel::Realistic: return realistic_interval == o.realistic_interval && min_depth == o.min_depth && seek_drops == o.seek_drops && drop_search == o.drop_search
                                           && displacement == o.displacement && splashes == o.splashes && splash == o.splash;
        }
 
        return false;
    }
 
    std::shared_ptr<const FluidRules> rules(int sea_level = RealisticFluid::NO_SEA) const {
        switch (flow) {
            case FlowModel::Still:     return std::make_shared<StillFluid>();
            case FlowModel::Minecraft: return std::make_shared<MinecraftFluid>(minecraft_interval, flow_spread, slope_search, infinite_sources);
            case FlowModel::Realistic: return std::make_shared<RealisticFluid>(realistic_interval, min_depth, seek_drops, drop_search, displacement, splashes ? splash : 0.0, sea_level);
        }
 
        return std::make_shared<StillFluid>();
    }
 
    std::shared_ptr<const FluidResistance> drag(double body_mass = QuadraticFluidDrag::BODY_MASS) const {
        switch (resistance) {
            case FluidModel::None:        return std::make_shared<NoFluidResistance>();
            case FluidModel::Linear:      return std::make_shared<LinearFluidDrag>(linear_drag);
            case FluidModel::TickDamping: return std::make_shared<TickFluidDamping>(tick_damping, damping_ticks);
            case FluidModel::Quadratic:   return std::make_shared<QuadraticFluidDrag>(fluid_density, drag_coefficient, body_mass);
        }
 
        return std::make_shared<NoFluidResistance>();
    }
};

struct GlowSettings {
    bool   enabled  = true;
    double size     = 5.0;
    double strength = 0.4;
    int    rings    = 12;
};

struct SunSettings {
    bool   visible          = true;
    double size             = 2.5;
    int    segments         = 48;
    int    rings            = 6;
    Color  color            = Color(255, 238, 190);
    Color  dusk_color       = Color(255, 150, 70);
    double brightness       = 1.0;
    double limb_darkening   = 0.3;
    double edge_softness    = 0.08;
    GlowSettings glow;

    bool   emits_light      = true;
    double light_strength   = 1.0;
    Color  light_color      = Color(255, 244, 222);
    Color  dusk_light_color = Color(255, 160, 90);
};

struct MoonSettings {
    bool   visible        = true;
    double size           = 2.8;
    int    segments       = 40;
    int    rings          = 16;
    Color  color          = Color(228, 232, 242);
    double brightness     = 0.95;
    double edge_softness  = 0.05;
    double day_visibility = 0.45;

    bool   phases         = true;
    double cycle_days     = 8.0;
    double phase_offset   = 0.0;
    double earthshine     = 0.06;
    double dark_opacity   = 0.3;
    double terminator     = 0.08;

    GlowSettings glow { true, 3.5, 0.12, 10 };

    bool   emits_light    = true;
    double light_strength = 0.3;
    Color  light_color    = Color(170, 190, 255);
};

struct StarSettings {
    bool          visible              = true;
    int           count                = 1600;
    double        size                 = 0.3;
    double        size_variation       = 0.6;
    double        brightness           = 1.0;
    double        brightness_variation = 0.75;
    double        color_variation      = 0.35;
    Color         color                = Color(255, 255, 255);
    Color         warm_color           = Color(255, 196, 140);
    Color         cool_color           = Color(160, 190, 255);
    double        twinkle              = 0.35;
    double        twinkle_speed        = 1.5;
    double        day_visibility       = 0.0;
    bool          rotate               = true;
    std::uint32_t seed                 = 1337;
};

struct CelestialSettings {
    double       distance        = 0.85;
    double       horizon_fade    = 0.03;
    bool         hide_underwater = true;
    SunSettings  sun;
    MoonSettings moon;
    StarSettings stars;
};

enum class SeasonMode      : std::uint8_t { Cycle = 0, Fixed, Off };
enum class Season          : std::uint8_t { Spring = 0, Summer, Autumn, Winter };
enum class WeatherMode     : std::uint8_t { Changing = 0, AlwaysClear, AlwaysRain, AlwaysStorm };
enum class RainStyle       : std::uint8_t { Realistic = 0, Simple };
enum class TemperatureUnit : std::uint8_t { Celsius = 0, Fahrenheit };

struct Seasons {
    static constexpr int COUNT = 4;

    static constexpr std::array<const char*, COUNT> NAMES{ "Spring", "Summer", "Autumn", "Winter" };

    static const char* name(Season s) noexcept { return NAMES[static_cast<std::size_t>(s)]; }
};

struct SeasonSettings {
    SeasonMode mode              = SeasonMode::Cycle;
    Season     fixed_season      = Season::Summer;
    Season     start_season      = Season::Spring;
    int        days_per_month    = 8;
    int        months_per_season = 3;
    double     sun_swing         = 23.4;
    double     temperature       = 1.0;

    int    days_per_season() const noexcept { return days_per_month * months_per_season; }
    int    days_per_year()   const noexcept { return days_per_season() * Seasons::COUNT; }
    int    months_per_year() const noexcept { return months_per_season * Seasons::COUNT; }
};

struct TemperatureSettings {
    bool   daily_change    = true;
    double daily_strength  = 1.0;
    double altitude_drop   = 1.5;
    double offset          = 0.0;
    double weather_cooling = 4.0;
    double cloud_damping   = 0.6;
};

struct WeatherSettings {
    static constexpr std::size_t SEASON_COUNT = Seasons::COUNT;

    WeatherMode mode            = WeatherMode::Changing;
    RainStyle   style           = RainStyle::Realistic;
    double      clear_days      = 1.2;
    double      rain_days       = 0.35;
    double      storm_chance    = 0.25;
    double      hail_chance     = 0.35;
    double      change_hours    = 1.0;
    double      snow_below      = -1.0;
    double      sleet_below     = 2.5;
    double      hail_above      = 12.0;
    double      dry_below       = 0.15;
    double      darkening       = 0.55;
    double      storm_darkening = 0.8;
    double      fog             = 3.0;
    bool        lightning       = true;
    double      lightning_rate  = 6.0;
    double      wind            = 1.0;
    double      showers         = 0.35;
    double      shower_hours    = 3.0;
    std::array<double, SEASON_COUNT> season_rain{ 1.3, 0.9, 1.1, 1.0 };
    TemperatureSettings temperature;
};

struct WeatherViewSettings {
    bool            precipitation = true;
    double          amount        = 1.0;
    int             max_drops     = 6000;
    double          radius        = 22.0;
    bool            flashes       = true;
    TemperatureUnit unit          = TemperatureUnit::Celsius;
};

struct BiomeOptions {
    static constexpr double DEFAULT_WEIGHT = 1.0;
    static constexpr double DEFAULT_SIZE   = 1.0;

    bool                  enabled     = true;
    double                weight      = DEFAULT_WEIGHT;
    double                size        = DEFAULT_SIZE;
    double                temperature = 0.0;
    double                humidity    = 0.0;
    std::string           top;
    std::string           filler;
    std::string           underwater;
    std::string           cliff;
    std::string           deep;
    std::optional<int>    filler_depth;
    std::optional<bool>   freezes;
    std::optional<double> mean_temperature;
    std::optional<double> daily_swing;
    std::optional<double> season_swing;
    std::optional<double> rainfall;
    std::optional<double> waves;

    bool is_default() const noexcept {
        return enabled && weight == DEFAULT_WEIGHT && size == DEFAULT_SIZE && temperature == 0.0 && humidity == 0.0
            && top.empty() && filler.empty() && underwater.empty() && cliff.empty() && deep.empty() && !filler_depth && !freezes
            && !mean_temperature && !daily_swing && !season_swing && !rainfall && !waves;
    }
};

struct CaveSettings {
    bool   enabled        = true;
    double tunnels        = 1.0;
    double tunnel_width   = 1.0;
    double tunnel_length  = 1.0;
    double flatness       = 1.0;
    double caverns        = 1.0;
    double cavern_size    = 1.0;
    int    cavern_roof    = 6;
    int    min_depth      = 0;
    int    max_depth      = 0;
    double deep_growth    = 0.5;
    double entrances      = 1.0;
    int    entrance_depth = 32;
    double entrance_width = 1.0;
    double canyons        = 0.6;
    int    canyon_depth   = 24;
    double canyon_width   = 1.0;
    bool   underwater     = true;
};

struct TerrainSettings {
    int          sea_level       = 64;
    int          bedrock_layers  = 3;
    double       biome_size      = 1.0;
    double       continent_size  = 0.7;
    double       size_variation  = 0.25;
    double       climate_shift   = 0.15;
    double       ocean_amount    = 0.8;
    double       land_height     = 30.0;
    double       ocean_depth     = 26.0;
    double       mountain_amount = 0.6;
    double       mountain_height = 100.0;
    double       flatness        = 1.2;
    double       hill_height     = 10.0;
    double       roughness       = 1.0;
    bool         rivers          = true;
    double       river_width     = 1.0;
    double       river_depth     = 5.0;
    double       snow_line       = 1.25;
    double       shelf_width     = 1.5;
    CaveSettings caves;
 
    std::map<std::string, BiomeOptions> biomes;
 
    const BiomeOptions& biome(const std::string& id) const {
        static const BiomeOptions fallback;
        auto it = biomes.find(id);
        return it == biomes.end() ? fallback : it->second;
    }
 
    BiomeOptions& edit_biome(const std::string& id) { return biomes[id]; }
 
    void tidy_biome(const std::string& id) {
        auto it = biomes.find(id);
        if (it != biomes.end() && it->second.is_default()) biomes.erase(it);
    }
};

struct GameSettings {
    WorldSettings       world;
    TerrainSettings     terrain;
    PhysicsSettings     physics;
    WaterSettings       water;
    CharacterSettings   character;
    InputBindings       bindings = InputBindings::defaults();
    MenuSettings        menu;
    SaveSettings        saves;
    StreamingSettings   streaming;
    LodSettings         lod;
    LightingSettings    lighting;
    DayCycleSettings    day_cycle;
    CelestialSettings   celestial;
    SeasonSettings      seasons;
    WeatherSettings     weather;
    WeatherViewSettings weather_view;
    DynamicLight        hand_light = DynamicLight::glow(Color(255, 190, 120), 11.0, true);
    ParticleSettings    particles;
    CameraSettings      camera;
    DisplaySettings     display;
    ControlSettings     controls;
    SimulationSettings  simulation;
    EntitySettings      entities;
    RenderSettings      render;
    HudSettings         hud;
};

} // namespace voxelspire

#endif // VOXELSPIRE_CORE_SETTINGS_HPP