#ifndef VOXELSPIRE_RENDER_PROJECTOR_HPP
#define VOXELSPIRE_RENDER_PROJECTOR_HPP

#include <array>
#include "../core/types.hpp"

namespace voxelspire {

struct ScreenPoint { double x = 0.0, y = 0.0; };

class Projector {
public:
    static constexpr int MAX_POINTS = 8; 

    explicit Projector(const fizmo::graphics::Camera3D& cam)
        : m_view(cam.view_matrix().data), m_proj(cam.projection_matrix().data),
          m_w(double(cam.viewport_width())), m_h(double(cam.viewport_height())),
          m_near(cam.near_plane() * 1.001), m_cam_pos(cam.position()), m_forward(cam.forward()),
          m_perspective(cam.projection_type() == fizmo::graphics::Projection3D::Perspective) {}

    const vector3d& camera_position() const noexcept { return m_cam_pos; }
    const vector3d& camera_forward()  const noexcept { return m_forward; }
    bool perspective() const noexcept { return m_perspective; }

    vector3d to_view(const vector3d& p) const noexcept {
        const auto& m = m_view;
        return { m[0] * p.x + m[1] * p.y + m[2]  * p.z + m[3],
                 m[4] * p.x + m[5] * p.y + m[6]  * p.z + m[7],
                 m[8] * p.x + m[9] * p.y + m[10] * p.z + m[11] };
    }

    ScreenPoint view_to_screen(const vector3d& v) const noexcept {
        const auto& m = m_proj;
        const double cx = m[0]  * v.x + m[1]  * v.y + m[2]  * v.z + m[3];
        const double cy = m[4]  * v.x + m[5]  * v.y + m[6]  * v.z + m[7];
        const double cw = m[12] * v.x + m[13] * v.y + m[14] * v.z + m[15];
        const double inv_w = 1.0 / cw;
        const double LIMIT = 1.0e5;
        return { vclamp((cx * inv_w * 0.5 + 0.5) * m_w, -LIMIT, LIMIT),
                 vclamp((0.5 - cy * inv_w * 0.5) * m_h, -LIMIT, LIMIT) };
    }

    int project_polygon(const vector3d* world, int n, ScreenPoint* out) const noexcept {
        std::array<vector3d, MAX_POINTS> in{}, clipped{};
        bool all_front = true, all_behind = true;

        for (int i = 0; i < n; ++i) {
            in[i] = to_view(world[i]);
            if (in[i].z <= -m_near) all_behind = false; else all_front = false;
        }

        if (all_behind) return 0;

        const vector3d* src = in.data();
        int count = n;

        if (!all_front) { 
            count = 0;

            for (int i = 0; i < n; ++i) {
                const vector3d& a = in[i];
                const vector3d& b = in[(i + 1) % n];
                const bool a_in = a.z <= -m_near, b_in = b.z <= -m_near;
                if (a_in) clipped[count++] = a;

                if (a_in != b_in && count < MAX_POINTS) {
                    const double t = (-m_near - a.z) / (b.z - a.z);
                    clipped[count++] = a + (b - a) * t;
                }
            }

            src = clipped.data();
        }

        for (int i = 0; i < count; ++i) out[i] = view_to_screen(src[i]);
        return count;
    }

    bool project_segment(const vector3d& a_world, const vector3d& b_world, ScreenPoint& a_out, ScreenPoint& b_out) const noexcept {
        vector3d a = to_view(a_world), b = to_view(b_world);
        const bool a_in = a.z <= -m_near, b_in = b.z <= -m_near;
        if (!a_in && !b_in) return false;
        if (!a_in) a = a + (b - a) * ((-m_near - a.z) / (b.z - a.z));
        if (!b_in) b = b + (a - b) * ((-m_near - b.z) / (a.z - b.z));
        a_out = view_to_screen(a);
        b_out = view_to_screen(b);
        return true;
    }

    bool faces_camera(const vector3d& normal, const vector3d& point_on_face) const noexcept {
        if (m_perspective) return normal.dot(m_cam_pos - point_on_face) > 0.0;
        return normal.dot(m_forward) < 0.0;
    }

private:
    std::array<double, 16> m_view;
    std::array<double, 16> m_proj;
    double   m_w, m_h, m_near;
    vector3d m_cam_pos, m_forward;
    bool     m_perspective;
};

} // namespace voxelspire

#endif // VOXELSPIRE_RENDER_PROJECTOR_HPP
