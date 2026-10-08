#ifndef VOXELSPIRE_CORE_SETTINGS_ITEMS_HPP
#define VOXELSPIRE_CORE_SETTINGS_ITEMS_HPP

#include <cstdint>
#include "../types.hpp"

namespace voxelspire {

enum class WhileInventoryOpen : std::uint8_t { KeepRunning = 0, PauseWorld, PauseAndHide };

struct ItemDefaults {
    static constexpr int    stack  = 64;
    static constexpr double weight = 0.5;
};

struct ItemRules {
    int  default_stack  = ItemDefaults::stack;
    bool catalog_gives  = false;
};

struct InventorySettings {
    WhileInventoryOpen while_open      = WhileInventoryOpen::KeepRunning;
    bool               show_hotbar     = true;
    bool               show_counts     = true;
    bool               show_numbers    = true;
    double             slot_size       = 40.0;
    double             hotbar_bottom   = 8.0;
    bool               scroll_wraps    = true;
    bool               invert_scroll   = false;
    Color              slot_color      { 38, 43, 57, 235 };
    Color              slot_hover      { 64, 72, 94, 245 };
    Color              selected_color  { 255, 255, 255, 235 };
    Color              backdrop        { 0, 0, 0, 140 };
    Color              hidden_world    { 14, 16, 22, 255 };
    Color              count_color     { 255, 255, 255 };
    Color              count_shadow    { 0, 0, 0, 220 };
    Color              number_color    { 255, 255, 255, 120 };
};

} // namespace voxelspire

#endif // VOXELSPIRE_CORE_SETTINGS_ITEMS_HPP