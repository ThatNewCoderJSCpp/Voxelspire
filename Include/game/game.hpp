#ifndef VOXELSPIRE_GAME_HPP
#define VOXELSPIRE_GAME_HPP

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>
#include "../block/blocks.hpp"
#include "../camera/game_camera.hpp"
#include "../core/process_memory.hpp"
#include "../core/settings.hpp"
#include "../entity/entity_manager.hpp"
#include "../entity/player.hpp"
#include "../input/input_bindings.hpp"
#include "../lighting/director.hpp"
#include "../core/presets.hpp"
#include "../lighting/world_light.hpp"
#include "../physics/heat.hpp"
#include "../particles/system.hpp"
#include "../render/block_outline_renderer.hpp"
#include "../render/capsule_renderer.hpp"
#include "../render/hud.hpp"
#include "../render/reflection_planes.hpp"
#include "../render/hotbar.hpp"
#include "../render/vitals_hud.hpp"
#include "../render/world_renderer.hpp"
#include "../sky/renderer.hpp"
#include "../survival/player_survival.hpp"
#include "../ui/inventory.hpp"
#include "../ui/pause_menu.hpp"
#include "../ui/settings_file.hpp"
#include "../ui/settings_menu.hpp"
#include "../ui/settings_pages.hpp"
#include "../ui/settings_registry.hpp"
#include "../world/chunk_streamer.hpp"
#include "../world/terrain_generator.hpp"
#include "../world/fluid_simulator.hpp"
#include "../world/water_weather.hpp"
#include "../world/world_save.hpp"
#include "../weather/climate_field.hpp"
#include "../weather/precipitation.hpp"
#include "../weather/sky_weather.hpp"

namespace voxelspire {

struct WorldSlot {
    std::filesystem::path                   folder;
    std::string                             name;
    WorldState                              state;
    std::function<bool(const WorldState&)>  write_state;
    std::function<bool(const GameSettings&, const WorldState&)> write_options;
};

class Game {
public:
    explicit Game(const GameSettings& settings = GameSettings{}, const WorldGeneratorFactory& generator = {}, WorldSlot slot = {})
;

    ~Game() { m_jobs.shutdown(); }

    Game(const Game&) = delete;
    Game& operator=(const Game&) = delete;

    void start(fizmo::windows::Window& window, unsigned int viewport_w, unsigned int viewport_h);

    void shutdown() noexcept { if (m_cursor) m_cursor->unlock(); }

    bool save_world();

    void request_quit_to_title() noexcept { m_quit_requested = true; }
    bool take_quit_request() noexcept { const bool q = m_quit_requested; m_quit_requested = false; return q; }
    const std::filesystem::path& save_folder() const noexcept { return m_slot.folder; }

    bool mouse_locked() const noexcept { return m_cursor && m_cursor->locked(); }

    void handle_event(const fizmo::windows::WindowEvent& e);

    void update(double frame_dt, const fizmo::windows::InputManager& input);

    void render(fizmo::windows::Renderer& renderer, double fps_average);

    static fizmo::windows::UpscaleFilter upscale_filter(UpscaleMode mode) noexcept;

    void render_frame(fizmo::windows::Renderer& renderer, double fps_average);

    void track_gpu(fizmo::windows::Renderer& renderer);

    fizmo::windows::GpuTimings average_gpu() const noexcept;

    void open_inventory();

    void open_inventory(const std::string& tab) {
        m_inventory_screen.show_tab(tab);
        open_inventory();
    }

    void close_inventory();

    void toggle_inventory() {
        if (m_inventory_screen.is_open()) close_inventory();
        else open_inventory();
    }

    void open_status() { open_inventory(InventoryTabs::STATUS); }
    void close_status() { close_inventory(); }

    bool inventory_open() const noexcept { return m_inventory_screen.is_open(); }

    bool world_paused_for_inventory() const noexcept;

    bool world_hidden_for_inventory() const noexcept;

    InventoryView inventory_view();

    double carried_weight();

    Inventory&    inventory()    noexcept { return m_inventory; }
    StackRules&   stack_rules()  noexcept { return m_stack_rules; }
    ItemSystem&   items()        noexcept { return m_items; }
    CursorStack&  cursor_stack() noexcept { return m_cursor_stack; }
    InventoryScreen& inventory_screen() noexcept { return m_inventory_screen; }

    void open_menu();

