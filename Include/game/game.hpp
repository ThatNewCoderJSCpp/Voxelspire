#ifndef VOXELSPIRE_GAME_HPP
#define VOXELSPIRE_GAME_HPP

#include <algorithm>
#include <memory>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>
#include "../block/blocks.hpp"
#include "../camera/game_camera.hpp"
#include "../core/settings.hpp"
#include "../entity/entity_manager.hpp"
#include "../entity/player.hpp"
#include "../input/input_bindings.hpp"
#include "../lighting/director.hpp"
#include "../lighting/presets.hpp"
#include "../lighting/world_light.hpp"
#include "../physics/water_presets.hpp"
#include "../particles/system.hpp"
#include "../render/block_outline_renderer.hpp"
#include "../render/capsule_renderer.hpp"
#include "../render/hud.hpp"
#include "../render/reflection_planes.hpp"
#include "../render/world_renderer.hpp"
#include "../sky/renderer.hpp"
#include "../ui/settings_file.hpp"
#include "../ui/settings_menu.hpp"
#include "../ui/settings_pages.hpp"
#include "../ui/settings_registry.hpp"
#include "../world/chunk_streamer.hpp"
#include "../world/world_generator.hpp"
#include "../world/fluid_simulator.hpp"

namespace voxelspire {

class Game {
public:
    explicit Game(const GameSettings& settings = GameSettings{})
        : m_settings(with_seed(settings)),
          m_default_settings(m_settings),
          m_block_ids(DefaultBlocks::register_all(m_registry, settings.render.block_color_variation)),
          m_attributes(make_attributes()),
          m_movement_modes(make_movement_modes(settings.character)),
          m_particle_types(make_particle_types()),
          m_world(m_registry, m_settings.world),
          m_generator(std::make_unique<FlatWorldGenerator>(m_settings.flat_world, m_registry, m_settings.world.seed)),
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
        m_lighting.configure(m_settings.lighting);
        m_celestial.configure(m_settings.celestial);
        m_terrain.configure(m_settings.render, m_settings.streaming, m_settings.lod, m_settings.lighting);
        m_camera.add_rig(std::make_unique<FirstPersonRig>());
        m_camera.add_rig(std::make_unique<ThirdPersonBackRig>());
        m_camera.add_rig(std::make_unique<ThirdPersonFrontRig>());
        apply_character();
        apply_physics();
        SettingsPages::register_all(m_settings_registry, make_hooks(), m_presets, m_water_presets);
    }

    ~Game() { m_jobs.shutdown(); }

    Game(const Game&) = delete;
    Game& operator=(const Game&) = delete;

    void start(fizmo::windows::Window& window, unsigned int viewport_w, unsigned int viewport_h) {
        m_registry.lock();
        m_cursor = std::make_unique<fizmo::windows::CursorLock>(window);
        const ColumnPos spawn = m_generator->spawn_column();
        m_streamer.load_now(columns_around(spawn, simulation_distance()));
        m_world.update_lighting(std::numeric_limits<double>::max());
        respawn_player();
        update_streaming(true);
        m_camera.set_viewport(viewport_w, viewport_h);
        m_viewport_w = viewport_w;
        m_viewport_h = viewport_h;
        m_camera.update(*m_player, m_world, 1.0, 0.0);
        rebuild_menu();
    }

    void shutdown() noexcept { if (m_cursor) m_cursor->unlock(); }

    bool mouse_locked() const noexcept { return m_cursor && m_cursor->locked(); }

    void handle_event(const fizmo::windows::WindowEvent& e) {
        using fizmo::windows::WindowEventType;
        if (m_menu.on_event(e)) { if (e.type == WindowEventType::KeyPress) m_key_consumed = true; return; }

        switch (e.type) {
            case WindowEventType::WindowResize:
                if (e.x > 0 && e.y > 0) { m_viewport_w = e.x; m_viewport_h = e.y; m_camera.set_viewport(e.x, e.y); }
                break;
            case WindowEventType::MouseClick:
                if (m_cursor && !m_cursor->locked() && !m_menu.is_open()) lock_cursor();
                break;
            case WindowEventType::KeyPress:
                m_last_key = e.key_name;
                break;
            default: break;
        }
    }

    void update(double frame_dt, const fizmo::windows::InputManager& input) {
        m_frame_dt = frame_dt;
        m_seconds += frame_dt;
        const InputBindings& keys = m_settings.bindings;
        if (keys.just_pressed(input, Action::OpenMenu) && !m_key_consumed) toggle_menu();
        m_key_consumed = false;

        if (m_menu.is_open()) {
            m_menu.update(input, frame_dt);
            apply_settings(m_menu.take_changes());
            if (m_menu.take_close_request()) close_menu();
        }

        const bool playing = !m_menu.is_open();
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
        if (playing || !m_settings.menu.pause_game) advance(frame_dt * vmax(m_settings.simulation.game_speed, 0.0));
        ensure_player_terrain();
        update_streaming(false);
        m_streamer.update(m_settings.streaming.max_column_jobs);
        m_jobs.run_completions(m_settings.streaming.result_time_budget_ms);
        m_world.update_lighting(m_settings.lighting.update_budget_ms);
        m_terrain.update(m_player->eye_position(m_alpha));
        m_particles.update(frame_dt, m_camera.camera().position(), m_world, m_world.settings().gravity);
        m_target = raycast_blocks(m_world, m_player->eye_position(m_alpha), m_player->look_direction(), m_player->attributes().value(Attributes::BlockReach));
        m_camera.update(*m_player, m_world, m_alpha, frame_dt);
    }

