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
#include "../render/world_renderer.hpp"
#include "../sky/renderer.hpp"
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
};

class Game {
public:
    explicit Game(const GameSettings& settings = GameSettings{}, const WorldGeneratorFactory& generator = {}, WorldSlot slot = {})
        : m_settings(with_seed(settings)),
          m_default_settings(m_settings),
          m_block_ids(DefaultBlocks::register_all(m_registry, settings.render.block_color_variation)),
          m_attributes(make_attributes()),
          m_movement_modes(make_movement_modes(settings.character)),
          m_particle_types(make_particle_types()),
          m_world(m_registry, m_settings.world),
          m_generator((generator ? generator : terrain_world(m_settings.terrain))(m_registry, m_settings.world)),
          m_jobs(settings.streaming.worker_threads),
          m_streamer(m_world, *m_generator, m_jobs),
          m_terrain(m_world, *m_generator, m_jobs, m_streamer),
          m_physics(m_world.settings()),
          m_particles(m_particle_types, settings.particles),
          m_entities(settings.entities),
          m_player(&m_entities.spawn<Player>(m_attributes, m_movement_modes)),
          m_camera(settings.camera, 1, 1, settings.render.render_distance_blocks()),
          m_clock(settings.day_cycle.start_time),
          m_sun_path(std::make_unique<DefaultSunPath>(m_settings.day_cycle, m_settings.celestial)),
          m_capsule(make_capsule(settings.render)) {
        m_world.attach_lighting(make_light_engine(m_world, m_settings.world.light_format));
        m_world.set_wave_sampler([this](double x, double y) { return m_waves.at(x, y, wave_scale_at(x, y)); });
        m_climate_field = ClimateField(m_settings.world.seed);
        m_body_heat.reset(m_settings.physics.heat);
        m_lighting.configure(m_settings.lighting);
        m_celestial.configure(m_settings.celestial);
        m_terrain.configure(m_settings.render, m_settings.streaming, m_settings.lod, m_settings.lighting);
        m_terrain.set_wave_detail(m_settings.render.wave_detail);
        m_camera.add_rig(std::make_unique<FirstPersonRig>());
        m_camera.add_rig(std::make_unique<ThirdPersonBackRig>());
        m_camera.add_rig(std::make_unique<ThirdPersonFrontRig>());
        apply_character();
        apply_physics();
        SettingsPages::register_personal(m_settings_registry, make_hooks(), m_presets);
        m_slot = std::move(slot);
        m_pause.set_subtitle(m_slot.name);

        if (!m_slot.folder.empty()) {
            m_archive = std::make_unique<ChunkArchive>(m_slot.folder, m_registry);
            m_streamer.set_archive(m_archive.get());
        }
    }

    ~Game() { m_jobs.shutdown(); }

    Game(const Game&) = delete;
    Game& operator=(const Game&) = delete;

    void start(fizmo::windows::Window& window, unsigned int viewport_w, unsigned int viewport_h) {
        m_registry.lock();
        m_cursor = std::make_unique<fizmo::windows::CursorLock>(window);
        const ColumnPos spawn = m_slot.state.has_player ? World::column_of(m_slot.state.position) : m_generator->spawn_column();
        m_streamer.load_now(columns_around(spawn, simulation_distance()));
        m_world.update_lighting(std::numeric_limits<double>::max());
        if (m_slot.state.has_time) m_clock.set(m_slot.state.day, m_slot.state.time);
        restore_weather(m_slot.state);

        if (m_slot.state.has_player) {
            m_player->respawn(m_slot.state.position);
            m_player->set_look(m_slot.state.yaw, m_slot.state.pitch);
        } else {
            respawn_player();
        }

        update_streaming(true);
        m_camera.set_viewport(viewport_w, viewport_h);
        m_viewport_w = viewport_w;
        m_viewport_h = viewport_h;
        m_camera.update(*m_player, m_world, 1.0, 0.0);
        rebuild_menu();
    }

    void shutdown() noexcept { if (m_cursor) m_cursor->unlock(); }

    bool save_world() {
        if (!m_archive) return false;
        const bool chunks = m_streamer.save(*m_archive);
        WorldState state;
        state.has_player = true;
        state.position   = m_player->position();
        state.yaw        = m_player->yaw();
        state.pitch      = m_player->pitch();
        state.has_time   = true;
        state.time       = m_clock.time();
        state.day        = m_clock.day();
        store_weather(state);
        m_autosave_timer = 0.0;
        m_slot.state     = state;
        const bool wrote = !m_slot.write_state || m_slot.write_state(state);
        return chunks && wrote;
    }

    void request_quit_to_title() noexcept { m_quit_requested = true; }
    bool take_quit_request() noexcept { const bool q = m_quit_requested; m_quit_requested = false; return q; }
    const std::filesystem::path& save_folder() const noexcept { return m_slot.folder; }

    bool mouse_locked() const noexcept { return m_cursor && m_cursor->locked(); }

    void handle_event(const fizmo::windows::WindowEvent& e) {
        using fizmo::windows::WindowEventType;
        if (m_menu.is_open() && m_menu.on_event(e)) { if (e.type == WindowEventType::KeyPress) m_key_consumed = true; return; }
        if (!m_menu.is_open() && m_pause.on_event(e)) { if (e.type == WindowEventType::KeyPress) m_key_consumed = true; return; }

        switch (e.type) {
            case WindowEventType::WindowResize:
                if (e.x > 0 && e.y > 0) { m_viewport_w = e.x; m_viewport_h = e.y; m_camera.set_viewport(e.x, e.y); }
                break;
            case WindowEventType::MouseClick:
                if (m_cursor && !m_cursor->locked() && !paused()) lock_cursor();
                break;
            case WindowEventType::KeyPress:
                m_last_key = e.key_name;
                break;
            default: break;
        }
    }

