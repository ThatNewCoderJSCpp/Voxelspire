#ifndef VOXELSPIRE_GAME_CAMERA_HPP
#define VOXELSPIRE_GAME_CAMERA_HPP

#include <memory>
#include <vector>
#include "camera_rig.hpp"

namespace voxelspire {

class GameCamera {
public:
    static constexpr double CHUNK_DIAGONAL = 1.7320508075688772;

    GameCamera(const CameraSettings& settings, unsigned int viewport_w, unsigned int viewport_h, double view_distance)
        : m_settings(settings),
          m_camera({}, viewport_w, viewport_h, settings.fov_y, settings.near_plane, far_plane_for(view_distance)) {}

    void set_view_distance(double distance) noexcept { m_camera.set_clip_planes(m_settings.near_plane, far_plane_for(distance)); }

    void set_settings(const CameraSettings& settings, double view_distance) noexcept {
        m_settings = settings;
        m_camera.set_fov_y(settings.fov_y);
        set_view_distance(view_distance);
    }

    static double far_plane_for(double view_distance) noexcept {
        return view_distance + static_cast<double>(EngineLimits::CHUNK_SIZE) * CHUNK_DIAGONAL;
    }

    void add_rig(std::unique_ptr<CameraRig> rig) { m_rigs.push_back(std::move(rig)); }

    void cycle_rig() noexcept { if (!m_rigs.empty()) m_active = (m_active + 1) % m_rigs.size(); }
    void set_rig(std::size_t index) noexcept { if (index < m_rigs.size()) m_active = index; }

    const CameraRig* active_rig() const noexcept { return m_rigs.empty() ? nullptr : m_rigs[m_active].get(); }
    const char* mode_name() const noexcept { return active_rig() ? active_rig()->name() : "none"; }
    bool shows_player() const noexcept { return active_rig() && active_rig()->shows_player(); }

    void set_viewport(unsigned int w, unsigned int h) noexcept { m_camera.set_viewport_size(w, h); }

    void update(const Player& player, const World& world, double alpha, double frame_dt) {
        if (m_rigs.empty()) return;
        const vector3d before = m_camera.position();
        m_rigs[m_active]->apply(m_camera, CameraRigContext{ player, world, m_settings, alpha });
        keep_off_surface(world);
        const vector3d now = m_camera.position();
        if (m_has_previous && frame_dt > 0.0) m_velocity = (now - before) / frame_dt;
        m_has_previous = true;
    }

    fizmo::graphics::Camera3D&       camera()       noexcept { return m_camera; }
    const fizmo::graphics::Camera3D& camera() const noexcept { return m_camera; }
    const vector3d& position()                const noexcept { return m_camera.position(); }
    const vector3d& velocity()                const noexcept { return m_velocity; }

private:
    static constexpr double SURFACE_CLEARANCE = 2.5;

    void keep_off_surface(const World& world) {
        vector3d p = m_camera.position();
        const double margin = m_settings.near_plane * SURFACE_CLEARANCE;
        const BlockPos cell = BlockPos::containing(p);

        for (int dz = -1; dz <= 1; ++dz) {
            const BlockPos c{ cell.x, cell.y, cell.z + dz };
            const BlockId id = world.block_id_at(c);
            if (!world.blocks().traits(id).fluid || world.block_id_at({ c.x, c.y, c.z + 1 }) == id) continue;
            const double surface = c.z + world.fluid_height(c);
            if (std::fabs(p.z - surface) >= margin) continue;
            p.z = p.z >= surface ? surface + margin : surface - margin;
            m_camera.set_position(p);
            return;
        }
    }

    CameraSettings                          m_settings;
    fizmo::graphics::Camera3D               m_camera;
    std::vector<std::unique_ptr<CameraRig>> m_rigs;
    std::size_t                             m_active = 0;
    vector3d                                m_velocity{};
    bool                                    m_has_previous = false;
};

} // namespace voxelspire

#endif // VOXELSPIRE_GAME_CAMERA_HPP