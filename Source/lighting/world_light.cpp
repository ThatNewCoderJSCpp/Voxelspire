#include "lighting/world_light.hpp"

namespace voxelspire {

std::unique_ptr<LightEngine> make_light_engine(const World& world, LightFormat format) {
    switch (format) {
        case LightFormat::Plain:   return std::make_unique<WorldLight<PlainLight>>(world);
        case LightFormat::Colored: return std::make_unique<WorldLight<ColoredLight>>(world);
        case LightFormat::None:    break;
    }

    return nullptr;
}

} // namespace voxelspire