    void update(double frame_dt, const fizmo::windows::InputManager& input) {
        const auto update_start = Clock::now();
        m_cpu_frame = CpuTimings{};
        m_frame_dt = frame_dt;
        m_seconds += frame_dt;
        autosave(frame_dt);
        const InputBindings& keys = m_settings.bindings;
        if (keys.just_pressed(input, Action::OpenMenu) && !m_key_consumed) toggle_menu();
        m_key_consumed = false;

        if (m_menu.is_open()) {
            m_menu.update(input, frame_dt);
            apply_settings(m_menu.take_changes());
            if (m_menu.take_close_request()) close_options();
        } else if (m_pause.is_open()) {
            m_pause.update(input, frame_dt);
            act(m_pause.take_action());
        }

        const bool playing = !paused();
        if (playing) handle_game_keys(input);
        MovementIntent intent;

        if (playing) {
            intent.forward = (keys.is_down(input, Action::MoveForward) ? 1.0 : 0.0) - (keys.is_down(input, Action::MoveBack) ? 1.0 : 0.0);
            intent.strafe  = (keys.is_down(input, Action::MoveRight) ? 1.0 : 0.0) - (keys.is_down(input, Action::MoveLeft) ? 1.0 : 0.0);
            if (keys.just_pressed(input, Action::Jump)) m_jump_latch = true;
            intent.jump    = keys.is_down(input, Action::Jump) || m_jump_latch;
            intent.sprint  = keys.is_down(input, Action::Sprint);
            intent.crouch  = keys.is_down(input, Action::Crouch);
            intent.crawl   = keys.is_down(input, Action::Crawl);
            intent.swim    = keys.is_down(input, Action::Swim);
            intent.alt     = keys.is_down(input, Action::Alt);
        }

        m_player->set_intent(intent);
        const auto ticks_start = Clock::now();
        if (playing || !m_settings.menu.pause_game) advance(frame_dt * vmax(m_settings.simulation.game_speed, 0.0));
        const auto streaming_start = Clock::now();
        m_cpu_frame.ticks = ms_between(ticks_start, streaming_start);
        ensure_player_terrain();
        update_streaming(false);
        m_streamer.update(m_settings.streaming.max_column_jobs);
        m_jobs.run_completions(m_settings.streaming.result_time_budget_ms);
        const auto light_start = Clock::now();
        m_world.update_lighting(m_settings.lighting.update_budget_ms);
        const auto terrain_start = Clock::now();
        m_cpu_frame.lighting = ms_between(light_start, terrain_start);
        m_terrain.update(m_player->eye_position(m_alpha));
        m_cpu_frame.streaming = ms_between(streaming_start, light_start) + ms_between(terrain_start, Clock::now());
        m_particles.update(frame_dt, m_camera.camera().position(), m_world, m_world.settings().gravity);
        update_weather(frame_dt, playing || !m_settings.menu.pause_game);
        scan_heat(frame_dt);
        m_target = raycast_blocks(m_world, m_player->eye_position(m_alpha), m_player->look_direction(), m_player->attributes().value(Attributes::BlockReach));
        m_camera.update(*m_player, m_world, m_alpha, frame_dt);
        m_cpu_frame.update = ms_between(update_start, Clock::now());
        m_cpu.blend_update(m_cpu_frame);
    }

    void render(fizmo::windows::Renderer& renderer, double fps_average) {
        const auto render_start = Clock::now();
        render_frame(renderer, fps_average);
        CpuTimings::blend(m_cpu.render, ms_between(render_start, Clock::now()));
    }

