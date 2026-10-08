#ifndef VOXELSPIRE_CORE_SETTINGS_CAMERA_HPP
#define VOXELSPIRE_CORE_SETTINGS_CAMERA_HPP

#include <cstdint>

namespace voxelspire {

struct CameraSettings {
    double fov_y                 = 70.0;
    double near_plane            = 0.05;
    double third_person_distance = 4.0;
    double collision_margin      = 0.2;
};

enum class RenderResolution : std::uint8_t { Scale = 0, P720, P1080, P1440, P2160, P4320 };

enum class UpscaleMode : std::uint8_t { Sharp = 0, Smooth, Blocky };

struct DisplaySettings {
    static constexpr double FULL_SCALE = 1.0;

    bool             vsync        = false;
    int              max_fps      = 0;
    RenderResolution resolution   = RenderResolution::Scale;
    double           render_scale = FULL_SCALE;
    UpscaleMode      upscale      = UpscaleMode::Sharp;
    double           sharpness    = 0.5;

    static double lines(RenderResolution r) noexcept {
        switch (r) {
            case RenderResolution::P720:  return HD;
            case RenderResolution::P1080: return FULL_HD;
            case RenderResolution::P1440: return QUAD_HD;
            case RenderResolution::P2160: return ULTRA_HD;
            case RenderResolution::P4320: return EIGHT_K;
            case RenderResolution::Scale: break;
        }

        return 0.0;
    }

    double scale_for(unsigned int window_height) const noexcept {
        double s = render_scale;
        if (resolution != RenderResolution::Scale && window_height > 0) s = lines(resolution) / static_cast<double>(window_height);
        return s < FULL_SCALE ? s : FULL_SCALE;
    }

private:
    static constexpr double HD       = 720.0;
    static constexpr double FULL_HD  = 1080.0;
    static constexpr double QUAD_HD  = 1440.0;
    static constexpr double ULTRA_HD = 2160.0;
    static constexpr double EIGHT_K  = 4320.0;
};

struct ControlSettings {
    double mouse_sensitivity = 0.12;
    bool   invert_y          = false;
};

} // namespace voxelspire

#endif // VOXELSPIRE_CORE_SETTINGS_CAMERA_HPP