    void render(fizmo::windows::Renderer& renderer, double fps_average) {
        m_terrain.upload(renderer);
        const vector3d eye = m_camera.camera().position();
        m_sky = m_sun_path->evaluate(m_clock.time());
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
        renderer.set_scene_lighting(m_lighting.build(m_sky, m_world, eye, m_frame_lights, m_seconds, submerged ? &medium : nullptr));
        renderer.begin_3d(m_camera.camera());
        renderer.set_light_3d(fizmo::graphics::Light3D::sun(LEGACY_SUN));
        m_celestial.render(renderer, celestial_view(eye, submerged));
        WorldRenderOptions options;
        options.render_distance = m_settings.render.render_distance_blocks();
        options.cave_culling    = m_settings.render.cave_culling;
        options.face_culling    = m_settings.render.face_culling;
        options.shadow_distance = m_lighting.shadows() ? m_lighting.shadow_distance() : 0.0;
        options.reflections     = m_reflections.planes();
        options.reflection_distance = vmin(static_cast<double>(m_settings.lighting.reflection_view_chunks) * Chunk::SIZE + m_reflections.farthest(), options.render_distance);
        m_stats = m_world_renderer.render(renderer, m_terrain, m_camera.camera(), options);
        m_particles.render(renderer, eye, [this](const BlockPos& b) { return m_lighting.render_light_at(m_world, b.center()); });

        if (show_body) {
            const auto view = third_person ? fizmo::graphics::View3D::Everywhere : fizmo::graphics::View3D::ReflectionsOnly;
            m_capsule.render(renderer, *m_player, m_alpha, m_lighting.render_light_at(m_world, m_player->interpolated_position(m_alpha) + vector3d{ 0.0, 0.0, m_player->height() * 0.5 }), view, !m_lighting.capsule_shadows());
        }

        if (m_target && !m_menu.is_open()) m_outline.render(renderer, m_world, *m_target, m_settings.render);
        renderer.end_3d();

        if (m_menu.is_open()) {
            m_menu.render(renderer, m_viewport_w, m_viewport_h, m_settings.menu);
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
        if (m_menu.is_open()) return;
        m_menu.open();
        if (m_cursor) m_cursor->unlock();
    }

    void close_menu() {
        if (!m_menu.is_open()) return;
        m_menu.close();
        if (m_settings.menu.save_on_close) save_settings();
        lock_cursor();
    }

    void lock_cursor() {
        if (!m_cursor) return;
        m_cursor->lock();
        m_ignore_look = LOOK_SETTLE_FRAMES;
    }

    void toggle_menu() { if (m_menu.is_open()) close_menu(); else open_menu(); }
    bool menu_open() const noexcept { return m_menu.is_open(); }
    SettingsMenu& menu() noexcept { return m_menu; }

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
        if (groups & Apply::Streaming)   set_render_distance(m_settings.render.render_distance);
        if (groups & Apply::Particles)   m_particles.settings() = m_settings.particles;
        if (groups & Apply::Entities)    m_entities.settings() = m_settings.entities;
        if (groups & Apply::Player)      m_capsule = make_capsule(m_settings.render);

        if ((groups & Apply::HandLight) && m_player->has_hand_light()) {
            const DynamicLight light = m_settings.hand_light;
            m_player->set_hand_light(&light);
        }
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
        info.target_name    = m_target ? m_world.block_at(m_target->block).name() : std::string();
        info.mouse_captured = mouse_locked();
        info.last_key       = m_last_key;
        info.movement_mode  = m_player->movement_mode() ? m_player->movement_mode()->id().str() : std::string("none");
        info.entity_count    = m_entities.size();
        info.entity_contacts = m_entities.last_contact_count();
        info.air_model         = m_world.settings().air_resistance->id();
        info.terminal_velocity = m_physics.terminal_velocity(*m_player, m_player->effective_gravity_scale(), m_player->effective_drag_scale());
        info.streaming         = m_streamer.stats();
        info.simulation_distance = m_settings.streaming.simulation_distance;
        info.detail_distance     = m_settings.streaming.detail_distance;
        info.render_distance     = m_settings.render.render_distance;
        info.daylight            = m_sky.daylight;
        info.worker_threads      = m_jobs.worker_count();
        info.particles           = m_particles.count();
        info.particle_emitters   = m_particles.emitter_count();
        info.seed                = m_settings.world.seed;
        info.light               = m_world.lighting() ? m_world.lighting()->stats() : LightStats{};
        info.lighting_preset     = m_settings.lighting.name;
        info.time_hours          = m_clock.hours(m_settings.day_cycle);
        info.minutes_per_hour    = m_settings.day_cycle.minutes_per_hour;
        info.seconds_per_minute  = m_settings.day_cycle.seconds_per_minute;
        info.day                 = m_clock.day();
        info.day_cycle           = m_settings.day_cycle.enabled;
        info.light_here          = light_level_at(m_player->eye_position(m_alpha));
        info.dynamic_lights      = m_lighting.scene().point_lights.size();
        info.shadow_casters      = m_stats.shadow_casters;
        info.water_preset        = m_settings.water.name;
        info.fluid_model         = m_world.settings().fluid_resistance->id();
        info.flow_model          = m_world.fluid_rules().id();
        info.fluids              = m_fluids.stats();
        info.reflection_planes   = m_reflections.planes().size();
        info.gpu                 = average_gpu();
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

    void toggle_hand_light() {
        const DynamicLight torch = m_settings.hand_light;
        m_player->set_hand_light(m_player->has_hand_light() ? nullptr : &torch);
    }

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

        const double hour = 1.0 / vmax(m_settings.day_cycle.hours_per_day, 1.0);
        if (keys.just_pressed(input, Action::CycleCamera))        m_camera.cycle_rig();
        if (keys.just_pressed(input, Action::ToggleHud))          m_settings.hud.show_debug = !m_settings.hud.show_debug;
        if (keys.just_pressed(input, Action::CycleHudCorner))     m_settings.hud.corner = Hud::next_corner(m_settings.hud.corner);
        if (keys.just_pressed(input, Action::HudLarger))          m_settings.hud.scale = Hud::stepped_scale(m_settings.hud.scale, 1);
        if (keys.just_pressed(input, Action::HudSmaller))         m_settings.hud.scale = Hud::stepped_scale(m_settings.hud.scale, -1);
        if (keys.just_pressed(input, Action::Respawn))            respawn_player();
        if (keys.just_pressed(input, Action::RenderDistanceUp))   set_render_distance(m_settings.render.render_distance + m_settings.render.render_distance_step);
        if (keys.just_pressed(input, Action::RenderDistanceDown)) set_render_distance(m_settings.render.render_distance - m_settings.render.render_distance_step);
        if (keys.just_pressed(input, Action::CycleLighting))      cycle_lighting();
        if (keys.just_pressed(input, Action::ToggleHandLight))    toggle_hand_light();
        if (keys.just_pressed(input, Action::TimeForward))        m_clock.add(hour);
        if (keys.just_pressed(input, Action::TimeBackward))       m_clock.add(-hour);
        if (keys.just_pressed(input, Action::ToggleDayCycle))     m_settings.day_cycle.enabled = !m_settings.day_cycle.enabled;
    }