    void render_frame(fizmo::windows::Renderer& renderer, double fps_average) {
        m_terrain.upload(renderer);
        const vector3d eye = m_camera.camera().position();
        fizmo::graphics::Swell3D swell = m_waves.swell(eye);
        swell.detail = static_cast<float>(m_waves_on ? m_settings.render.wave_detail * Chunk::SIZE : 0);
        m_lighting.set_swell(swell);
        m_sun_path->set_declination(declination());
        m_sky = m_sun_path->evaluate(m_clock.time());
        SkyWeather::apply(m_sky, m_local_weather, m_settings.weather, m_settings.weather_view);
        gather_lights();
        const BlockPos eye_cell = BlockPos::containing(eye);
        const double eye_water = m_world.fluid_height(eye_cell);
        const bool submerged = eye_water > 0.0 && eye.z < eye_cell.z + eye_water;
        const Color medium = submerged ? opaque(m_world.traits_at(eye_cell).face(Face::Up).base) : Color();
        const bool third_person = m_camera.shows_player();
        const bool show_body = third_person || m_settings.render.first_person_body;
        gather_occluders(show_body);
        m_reflections.update(m_world, m_camera.camera(), m_settings.lighting, m_settings.render.render_distance_blocks());
        m_lighting.set_reflection_planes(m_reflections.planes());
        const fizmo::graphics::SceneLighting3D& scene = m_lighting.build(m_sky, m_world, eye, m_frame_lights, m_seconds, submerged ? &medium : nullptr);
        renderer.set_scene_lighting(scene);
        renderer.begin_3d(m_camera.camera());
        renderer.set_light_3d(fizmo::graphics::Light3D::sun(LEGACY_SUN));
        m_celestial.render(renderer, celestial_view(eye, submerged));
        WorldRenderOptions options;
        options.render_distance = m_settings.render.render_distance_blocks();
        options.cave_culling    = m_settings.render.cave_culling;
        options.face_culling    = m_settings.render.face_culling;
        options.shadow_distance = m_lighting.shadows() ? m_lighting.shadow_distance() : 0.0;
        options.sun_casters     = scene.sun_shadow.enabled && scene.sun_intensity > 0.0f;
        for (const auto& l : scene.point_lights) if (l.casts_shadows) options.point_casters.push_back({ l.position, l.radius });
        options.reflections     = m_reflections.planes();
        options.reflection_distance = vmin(static_cast<double>(m_settings.lighting.reflection_view_chunks) * Chunk::SIZE + m_reflections.farthest(), options.render_distance);
        m_stats = m_world_renderer.render(renderer, m_terrain, m_camera.camera(), options);
        m_particles.render(renderer, eye, [this](const BlockPos& b) { return m_lighting.render_light_at(m_world, b.center()); });
        if (!submerged) m_precipitation.render(renderer, precipitation_brightness(eye));

        if (show_body) {
            const auto view = third_person ? fizmo::graphics::View3D::Everywhere : fizmo::graphics::View3D::ReflectionsOnly;
            m_capsule.render(renderer, *m_player, m_alpha, m_lighting.render_light_at(m_world, m_player->interpolated_position(m_alpha) + vector3d{ 0.0, 0.0, m_player->height() * 0.5 }), view, !m_lighting.capsule_shadows());
        }

        if (m_target && !paused()) m_outline.render(renderer, m_world, *m_target, m_settings.render);
        renderer.end_3d();

        if (m_menu.is_open()) {
            m_menu.render(renderer, m_viewport_w, m_viewport_h, m_settings.menu);
            return;
        }

        if (m_pause.is_open()) {
            m_pause.render(renderer, m_viewport_w, m_viewport_h, m_settings.menu);
            return;
        }

        track_gpu(renderer);

        if (m_hud.due(m_frame_dt, m_settings.hud)) {
            m_hud.set_info(hud_info(fps_average), m_settings.hud);
            m_gpu_sum = {};
            m_gpu_frames = 0;
        }

        m_hud.render(renderer, m_viewport_w, m_viewport_h, m_settings.hud);
    }

    void track_gpu(fizmo::windows::Renderer& renderer) {
        renderer.set_gpu_timing(m_settings.hud.show_debug && m_settings.hud.sections.gpu);
        if (m_settings.hud.show_debug) m_gpu_memory = renderer.gpu_memory();
        const fizmo::windows::GpuTimings t = renderer.gpu_timings();
        if (!t.valid) return;
        m_gpu_sum.valid = true;
        m_gpu_sum.total_ms += t.total_ms;
        for (std::size_t i = 0; i < t.pass_ms.size(); ++i) m_gpu_sum.pass_ms[i] += t.pass_ms[i];
        ++m_gpu_frames;
    }

    fizmo::windows::GpuTimings average_gpu() const noexcept {
        fizmo::windows::GpuTimings out = m_gpu_sum;
        if (m_gpu_frames == 0) return out;
        const double n = static_cast<double>(m_gpu_frames);
        out.total_ms /= n;
        for (double& ms : out.pass_ms) ms /= n;
        return out;
    }

    void open_menu() {
        if (paused()) return;
        m_pause.open();
        if (m_cursor) m_cursor->unlock();
    }

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

    void close_options() {
        if (!m_menu.is_open()) return;
        m_menu.close();
        if (m_settings.menu.save_on_close) save_settings();
    }

    void save_and_exit() {
        if (m_archive && !save_world()) {
            m_pause.set_status("The world could not be saved. Check that its folder can be written to.", true);
            return;
        }

        request_quit_to_title();
    }

    void lock_cursor() {
        if (!m_cursor) return;
        m_cursor->lock();
        m_ignore_look = LOOK_SETTLE_FRAMES;
    }

    void toggle_menu() {
        if (m_menu.is_open()) close_options();
        else if (m_pause.is_open()) close_menu();
        else open_menu();
    }

    bool          paused()     const noexcept { return m_pause.is_open() || m_menu.is_open(); }
    bool          menu_open()  const noexcept { return m_menu.is_open(); }
    SettingsMenu& menu()       noexcept { return m_menu; }
    PauseMenu&    pause_menu() noexcept { return m_pause; }

    bool save_settings() { return SettingsFile::save(m_menu.tabs(), m_settings.menu.file); }

    void load_settings() {
        const SettingsLoad loaded = SettingsFile::load(m_menu.tabs(), m_settings.menu.file);
        apply_settings(loaded.applied);
        m_menu.report_problems(loaded.problems);
    }

    void rebuild_menu() {
        m_menu.set_tabs(m_settings_registry.build(m_settings, m_default_settings));
        load_settings();
    }

    SettingsRegistry& settings_registry() noexcept { return m_settings_registry; }
    LightingPresets&  lighting_presets()  noexcept { return m_presets; }
    WaterPresets&     water_presets()     noexcept { return m_water_presets; }

    bool take_display_change() noexcept { const bool d = m_display_dirty; m_display_dirty = false; return d; }