    void close_menu() {
        if (!paused()) return;
        if (m_menu.is_open()) close_options();
        m_pause.close();
        lock_cursor();
    }

    void open_options() {
        if (!m_pause.is_open()) open_menu();
        m_menu.open();
    }

    void close_options();

    void save_and_exit();

    void lock_cursor() {
        if (!m_cursor) return;
        m_cursor->lock();
        m_ignore_look = LOOK_SETTLE_FRAMES;
    }

    void toggle_menu();

    bool          paused()     const noexcept { return m_pause.is_open() || m_menu.is_open(); }
    bool          menu_open()  const noexcept { return m_menu.is_open(); }
    SettingsMenu& menu()       noexcept { return m_menu; }
    PauseMenu&    pause_menu() noexcept { return m_pause; }

    bool save_settings();

    void load_settings();

    std::vector<SettingsTab> tabs_in(SettingsScope scope);

    bool write_world_file(const WorldState& state);

    void rebuild_menu();

    SettingsRegistry& settings_registry() noexcept { return m_settings_registry; }
    LightingPresets&  lighting_presets()  noexcept { return m_presets; }
    WaterPresets&     water_presets()     noexcept { return m_water_presets; }

    bool take_display_change() noexcept { const bool d = m_display_dirty; m_display_dirty = false; return d; }

    void apply_settings(std::uint32_t groups);

    HudInfo hud_info(double fps_average) const;

    LightLevel light_level_at(const vector3d& p) const { return m_lighting.light_level_at(m_world, p, m_frame_lights); }

    const LightingSettings& lighting() const noexcept { return m_settings.lighting; }

    void set_lighting(const LightingSettings& lighting);

    void cycle_lighting();

    WeatherSystem&      weather()       noexcept       { return m_weather; }
    const LocalWeather& local_weather() const noexcept { return m_local_weather; }
    const CalendarDate& date()          const noexcept { return m_date; }

    void set_sun_path(std::unique_ptr<SunPath> path) { if (path) m_sun_path = std::move(path); }
    CelestialRenderer& celestial()       noexcept { return m_celestial; }
    const SunPath&     sun_path()  const noexcept { return *m_sun_path; }
    WorldClock&        clock()           noexcept { return m_clock; }
    const SkyState&    sky()       const noexcept { return m_sky; }
    Color              sky_color() const noexcept { return m_lighting.enabled() ? m_sky.sky_color : m_settings.render.sky_color; }
    DynamicLights&     dynamic_lights()  noexcept { return m_dynamic_lights; }

public:
    GameSettings&         settings()        noexcept { return m_settings; }
    std::uint64_t         seed()      const noexcept { return m_settings.world.seed; }
    World&                world()           noexcept { return m_world; }
    Player&               player()          noexcept { return *m_player; }
    PlayerSurvival&       survival()        noexcept { return m_survival; }
    EntityManager&        entities()        noexcept { return m_entities; }
    GameCamera&           camera()          noexcept { return m_camera; }
    BlockRegistry&        blocks()          noexcept { return m_registry; }
    AttributeRegistry&    attribute_types() noexcept { return m_attributes; }
    MovementModeRegistry& movement_modes()  noexcept { return m_movement_modes; }
    ParticleTypeRegistry& particle_types()  noexcept { return m_particle_types; }
    ParticleSystem&       particles()       noexcept { return m_particles; }

    const DefaultBlocks& block_ids() const noexcept { return m_block_ids; }
    const WorldRenderStats& last_render_stats() const noexcept { return m_stats; }
    const std::optional<RaycastHit>& target() const noexcept { return m_target; }

    void set_render_distance(int chunks) noexcept;

    void set_simulation_distance(int chunks) noexcept;

    TerrainMeshes&       terrain()          noexcept { return m_terrain; }
    ChunkStreamer&       streamer()         noexcept { return m_streamer; }
    JobSystem&           jobs()             noexcept { return m_jobs; }
    const WorldGenerator& generator() const noexcept { return *m_generator; }

    int render_distance() const noexcept { return m_settings.render.render_distance; }

    void advance(double dt);

private:
    CelestialView celestial_view(const vector3d& eye, bool submerged) const;

    void handle_game_keys(const fizmo::windows::InputManager& input);

    void act(PauseAction action);

    SettingsHooks make_hooks();

    void apply_character();

    void apply_physics();

    void apply_light_format();

    static CapsuleRenderer make_capsule(const RenderSettings& r);

    void autosave(double dt);

