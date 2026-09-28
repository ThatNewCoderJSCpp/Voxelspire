#ifndef VOXELSPIRE_GAME_HPP
#define VOXELSPIRE_GAME_HPP

#include <memory>
#include <optional>
#include <string>
#include "../block/blocks.hpp"
#include "../camera/game_camera.hpp"
#include "../core/settings.hpp"
#include "../entity/player.hpp"
#include "../input/input_bindings.hpp"
#include "../render/block_outline_renderer.hpp"
#include "../render/capsule_renderer.hpp"
#include "../world/chunk_mesh.hpp"
#include "../render/hud.hpp"
#include "../render/world_renderer.hpp"
#include "../world/world_generator.hpp"

namespace voxelspire {

class Game {
public:
    explicit Game(const GameSettings& settings = GameSettings{})
        : m_settings(settings),
          m_block_ids(DefaultBlocks::register_all(m_registry)),
          m_attributes(make_attributes()),
          m_movement_modes(make_movement_modes()),
          m_world(m_registry, settings.world),
          m_generator(std::make_unique<FlatWorldGenerator>(settings.flat_world)),
          m_physics(m_world.settings()),
          m_player(m_attributes, m_movement_modes),
          m_camera(settings.camera, 1, 1),
          m_bindings(InputBindings::defaults()),
          m_capsule(settings.render.capsule_segments, settings.render.capsule_rings,
                    settings.render.player_color, settings.render.player_visor) {
        m_camera.add_rig(std::make_unique<FirstPersonRig>());
        m_camera.add_rig(std::make_unique<ThirdPersonBackRig>());
        m_camera.add_rig(std::make_unique<ThirdPersonFrontRig>());
    }

    void start(fizmo::windows::Window& window, unsigned int viewport_w, unsigned int viewport_h) {
        m_cursor = std::make_unique<fizmo::windows::CursorLock>(window);
        m_generator->generate(m_world);
        m_meshes.update(m_world);
        m_player.respawn(m_generator->spawn_point(m_world));
        m_camera.set_viewport(viewport_w, viewport_h);
        m_viewport_w = viewport_w;
        m_viewport_h = viewport_h;
        m_camera.update(m_player, m_world, 1.0, 0.0);
    }

    void shutdown() noexcept { if (m_cursor) m_cursor->unlock(); }

    bool mouse_locked() const noexcept { return m_cursor && m_cursor->locked(); }

    void handle_event(const fizmo::windows::WindowEvent& e) {
        using fizmo::windows::WindowEventType;
        switch (e.type) {
            case WindowEventType::WindowResize:
                if (e.x > 0 && e.y > 0) { m_viewport_w = e.x; m_viewport_h = e.y; m_camera.set_viewport(e.x, e.y); }
                break;
            case WindowEventType::MouseClick:
                if (m_cursor && !m_cursor->locked()) m_cursor->lock();
                break;
            case WindowEventType::KeyPress:
                m_last_key = e.key_name;
                break;
            default: break;
        }
    }

    void update(double frame_dt, const fizmo::windows::InputManager& input) {
        if (mouse_locked()) {
            const double sens = m_settings.controls.mouse_sensitivity;
            const double invert = m_settings.controls.invert_y ? -1.0 : 1.0;
            m_player.add_look(-input.mouse_delta_x() * sens, -input.mouse_delta_y() * sens * invert);
        }

        if (m_bindings.just_pressed(input, Action::CycleCamera))  m_camera.cycle_rig();
        if (m_bindings.just_pressed(input, Action::ReleaseMouse) && m_cursor) m_cursor->unlock();
        if (m_bindings.just_pressed(input, Action::ToggleHud))    m_settings.render.show_debug_hud = !m_settings.render.show_debug_hud;
        if (m_bindings.just_pressed(input, Action::Respawn))      m_player.respawn(m_generator->spawn_point(m_world));
        MovementIntent intent;
        intent.forward = (m_bindings.is_down(input, Action::MoveForward) ? 1.0 : 0.0) - (m_bindings.is_down(input, Action::MoveBack) ? 1.0 : 0.0);
        intent.strafe  = (m_bindings.is_down(input, Action::MoveRight) ? 1.0 : 0.0) - (m_bindings.is_down(input, Action::MoveLeft) ? 1.0 : 0.0);
        if (m_bindings.just_pressed(input, Action::Jump)) m_jump_latch = true;
        intent.jump    = m_bindings.is_down(input, Action::Jump) || m_jump_latch;
        intent.sprint  = m_bindings.is_down(input, Action::Sprint);
        intent.crouch  = m_bindings.is_down(input, Action::Crouch);
        intent.crawl   = m_bindings.is_down(input, Action::Crawl);
        m_player.set_intent(intent);
        advance(frame_dt);
        m_meshes.update(m_world);
        m_target = raycast_blocks(m_world, m_player.eye_position(m_alpha), m_player.look_direction(), m_player.attributes().value(Attributes::BlockReach));
        m_camera.update(m_player, m_world, m_alpha, frame_dt);
    }