    void apply_settings(std::uint32_t groups) {
        if (groups == Apply::Nothing) return;
        if (groups & Apply::Display)     m_display_dirty = true;
        if (groups & Apply::Camera)      m_camera.set_settings(m_settings.camera, m_settings.render.render_distance_blocks());
        if (groups & Apply::Character)   apply_character();
        if (groups & Apply::Physics)     apply_physics();
        if (groups & Apply::Celestial)   m_celestial.configure(m_settings.celestial);
        if (groups & (Apply::Sky | Apply::Celestial)) m_sun_path = std::make_unique<DefaultSunPath>(m_settings.day_cycle, m_settings.celestial);
        if (groups & Apply::LightFormat) apply_light_format();
        if (groups & Apply::Lighting)    set_lighting(m_settings.lighting);
        if (groups & (Apply::Terrain | Apply::Streaming)) m_terrain.configure(m_settings.render, m_settings.streaming, m_settings.lod, m_settings.lighting);
        if (groups & Apply::Terrain)     m_terrain.set_wave_detail(m_settings.render.wave_detail);
        if (groups & Apply::Streaming)   set_render_distance(m_settings.render.render_distance);
        if (groups & Apply::Particles)   m_particles.settings() = m_settings.particles;
        if (groups & Apply::Entities)    m_entities.settings() = m_settings.entities;
        if (groups & Apply::Player)      m_capsule = make_capsule(m_settings.render);
    }

    HudInfo hud_info(double fps_average) const {
        HudInfo info;
        info.fps_average    = fps_average;
        info.position       = m_player->interpolated_position(m_alpha);
        info.velocity       = m_player->velocity();
        info.on_ground      = m_player->on_ground();
        info.camera_mode    = m_camera.mode_name();
        info.stats          = m_stats;
        info.target         = m_target;
        info.target_name    = m_target ? m_world.block_at(m_target->block).identifier().str() : std::string();
        info.mouse_captured = mouse_locked();
        info.last_key       = m_last_key;
        info.movement_mode  = m_player->movement_mode() ? m_player->movement_mode()->id().str() : std::string("none");
        info.entity_count    = m_entities.size();
        info.entity_contacts = m_entities.last_contact_count();
        info.air_model         = m_world.settings().air_resistance->id().name();
        info.terminal_velocity = m_physics.terminal_velocity(*m_player, m_player->effective_gravity_scale(), m_player->effective_drag_scale());
        info.streaming         = m_streamer.stats();
        info.simulation_distance = m_settings.streaming.simulation_distance;
        info.detail_distance     = m_settings.streaming.detail_distance;
        info.render_distance     = m_settings.render.render_distance;
        info.daylight            = m_sky.daylight;
        info.worker_threads      = m_jobs.worker_count();
        info.particles           = m_particles.count();
        info.particle_emitters   = m_particles.emitter_count();
        const BlockPos feet      = BlockPos::containing(m_player->position());
        const Biome* biome       = m_generator->biome_at(feet.x, feet.y);
        info.biome               = biome ? biome->name() : std::string();
        info.light               = m_world.lighting() ? m_world.lighting()->stats() : LightStats{};
        info.lighting_preset     = m_settings.lighting.name;
        info.time_hours          = m_clock.hours();
        info.day                 = m_clock.day();
        info.date                = m_date.describe();
        info.weather             = weather_text();
        info.water_weather       = water_weather_text();
        info.temperature         = m_local_weather.temperature;
        info.heat_on             = m_settings.physics.heat.model != HeatModel::Off;
        info.body_temperature    = m_body_heat.body();
        info.felt_temperature    = m_heat_reading.felt;
        info.heat_warmth         = m_heat_reading.sources;
        info.heat_surround       = m_heat_reading.surround;
        info.heat_exchange       = m_heat_reading.exchange;
        info.heat_player         = m_settings.physics.heat.affects_player;
        info.clothing            = m_gear.insulation;
        info.body_state          = body_state_name(m_body_heat.state(m_settings.physics.heat));
        info.temperature_unit    = m_settings.weather_view.unit;
        info.day_cycle           = m_settings.day_cycle.enabled;
        info.light_here          = light_level_at(m_player->eye_position(m_alpha));
        info.dynamic_lights      = m_lighting.scene().point_lights.size();
        info.shadow_casters      = m_stats.shadow_casters;
        info.shadow_buried       = m_stats.shadow_buried;
        info.cpu                 = m_cpu;
        info.water_preset        = m_settings.water.name;
        info.fluid_model         = m_world.settings().fluid_resistance->id().name();
        info.flow_model          = m_world.fluid_rules().id().name();
        info.fluids              = m_fluids.stats();
        info.reflection_planes   = m_reflections.planes().size();
        info.gpu                 = average_gpu();
        info.vram                = m_gpu_memory;
        info.ram                 = ProcessMemory::read();
        return info;
    }

    LightLevel light_level_at(const vector3d& p) const { return m_lighting.light_level_at(m_world, p, m_frame_lights); }

    const LightingSettings& lighting() const noexcept { return m_settings.lighting; }

    void set_lighting(const LightingSettings& lighting) {
        m_settings.lighting = lighting;
        m_lighting.configure(lighting);
        m_terrain.configure(m_settings.render, m_settings.streaming, m_settings.lod, lighting);
    }

    void cycle_lighting() {
        if (const LightingSettings* next = m_presets.next(m_settings.lighting.name)) set_lighting(*next);
    }

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

    void set_render_distance(int chunks) noexcept {
        m_settings.render.render_distance = static_cast<int>(RenderLimits::render_distance.clamp(chunks));
        m_camera.set_view_distance(m_settings.render.render_distance_blocks());
        m_streaming_dirty = true;
    }

    void set_simulation_distance(int chunks) noexcept {
        m_settings.streaming.simulation_distance = static_cast<int>(StreamingLimits::simulation_distance.clamp(chunks));
        m_streaming_dirty = true;
    }

    TerrainMeshes&       terrain()          noexcept { return m_terrain; }
    ChunkStreamer&       streamer()         noexcept { return m_streamer; }
    JobSystem&           jobs()             noexcept { return m_jobs; }
    const WorldGenerator& generator() const noexcept { return *m_generator; }