    SettingsHooks make_hooks() {
        SettingsHooks hooks;
        hooks.hour     = [this] { return m_clock.hours(m_settings.day_cycle.hours_per_day); };
        hooks.set_hour = [this](double h) { m_clock.set_time(h / vmax(m_settings.day_cycle.hours_per_day, 1.0)); };
        hooks.save     = [this] { save_settings(); };
        hooks.reload   = [this] { load_settings(); };
        hooks.respawn  = [this] { respawn_player(); };
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
        w.fluid_resistance   = water.drag();
        w.fluid_buoyancy     = water.buoyancy;
        w.fluid_sink_speed   = water.sink_speed;
        w.current_speed      = water.current_speed;
        w.current_push       = water.current_push;
        w.wade_slowdown      = water.wade_slowdown;
        w.fall_break_depth   = water.fall_break_depth;
        w.fluid_updates      = water.updates;
        const bool new_rules = !m_flow_applied || !water.same_flow(m_applied_water);
        if (new_rules) w.fluid_rules = water.rules();
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

    void respawn_player() {
        m_streamer.load_now(columns_around(m_generator->spawn_column(), 1));
        m_player->respawn(m_generator->spawn_point(m_world));
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

    void tick_once(double dt) {
        m_simulation.center = m_player->position();
        m_simulation.radius = static_cast<double>(simulation_distance()) * Chunk::SIZE;
        m_simulation.world  = &m_world;
        TickContext ctx{ m_world, m_physics, m_entities, dt, &m_simulation };
        gather_displacers();
        m_fluids.update(m_world, dt, m_simulation.center, m_simulation.radius);
        m_entities.tick(ctx);
        m_clock.advance(dt, m_settings.day_cycle);
        if (m_jump_latch) { m_jump_latch = false; MovementIntent i = m_player->intent(); i.jump = false; m_player->set_intent(i); }
        const double void_z = m_world.settings().min_z - m_world.settings().void_depth;
        if (m_player->position().z < void_z) respawn_player();
    }

    inline static const vector3d LEGACY_SUN{ -0.35, 0.55, -1.0 };
    static constexpr int LOOK_SETTLE_FRAMES = 2;

    static Color opaque(const Color& c) noexcept { return Color(c.red(), c.green(), c.blue()); }

    void gather_displacers() {
        m_displacers.clear();
        if (m_world.fluid_rules().displaces()) m_entities.for_each([this](const Entity& e) { m_displacers.push_back(e.bounding_box()); });
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
    std::vector<AABB>               m_displacers;
    bool                            m_display_dirty = false;
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