    void respawn_player();

    void select_hotbar(const fizmo::windows::InputManager& input);

    void restore_inventory(const WorldState& state);

    void store_inventory(WorldState& state);

    void restore_vitals(const WorldState& state);

    void store_vitals(WorldState& state) const;

    BodyState felt_body() const noexcept;

    void tick_survival(double dt);

    VitalsContext vitals_context() const;

    std::string activity_name() const;

    void touching_ground(HeatEnvironment& e) const;

    void scan_heat(double frame_dt);

    HeatGearRegistry&       heat_gear()       noexcept { return m_heat_gear; }
    const HeatGearRegistry& heat_gear() const noexcept { return m_heat_gear; }

    double temperature_at(const vector3d& p) const;

    void update_body_heat(double dt);

    int simulation_distance() const noexcept { return vmax(m_settings.streaming.simulation_distance, 1); }

    static std::vector<ColumnPos> columns_around(const ColumnPos& c, int radius);

    void ensure_player_terrain();

    void update_streaming(bool force);

    std::string water_weather_text() const;

    std::string weather_text() const;

    double precipitation_brightness(const vector3d& eye) const;

    double declination() const noexcept;

    void update_weather(double frame_dt, bool running);

    void update_water_weather(double dt);

    void spawn_splashes();

    double wave_scale_at(double x, double y) const;

    void apply_waves();

    void restore_weather(const WorldState& s);

    void store_weather(WorldState& s) const;

    void tick_once(double dt);

    using Clock = std::chrono::steady_clock;

    static double ms_between(Clock::time_point a, Clock::time_point b) noexcept {
        return std::chrono::duration<double, std::milli>(b - a).count();
    }

    inline static const vector3d   LEGACY_SUN{ -0.35, 0.55, -1.0 };
    inline static const Identifier HEAT_MODIFIER{ core_id(Kind::Attribute, "body_heat") };
    inline static const Identifier INVENTORY_LOAD{ core_id(Kind::Survival, "inventory") };

    static constexpr int         LOOK_SETTLE_FRAMES   = 2;
    static constexpr double      SECONDS_PER_MINUTE   = 60.0;
    static constexpr double      CLIMATE_REFRESH      = 0.1;
    static constexpr double      GROUND_PROBE         = 0.05;
    static constexpr int         WATER_SAMPLES        = 3;
    static constexpr int         WATER_SPACING        = 8;
    static constexpr double      WIND_TURN            = 0.4;
    static constexpr double      WIND_CALM            = 0.6;
    static constexpr double      WIND_GUST            = 2.4;
    static constexpr double      HALF                 = 0.5;
    static constexpr double      PERCENT              = 100.0;
    static constexpr double      DROP_SKY_LIGHT       = 0.95;
    static constexpr double      DROP_MIN_LIGHT       = 0.08;
    static constexpr double      RAIN_SWELL           = 0.45;
    static constexpr std::size_t TEXT_BUFFER          = 160;
    static constexpr double      SPLASH_PARTICLES     = 6.0;
    static constexpr int         MAX_SPLASH_PARTICLES = 40;
    static constexpr double      SPLASH_OUT_MIN       = 0.5;
    static constexpr double      SPLASH_OUT_MAX       = 2.5;
    static constexpr double      SPLASH_UP_MIN        = 1.5;
    static constexpr double      SPLASH_UP_MAX        = 3.5;

    static Color opaque(const Color& c) noexcept { return Color(c.red(), c.green(), c.blue()); }

    void gather_displacers();

    void gather_lights();

    void gather_occluders(bool show_player);

    static AttributeRegistry make_attributes();

    static GameSettings with_seed(GameSettings s) {
        if (s.world.seed == WorldSettings::RANDOM_SEED) s.world.seed = SecureRandom::seed();
        return s;
    }

    static ParticleTypeRegistry make_particle_types() {
        ParticleTypeRegistry r;
        ParticleTypeRegistry::register_defaults(r);
        return r;
    }

    static MovementModeRegistry make_movement_modes(const CharacterSettings& character) {
        MovementModeRegistry r;
        MovementModeRegistry::register_defaults(r, character);
        return r;
    }