    int render_distance() const noexcept { return m_settings.render.render_distance; }

    void advance(double dt) {
        const double tick = 1.0 / m_settings.simulation.tick_rate;
        m_accumulator += dt;
        int ticks = 0;

        while (m_accumulator >= tick && ticks < m_settings.simulation.max_ticks_per_frame) {
            tick_once(tick);
            m_accumulator -= tick;
            ++ticks;
        }

        if (ticks == m_settings.simulation.max_ticks_per_frame) m_accumulator = 0.0;
        m_alpha = m_accumulator / tick;
    }

private:
    CelestialView celestial_view(const vector3d& eye, bool submerged) const {
        CelestialView view;
        view.eye        = eye;
        view.far_plane  = m_camera.camera().far_plane();
        view.sky        = m_sky;
        view.orbit_axis = CelestialRenderer::orbit_axis(*m_sun_path);
        view.days       = static_cast<double>(m_clock.day()) + m_clock.time();
        view.seconds    = m_seconds;
        view.submerged  = submerged;
        return view;
    }

    void handle_game_keys(const fizmo::windows::InputManager& input) {
        const InputBindings& keys = m_settings.bindings;

        if (m_ignore_look > 0) {
            --m_ignore_look;
        } else if (mouse_locked()) {
            const double sens = m_settings.controls.mouse_sensitivity;
            const double invert = m_settings.controls.invert_y ? -1.0 : 1.0;
            m_player->add_look(-input.mouse_delta_x() * sens, -input.mouse_delta_y() * sens * invert);
        }

        if (keys.just_pressed(input, Action::CycleCamera))        m_camera.cycle_rig();
        if (keys.just_pressed(input, Action::ToggleHud))          m_settings.hud.show_debug = !m_settings.hud.show_debug;
        if (keys.just_pressed(input, Action::RenderDistanceUp))   set_render_distance(m_settings.render.render_distance + m_settings.render.render_distance_step);
        if (keys.just_pressed(input, Action::RenderDistanceDown)) set_render_distance(m_settings.render.render_distance - m_settings.render.render_distance_step);
        if (keys.just_pressed(input, Action::CycleLighting))      cycle_lighting();
    }

    void act(PauseAction action) {
        switch (action) {
            case PauseAction::None:        break;
            case PauseAction::Resume:      close_menu(); break;
            case PauseAction::Options:     open_options(); break;
            case PauseAction::SaveAndExit: save_and_exit(); break;
        }
    }

    SettingsHooks make_hooks() {
        SettingsHooks hooks;
        hooks.hour       = [this] { return m_clock.hours(); };
        hooks.set_hour   = [this](double h) { m_clock.set_time(h / DayCycleSettings::HOURS_PER_DAY); };
        hooks.save       = [this] { save_settings(); };
        hooks.reload     = [this] { load_settings(); };
        hooks.respawn    = [this] { respawn_player(); };
        hooks.save_world = [this] { save_world(); };
        return hooks;
    }

    void apply_character() {
        const CharacterSettings& c = m_settings.character;
        m_player->set_body(c.body());
        AttributeMap& a = m_player->attributes();
        a.set_base(Attributes::MovementSpeed, c.walk_speed);
        a.set_base(Attributes::JumpVelocity, c.jump_velocity);
        a.set_base(Attributes::GroundAcceleration, c.ground_acceleration);
        a.set_base(Attributes::AirAcceleration, c.air_acceleration);
        a.set_base(Attributes::BlockReach, c.reach);
        MovementModeRegistry::register_defaults(m_movement_modes, c);
        m_player->forget_movement_mode();
    }

    void apply_physics() {
        const WaterSettings& water = m_settings.water;
        WorldSettings w      = m_world.settings();
        w.gravity            = m_settings.world.gravity;
        w.air_resistance     = m_settings.physics.air_resistance(w.gravity, m_settings.world.air_resistance);
        w.fluid_resistance   = water.drag(m_settings.character.mass);
        w.fluid_buoyancy     = water.buoyancy;
        w.fluid_sink_speed   = water.sink_speed;
        w.current_speed      = water.current_speed;
        w.current_push       = water.current_push;
        w.wade_slowdown      = water.wade_slowdown;
        w.fall_break_depth   = water.fall_break_depth;
        w.fluid_updates      = water.updates;
        m_player->set_mass(m_settings.character.mass);
        apply_waves();
        const bool new_rules = !m_flow_applied || !water.same_flow(m_applied_water) || m_applied_sea != m_settings.terrain.sea_level;
        if (new_rules) w.fluid_rules = water.rules(m_settings.terrain.sea_level);
        m_applied_sea        = m_settings.terrain.sea_level;
        m_applied_water      = water;
        m_flow_applied       = true;
        m_world.set_physics(w);
        m_physics.set_settings(m_world.settings());
        if (new_rules) m_terrain.remesh_all();
    }

    void apply_light_format() {
        if (m_world.light_format() == m_settings.world.light_format) return;
        m_world.attach_lighting(make_light_engine(m_world, m_settings.world.light_format));
        m_terrain.remesh_all();
    }

    static CapsuleRenderer make_capsule(const RenderSettings& r) {
        return CapsuleRenderer(r.capsule_segments, r.capsule_rings, r.player_color, r.player_visor);
    }

    void autosave(double dt) {
        const double every = m_settings.saves.autosave_minutes * SECONDS_PER_MINUTE;
        if (!m_archive || every <= 0.0) return;
        m_autosave_timer += dt;
        if (m_autosave_timer >= every) save_world();
    }

