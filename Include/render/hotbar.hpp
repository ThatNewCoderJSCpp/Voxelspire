#ifndef VOXELSPIRE_RENDER_HOTBAR_HUD_HPP
#define VOXELSPIRE_RENDER_HOTBAR_HUD_HPP

#include <cmath>
#include <string>
#include "../item/inventory.hpp"
#include "item_icon.hpp"

namespace voxelspire {

class HotbarHud {
public:
    static constexpr double GAP          = 4.0;
    static constexpr double ICON_INSET   = 0.1;
    static constexpr double COUNT_TEXT   = 0.32;
    static constexpr double COUNT_SHADOW = 1.0;
    static constexpr double LINE         = 1.2;
    static constexpr double OUTLINE      = 2.0;
    static constexpr double NUMBER_TEXT  = 0.24;

    void render(fizmo::windows::Renderer& r, unsigned int w, unsigned int h, const InventorySettings& s, double scale, const Inventory& inv, const ItemRegistry& items);

private:
    static constexpr int KEYS = 10;

    void number(fizmo::windows::Renderer& r, const InventorySettings& s, int x, int y, int slot, int i) const;

    static constexpr double HALF = 0.5;

    ItemIconPainter m_icons;
};

} // namespace voxelspire

#endif // VOXELSPIRE_RENDER_HOTBAR_HUD_HPP