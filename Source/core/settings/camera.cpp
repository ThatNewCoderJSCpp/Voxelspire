#include "core/settings/camera.hpp"

namespace voxelspire {

double DisplaySettings::lines(RenderResolution r) noexcept {
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

double DisplaySettings::scale_for(unsigned int window_height) const noexcept {
    double s = render_scale;
    if (resolution != RenderResolution::Scale && window_height > 0) s = lines(resolution) / static_cast<double>(window_height);
    return s < FULL_SCALE ? s : FULL_SCALE;
}

} // namespace voxelspire