    void respawn_player() {
        m_streamer.load_now(columns_around(m_generator->spawn_column(), 1));
        m_player->respawn(m_generator->spawn_point(m_world));
        m_body_heat.reset(m_settings.physics.heat);
    }

    void touching_ground(HeatEnvironment& e) const {
        if (!m_player->on_ground()) return;
        const vector3d p = m_player->position();
        const double half = m_player->width() * HALF;
        double temperature = 0.0, conductivity = 0.0;
        int count = 0;

        for (const double dx : { -half, half })
            for (const double dy : { -half, half }) {
                const BlockPos below = BlockPos::containing(p + vector3d{ dx, dy, -GROUND_PROBE });
                const BlockTraits& t = m_world.traits_at(below);
                if (!t.solid) continue;
                temperature  += t.thermal.surface(e.air);
                conductivity += t.thermal.conductivity;
                ++count;
            }

        if (count == 0) return;
        e.touching     = true;
        e.ground       = temperature / count;
        e.conductivity = conductivity / count;
    }

    void scan_heat(double frame_dt) {
        m_heat_scan -= frame_dt;
        if (m_heat_scan > 0.0) return;
        m_heat_scan = HeatSensor::SCAN_INTERVAL;
        m_heat_sensor.scan(m_world, m_player->position(), m_settings.physics.heat);
    }

    HeatGearRegistry&       heat_gear()       noexcept { return m_heat_gear; }
    const HeatGearRegistry& heat_gear() const noexcept { return m_heat_gear; }

    double temperature_at(const vector3d& p) const {
        return m_local_weather.temperature + HeatSensor::measure(m_world, p, m_frame_lights, m_settings.physics.heat);
    }

    void update_body_heat(double dt) {
        const HeatSettings& s = m_settings.physics.heat;
        const vector3d center = m_player->position() + vector3d{ 0.0, 0.0, m_player->height() * HALF };
        const BlockPos cell = BlockPos::containing(center);
        HeatGear base;
        base.insulation = m_settings.character.clothing;
        m_gear = m_heat_gear.total(*m_player, base);
        HeatEnvironment e;
        e.air       = m_local_weather.temperature;
        e.sources   = m_heat_sensor.warmth(m_world, center, m_frame_lights, s);
        e.wind      = m_wind.magnitude();
        e.rain      = m_local_weather.precipitation == Precipitation::None ? 0.0 : vmin(m_local_weather.amount, 1.0);
        e.open_sky  = static_cast<double>(m_world.light_at(cell).sky) / LightLimits::MAX;
        e.submerged = m_physics.submerged_fraction(*m_player, m_world);
        touching_ground(e);
        m_heat_reading = BodyHeat::read(e, m_gear, s);
        m_body_heat.update(dt, s, m_heat_reading, m_gear);
        const double speed = m_body_heat.speed_factor(s) * m_gear.speed;
        if (speed == m_heat_speed) return;
        m_heat_speed = speed;
        AttributeInstance& a = m_player->attributes().get(Attributes::MovementSpeed);
        if (speed >= 1.0) a.remove_modifier(HEAT_MODIFIER);
        else a.add_modifier({ HEAT_MODIFIER, speed - 1.0, ModifierOp::MultiplyTotal });
    }

    int simulation_distance() const noexcept { return vmax(m_settings.streaming.simulation_distance, 1); }

    static std::vector<ColumnPos> columns_around(const ColumnPos& c, int radius) {
        std::vector<ColumnPos> out;

        for (int dy = -radius; dy <= radius; ++dy)
            for (int dx = -radius; dx <= radius; ++dx)
                if (dx * dx + dy * dy <= radius * radius) out.push_back({ c.x + dx, c.y + dy });

        std::sort(out.begin(), out.end(), [&c](const ColumnPos& a, const ColumnPos& b) {
            return (a.x - c.x) * (a.x - c.x) + (a.y - c.y) * (a.y - c.y) < (b.x - c.x) * (b.x - c.x) + (b.y - c.y) * (b.y - c.y);
        });

        return out;
    }

    void ensure_player_terrain() {
        const ColumnPos c = World::column_of(m_player->position());
        std::vector<ColumnPos> missing;

        for (int dy = -1; dy <= 1; ++dy)
            for (int dx = -1; dx <= 1; ++dx) {
                const ColumnPos n{ c.x + dx, c.y + dy };
                if (!m_world.column_loaded(n) && m_world.column_in_bounds(n)) missing.push_back(n);
            }

        if (!missing.empty()) m_streamer.load_now(missing);
    }

    void update_streaming(bool force) {
        const vector3d eye = m_player->eye_position(m_alpha);
        const ColumnPos here = World::column_of(eye);
        if (!force && !m_streaming_dirty && here == m_stream_center) return;
        m_stream_center = here;
        m_streaming_dirty = false;
        const double detail = static_cast<double>(m_settings.streaming.detail_distance) * Chunk::SIZE;
        LodSelection sel = LodSelector::select(eye, m_settings.render.render_distance_blocks(), detail, m_settings.lod, m_terrain.layout(), m_world);
        std::unordered_set<ColumnPos, ColumnPosHash> load(sel.detail_set.begin(), sel.detail_set.end());

        for (const ColumnPos& c : sel.detail_columns)
            for (int dy = -1; dy <= 1; ++dy)
                for (int dx = -1; dx <= 1; ++dx) load.insert({ c.x + dx, c.y + dy });

        for (const ColumnPos& c : columns_around(here, simulation_distance())) load.insert(c);
        std::vector<ColumnPos> wanted;
        double farthest = 0.0;

        for (const ColumnPos& c : load) {
            if (!m_world.column_in_bounds(c)) continue;
            wanted.push_back(c);
            farthest = vmax(farthest, LodSelector::column_distance(c, eye.x, eye.y));
        }

        std::sort(wanted.begin(), wanted.end(), [&eye](const ColumnPos& a, const ColumnPos& b) {
            return LodSelector::column_distance(a, eye.x, eye.y) < LodSelector::column_distance(b, eye.x, eye.y);
        });

        const double keep = farthest + Chunk::SIZE * (1 + vmax(m_settings.streaming.unload_margin, 0));
        m_streamer.request(std::move(wanted), eye, keep);
        m_terrain.set_selection(std::move(sel));
    }

