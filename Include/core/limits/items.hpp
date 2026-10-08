#ifndef VOXELSPIRE_CORE_LIMITS_ITEMS_HPP
#define VOXELSPIRE_CORE_LIMITS_ITEMS_HPP

#include "bounds.hpp"

namespace voxelspire {

struct ItemLimits {
    static constexpr Bounds stack { 1.0, 999.0 };
};

struct InventoryLimits {
    static constexpr Bounds slot_size = Bounds::at_least(16.0, 96.0);
    static constexpr Bounds hotbar_bottom { 0.0, 400.0 };
};

} // namespace voxelspire

#endif // VOXELSPIRE_CORE_LIMITS_ITEMS_HPP