    GameSettings                    m_settings;
    GameSettings                    m_default_settings;
    BlockRegistry                   m_registry;
    DefaultBlocks                   m_block_ids;
    AttributeRegistry               m_attributes;
    MovementModeRegistry            m_movement_modes;
    ParticleTypeRegistry            m_particle_types;
    World                           m_world;
    std::unique_ptr<WorldGenerator> m_generator;
    JobSystem                       m_jobs;
    ChunkStreamer                   m_streamer;
    TerrainMeshes                   m_terrain;
    EntityPhysics                   m_physics;
    ParticleSystem                  m_particles;
    EntityManager                   m_entities;
    Player*                         m_player;
    GameCamera                      m_camera;
    std::unique_ptr<fizmo::windows::CursorLock> m_cursor;

    WorldClock                      m_clock;
    std::unique_ptr<SunPath>        m_sun_path;
    SkyState                        m_sky;
    LightingDirector                m_lighting;
    CelestialRenderer               m_celestial;
    DynamicLights                   m_dynamic_lights;
    std::vector<DynamicLight>       m_frame_lights;
    WorldRenderer                   m_world_renderer;
    CapsuleRenderer                 m_capsule;
    ReflectionPlanes                m_reflections;
    BlockOutlineRenderer            m_outline;
    Hud                             m_hud;
    SettingsMenu                    m_menu;
    SettingsRegistry                m_settings_registry;
    LightingPresets                 m_presets = LightingPresets::builtin();
    WaterPresets                    m_water_presets = WaterPresets::builtin();
    WaterSettings                   m_applied_water;
    bool                            m_flow_applied = false;
    int                             m_applied_sea  = RealisticFluid::NO_SEA;
    std::vector<FluidDisplacer>     m_displacers;
    bool                            m_display_dirty = false;
    bool                            m_quit_requested = false;
    double                          m_autosave_timer = 0.0;
    WorldSlot                       m_slot;
    std::unique_ptr<ChunkArchive>   m_archive;
    PauseMenu                       m_pause;
    WeatherSystem                   m_weather;
    PrecipitationRenderer           m_precipitation;
    LocalWeather                    m_local_weather;
    PrecipitationEasing             m_rain_easing;
    HeatSensor                      m_heat_sensor;
    BodyHeat                        m_body_heat;
    HeatReading                     m_heat_reading;
    double                          m_heat_scan  = 0.0;
    double                          m_heat_speed = 1.0;
    HeatGearRegistry                m_heat_gear;
    HeatGear                        m_gear;
    PlayerSurvival                  m_survival{ m_settings };
    VitalsHud                       m_vitals_hud;
    ItemSystem                      m_items{ m_registry, m_block_ids };
    Inventory                       m_inventory;
    CursorStack                     m_cursor_stack;
    StackRules                      m_stack_rules{ m_items.items(), &m_settings.items };
    InventoryScreen                 m_inventory_screen;
    HotbarHud                       m_hotbar_hud;
    double                          m_inventory_weight = 0.0;
    std::uint64_t                   m_weight_revision  = ~std::uint64_t(0);
    std::uint64_t                   m_weight_items     = ~std::uint64_t(0);
    bool                            m_drinking    = false;
    CalendarDate                    m_date;
    BiomeClimate                    m_climate;
    ClimateField                    m_climate_field;
    SeededRandom                    m_weather_rng;
    double                          m_climate_timer = 0.0;
    double                          m_wind_angle = 0.0;
    vector3d                        m_wind{};
    WaveField                       m_waves;
    WaterWeather                    m_water_weather;
    WaterLookPtr                    m_water_look;
    CpuTimings                      m_cpu;
    fizmo::windows::GpuMemory       m_gpu_memory;
    CpuTimings                      m_cpu_frame;
    bool                            m_waves_on = false;
    bool                            m_key_consumed = false;
    int                             m_ignore_look = 0;
    WorldRenderStats                m_stats;

    SimulationArea                  m_simulation;
    FluidSimulator                  m_fluids;
    ColumnPos                       m_stream_center{};
    bool                            m_streaming_dirty = true;
    std::optional<RaycastHit>       m_target;
    std::string                     m_last_key;
    bool                            m_jump_latch = false;
    double                          m_frame_dt = 0.0;
    fizmo::windows::GpuTimings      m_gpu_sum;
    std::size_t                     m_gpu_frames = 0;
    double                          m_seconds = 0.0;
    double                          m_accumulator = 0.0;
    double                          m_alpha = 1.0;
    unsigned int                    m_viewport_w = 1;
    unsigned int                    m_viewport_h = 1;
};

} // namespace voxelspire

#endif // VOXELSPIRE_GAME_HPP