    std::string water_weather_text() const {
        if (m_settings.water.flow != FlowModel::Realistic) return std::string();
        const WaterWeatherStats& st = m_water_weather.stats();
        const vector3d p = m_player->position();
        char buf[TEXT_BUFFER];
        std::snprintf(buf, sizeof(buf), "waves %.2f here (%.2f max), rain added %.1f blocks, drained %.1f, washed over %.1f",
                      m_waves.at(p.x, p.y, wave_scale_at(p.x, p.y)), m_waves.crest(wave_scale_at(p.x, p.y)),
                      st.rained / RealisticFluid::UNITS, st.drained / RealisticFluid::UNITS, st.spilled / RealisticFluid::UNITS);
        return buf;
    }

    std::string weather_text() const {
        const LocalWeather& w = m_local_weather;
        std::string out = weather_name(w.kind);
        if (w.precipitation != Precipitation::None) out += std::string(", ") + precipitation_name(w.precipitation) + " " + std::to_string(static_cast<int>(std::lround(w.amount * PERCENT))) + "%";
        else if (w.kind != WeatherKind::Clear) out += ", dry here";
        out += ", " + std::to_string(m_precipitation.count()) + " drops";
        return out;
    }

    double precipitation_brightness(const vector3d& eye) const {
        const LightLevel here = light_level_at(eye);
        const double sky = m_lighting.enabled() ? m_sky.daylight : 1.0;
        const double block = static_cast<double>(here.block()) / LightLimits::MAX;
        return vclamp(vmax(sky * DROP_SKY_LIGHT, block), DROP_MIN_LIGHT, 1.0);
    }

    double declination() const noexcept {
        if (m_settings.seasons.mode == SeasonMode::Off) return 0.0;
        return deg_to_rad(m_settings.seasons.sun_swing) * m_date.warmth;
    }

    void update_weather(double frame_dt, bool running) {
        const vector3d eye = m_camera.camera().position();
        m_date = Calendar::at(m_clock.day(), m_clock.time(), m_settings.seasons);
        m_climate_timer -= frame_dt;

        if (m_climate_timer <= 0.0) {
            m_climate_timer = CLIMATE_REFRESH;
            m_climate = m_climate_field.around(*m_generator, m_settings.terrain, m_settings.weather.temperature, eye);
        }

        const double game_days = static_cast<double>(m_clock.day()) + m_clock.time();
        BiomeClimate here = m_climate;
        here.temperature += m_climate_field.nearby_change(m_settings.weather.temperature, eye, game_days);
        m_local_weather = m_weather.local(here, eye.z, m_settings.terrain.sea_level, m_clock.time(), game_days, m_date, m_settings.weather, m_settings.seasons, eye.x, eye.y);
        m_rain_easing.apply(m_local_weather, frame_dt, m_weather, m_settings.weather);
        if (!running) return;
        m_wind_angle += frame_dt * WIND_TURN * (m_weather_rng.unit() - HALF);
        const double wind = m_settings.weather.wind * (WIND_CALM + WIND_GUST * m_local_weather.cloud);
        m_wind = vector3d{ std::cos(m_wind_angle) * wind, std::sin(m_wind_angle) * wind, 0.0 };
        m_precipitation.update(frame_dt, m_seconds, eye, m_world, m_local_weather, m_settings.weather_view, m_wind);
        const double storm = m_local_weather.cloud * (m_local_weather.kind == WeatherKind::Storm ? 1.0 : RAIN_SWELL) * m_settings.weather.wind;
        m_waves.update(m_settings.water.waves, storm, m_wind, m_seconds);
    }

    void update_water_weather(double dt) {
        const double hour_seconds = m_settings.day_cycle.real_hour_seconds();
        const WaterWeatherContext context{ m_world, m_block_ids.water, m_settings.water, m_waves, m_waves_on ? m_water_look.get() : nullptr, m_local_weather,
                                           m_player->position(), m_settings.terrain.sea_level, hour_seconds };
        m_water_weather.update(dt, context);
    }

    void spawn_splashes() {
        const ParticleTypeIndex* type = m_particle_types.find(Particles::Splash);

        for (const FluidSplash& splash : m_fluids.take_splashes()) {
            if (!type) continue;
            const int count = vmin(static_cast<int>(splash.strength * SPLASH_PARTICLES), MAX_SPLASH_PARTICLES);
            const vector3d at = splash.pos.center() + vector3d{ 0.0, 0.0, HALF };

            for (int i = 0; i < count; ++i) {
                const double a = m_weather_rng.range(0.0, 2.0 * PI), out = m_weather_rng.range(SPLASH_OUT_MIN, SPLASH_OUT_MAX);
                const vector3d v{ std::cos(a) * out, std::sin(a) * out, m_weather_rng.range(SPLASH_UP_MIN, SPLASH_UP_MAX) * std::sqrt(splash.strength) };
                m_particles.spawn(*type, at, v, m_weather_rng);
            }
        }
    }

    double wave_scale_at(double x, double y) const {
        if (!m_waves_on || !m_water_look) return 0.0;
        return (*m_water_look)(static_cast<int>(std::floor(x)), static_cast<int>(std::floor(y))).waves;
    }

