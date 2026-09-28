#ifndef VOXELSPIRE_GAME_CAMERA_HPP
#define VOXELSPIRE_GAME_CAMERA_HPP

#include <memory>
#include <vector>
#include "camera_rig.hpp"

namespace voxelspire {

class GameCamera {
public:
    GameCamera(const CameraSettings& settings, unsigned int viewport_w, unsigned int viewport_h)
        : m_settings(settings),
          m_camera({}, viewport_w, viewport_h, settings.fov_y, settings.near_plane, settings.far_plane) {}

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

        const vector3d now = m_camera.position();
        if (m_has_previous && frame_dt > 0.0) m_velocity = (now - before) / frame_dt;
        m_has_previous = true;
    }

    fizmo::graphics::Camera3D&       camera()       noexcept { return m_camera; }
    const fizmo::graphics::Camera3D& camera() const noexcept { return m_camera; }
    const vector3d& position() const noexcept { return m_camera.position(); }
    const vector3d& velocity() const noexcept { return m_velocity; }

private:
    CameraSettings                          m_settings;
    fizmo::graphics::Camera3D               m_camera;
    std::vector<std::unique_ptr<CameraRig>> m_rigs;
    std::size_t                             m_active = 0;
    vector3d                                m_velocity{};
    bool                                    m_has_previous = false;
};

} // namespace voxelspire

#endif // VOXELSPIRE_GAME_CAMERA_HPP