    void render(fizmo::windows::Renderer& renderer, double fps_average) {
        renderer.begin_3d(m_camera.camera());
        renderer.set_light_3d(fizmo::graphics::Light3D::sun({ -0.35, 0.55, -1.0 }));
        m_stats = m_world_renderer.render(renderer, m_meshes, m_camera.camera());
        if (m_camera.shows_player()) m_capsule.render(renderer, m_player, m_alpha);
        if (m_target) m_outline.render(renderer, m_world, *m_target, m_settings.render);
        renderer.end_3d();
        HudInfo info;
        info.fps_average    = fps_average;
        info.position       = m_player.interpolated_position(m_alpha);
        info.velocity       = m_player.velocity();
        info.on_ground      = m_player.on_ground();
        info.camera_mode    = m_camera.mode_name();
        info.stats          = m_stats;
        info.target         = m_target;
        info.target_name    = m_target ? m_world.block_at(m_target->block).name() : std::string();
        info.mouse_captured = mouse_locked();
        info.last_key       = m_last_key;
        info.movement_mode  = m_player.movement_mode() ? m_player.movement_mode()->id() : std::string("none");
        m_hud.render(renderer, m_viewport_w, m_viewport_h, info, m_settings.render);
    }

public:
    GameSettings&         settings()        noexcept { return m_settings; }
    World&                world()           noexcept { return m_world; }
    Player&               player()          noexcept { return m_player; }
    GameCamera&           camera()          noexcept { return m_camera; }
    BlockRegistry&        blocks()          noexcept { return m_registry; }
    AttributeRegistry&    attribute_types() noexcept { return m_attributes; }
    MovementModeRegistry& movement_modes()  noexcept { return m_movement_modes; }

    const DefaultBlocks& block_ids() const noexcept { return m_block_ids; }
    const WorldRenderStats& last_render_stats() const noexcept { return m_stats; }
    const std::optional<RaycastHit>& target() const noexcept { return m_target; }

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
    void tick_once(double dt) {
        TickContext ctx{ m_world, m_physics, dt };
        m_player.tick(ctx);
        if (m_jump_latch) { m_jump_latch = false; MovementIntent i = m_player.intent(); i.jump = false; m_player.set_intent(i); }
        const double void_z = m_world.settings().min_z - m_world.settings().void_depth;
        if (m_player.position().z < void_z) m_player.respawn(m_generator->spawn_point(m_world));
    }

    static AttributeRegistry make_attributes() {
        AttributeRegistry r;
        AttributeRegistry::register_defaults(r);
        return r;
    }

    static MovementModeRegistry make_movement_modes() {
        MovementModeRegistry r;
        MovementModeRegistry::register_defaults(r);
        return r;
    }

    GameSettings                    m_settings;
    BlockRegistry                   m_registry;
    DefaultBlocks                   m_block_ids;
    AttributeRegistry               m_attributes;
    MovementModeRegistry            m_movement_modes;
    World                           m_world;
    std::unique_ptr<WorldGenerator> m_generator;
    ChunkMeshCache                  m_meshes;
    EntityPhysics                   m_physics;
    Player                          m_player;
    GameCamera                      m_camera;
    InputBindings                   m_bindings;
    std::unique_ptr<fizmo::windows::CursorLock> m_cursor;

    WorldRenderer                   m_world_renderer;
    CapsuleRenderer                 m_capsule;
    BlockOutlineRenderer            m_outline;
    Hud                             m_hud;
    WorldRenderStats                m_stats;

    std::optional<RaycastHit>       m_target;
    std::string                     m_last_key;
    bool                            m_jump_latch = false;
    double                          m_accumulator = 0.0;
    double                          m_alpha = 1.0;
    unsigned int                    m_viewport_w = 1, m_viewport_h = 1;
};

} // namespace voxelspire

#endif // VOXELSPIRE_GAME_HPP