    void apply_waves() {
        const WaterSettings& water = m_settings.water;
        m_waves_on = water.flow == FlowModel::Realistic && water.waves.enabled;

        if (!m_water_look) {
            const WorldGenerator* generator = m_generator.get();
            const TerrainSettings terrain = m_settings.terrain;

            m_water_look = std::make_shared<const WaterLook>([generator, terrain](int x, int y) {
                std::array<BiomeClimate, WATER_SAMPLES * WATER_SAMPLES> found{};
                std::size_t count = 0;

                for (int i = 0; i < WATER_SAMPLES; ++i) {
                    for (int j = 0; j < WATER_SAMPLES; ++j) {
                        const int sx = x + (i - WATER_SAMPLES / 2) * WATER_SPACING, sy = y + (j - WATER_SAMPLES / 2) * WATER_SPACING;
                        const Biome* biome = generator->biome_at(sx, sy);
                        if (biome) found[count++] = BiomeWeather::effective(*biome, terrain.biome(biome->id().str()));
                    }
                }

                if (count == 0) return WaterSample{};
                const BiomeClimate c = BiomeWeather::blend(found.data(), count);
                return WaterSample{ vclamp(c.waves, 0.0, 1.0), c.water };
            });
        }

        m_terrain.set_water(m_water_look, m_waves_on);
    }

    void restore_weather(const WorldState& s) {
        if (!s.has_weather) {
            m_weather.force(WeatherKind::Clear, m_settings.weather, m_weather_rng);
            WeatherState w = m_weather.state();
            w.remaining = m_settings.weather.clear_days;
            m_weather.set_state(w);
            return;
        }

        WeatherState w;
        w.kind      = static_cast<WeatherKind>(vclamp(s.weather_kind, 0, static_cast<int>(WeatherKind::Storm)));
        w.intensity = s.weather_intensity;
        w.target    = s.weather_target;
        w.remaining = s.weather_remaining;
        w.hail      = s.weather_hail;
        m_weather.set_state(w);
    }

    void store_weather(WorldState& s) const {
        const WeatherState& w = m_weather.state();
        s.has_weather       = true;
        s.weather_kind      = static_cast<int>(w.kind);
        s.weather_intensity = w.intensity;
        s.weather_target    = w.target;
        s.weather_remaining = w.remaining;
        s.weather_hail      = w.hail;
    }

    void tick_once(double dt) {
        m_simulation.center = m_player->position();
        m_simulation.radius = static_cast<double>(simulation_distance()) * Chunk::SIZE;
        m_simulation.world  = &m_world;
        TickContext ctx{ m_world, m_physics, m_entities, dt, &m_simulation };
        gather_displacers();
        const auto fluids_start = Clock::now();
        m_fluids.update(m_world, dt, m_simulation.center, m_simulation.radius);
        m_cpu_frame.fluids += ms_between(fluids_start, Clock::now());
        spawn_splashes();
        update_water_weather(dt);
        m_entities.tick(ctx);
        update_body_heat(dt);
        m_clock.advance(dt, m_settings.day_cycle);
        const double day_seconds = m_settings.day_cycle.real_day_seconds();
        m_date = Calendar::at(m_clock.day(), m_clock.time(), m_settings.seasons);
        m_weather.update(day_seconds > 0.0 ? dt / day_seconds : 0.0, dt, m_settings.weather, m_date, m_weather_rng);
        if (m_jump_latch) { m_jump_latch = false; MovementIntent i = m_player->intent(); i.jump = false; m_player->set_intent(i); }
        const double void_z = m_world.settings().min_z - m_world.settings().void_depth;
        if (m_player->position().z < void_z) respawn_player();
    }

    using Clock = std::chrono::steady_clock;

    static double ms_between(Clock::time_point a, Clock::time_point b) noexcept {
        return std::chrono::duration<double, std::milli>(b - a).count();
    }

    inline static const vector3d   LEGACY_SUN{ -0.35, 0.55, -1.0 };
    inline static const Identifier HEAT_MODIFIER{ core_id(Kind::Attribute, "body_heat") };

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

    void gather_displacers() {
        m_displacers.clear();
        const WaterSettings& water = m_settings.water;

        if (m_world.fluid_rules().displaces()) m_entities.for_each([this, &water](const Entity& e) {
            const AABB box = e.bounding_box();
            const vector3d size = box.max - box.min;
            const double volume = size.x * size.y * size.z;
            double share = 1.0;
            if (water.displace_by == DisplaceBy::Weight && volume > 0.0) share = vmin(1.0, e.mass() / vmax(water.fluid_density, 1.0) / volume);
            m_displacers.push_back({ box, share });
        });
        m_world.displace(m_displacers);
    }

    void gather_lights() {
        m_frame_lights.clear();
        
        m_entities.for_each([this](const Entity& e) {
            DynamicLight l;
            if (e.emits_light(l, m_alpha)) m_frame_lights.push_back(l);
        });
        
        m_dynamic_lights.for_each([this](const DynamicLight& l) { m_frame_lights.push_back(l); });
    }

    void gather_occluders(bool show_player) {
        m_lighting.clear_occluders();

        m_entities.for_each([this, show_player](const Entity& e) {
            if (&e == m_player && !show_player) return;
            fizmo::graphics::CapsuleOccluder3D c;
            if (e.casts_capsule_shadow(c, m_alpha)) m_lighting.add_occluder(c);
        });
    }

    static AttributeRegistry make_attributes() {
        AttributeRegistry r;
        AttributeRegistry::register_defaults(r);
        return r;
    }

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