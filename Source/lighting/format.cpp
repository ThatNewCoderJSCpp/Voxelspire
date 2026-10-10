#include "lighting/format.hpp"

namespace voxelspire {

void LightLevel::raise_block(int r, int g, int b) noexcept {
    red   = static_cast<std::uint8_t>(vmax(static_cast<int>(red), vclamp(r, 0, LightLimits::MAX)));
    green = static_cast<std::uint8_t>(vmax(static_cast<int>(green), vclamp(g, 0, LightLimits::MAX)));
    blue  = static_cast<std::uint8_t>(vmax(static_cast<int>(blue), vclamp(b, 0, LightLimits::MAX)));
}

fizmo::graphics::BakedLight LightLevel::baked() const noexcept {
    auto b = [](int v) { return static_cast<std::uint8_t>(v * BYTE_SCALE); };
    return fizmo::graphics::BakedLight(b(red), b(green), b(blue), b(sky));
}

} // namespace voxelspire
