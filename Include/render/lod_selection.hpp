#ifndef VOXELSPIRE_RENDER_LOD_SELECTION_HPP
#define VOXELSPIRE_RENDER_LOD_SELECTION_HPP

#include <cmath>
#include <unordered_set>
#include <vector>
#include "lod.hpp"

namespace voxelspire {

struct LodSelection {
    std::vector<LodTileKey> tiles;
    std::vector<ColumnPos>  detail_columns;
    std::unordered_set<ColumnPos, ColumnPosHash> detail_set;
    ColumnPos center;
};

class LodSelector {
public:
    static double distance_to_rect(double px, double py, double x0, double y0, double x1, double y1) noexcept {
        const double dx = vmax(vmax(x0 - px, 0.0), px - x1);
        const double dy = vmax(vmax(y0 - py, 0.0), py - y1);
        return std::sqrt(dx * dx + dy * dy);
    }

    static double column_distance(const ColumnPos& c, double px, double py) noexcept {
        const double x0 = c.x * double(Chunk::SIZE), y0 = c.y * double(Chunk::SIZE);
        return distance_to_rect(px, py, x0, y0, x0 + Chunk::SIZE, y0 + Chunk::SIZE);
    }

    static LodSelection select(
        const vector3d& camera, double render_distance, double detail_distance,
        const LodSettings& lod, const LodLayout& layout, const World& world
    ) {
        LodSelection out;
        out.center = World::column_of(camera);

        if (!lod.enabled) {
            add_circle(out, camera, render_distance, world);
            return out;
        }

        const double R = render_distance;
        int top = 1;
        while (top < lod.max_level && layout.tile_blocks(top) < R) ++top;
        const int tb = layout.tile_blocks(top);
        const int tx0 = floor_div(static_cast<int>(std::floor(camera.x - R)), tb), tx1 = floor_div(static_cast<int>(std::floor(camera.x + R)), tb);
        const int ty0 = floor_div(static_cast<int>(std::floor(camera.y - R)), tb), ty1 = floor_div(static_cast<int>(std::floor(camera.y + R)), tb);

        for (int ty = ty0; ty <= ty1; ++ty)
            for (int tx = tx0; tx <= tx1; ++tx)
                visit(out, { top, tx, ty }, camera, R, detail_distance, layout, world);

        return out;
    }

private:
    static void add_circle(LodSelection& out, const vector3d& camera, double R, const World& world) {
        const int r = static_cast<int>(std::ceil(R / Chunk::SIZE)) + 1;

        for (int dy = -r; dy <= r; ++dy)
            for (int dx = -r; dx <= r; ++dx) {
                const ColumnPos c{ out.center.x + dx, out.center.y + dy };
                if (column_distance(c, camera.x, camera.y) >= R || !world.column_in_bounds(c)) continue;
                out.detail_columns.push_back(c);
                out.detail_set.insert(c);
            }
    }

    static void visit(
        LodSelection& out, 
        const LodTileKey& k, 
        const vector3d& camera, 
        double R, 
        double D,
        const LodLayout& layout, 
        const World& world
    ) {
        const BlockPos o = layout.origin(k);
        const double size = layout.tile_blocks(k.level);
        const double dist = distance_to_rect(camera.x, camera.y, o.x, o.y, o.x + size, o.y + size);
        if (dist >= R) return;
        const int limit = world.settings().horizontal_limit;
        if (o.x >= limit || o.x + size <= -limit || o.y >= limit || o.y + size <= -limit) return;

        if (k.level == 0) {
            const ColumnPos first = layout.first_column(k);
            const int n = layout.columns_per_side(0);

            for (int dy = 0; dy < n; ++dy)
                for (int dx = 0; dx < n; ++dx) {
                    const ColumnPos c{ first.x + dx, first.y + dy };
                    if (column_distance(c, camera.x, camera.y) >= R || !world.column_in_bounds(c)) continue;
                    out.detail_columns.push_back(c);
                    out.detail_set.insert(c);
                }
            return;
        }

        if (dist < D * static_cast<double>(1 << (k.level - 1))) {
            for (int cy = 0; cy < 2; ++cy)
                for (int cx = 0; cx < 2; ++cx)
                    visit(out, { k.level - 1, k.x * 2 + cx, k.y * 2 + cy }, camera, R, D, layout, world);
            return;
        }

        out.tiles.push_back(k);
    }
};

} // namespace voxelspire

#endif // VOXELSPIRE_RENDER_LOD_SELECTION_HPP