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
    static double distance_to_rect(double px, double py, double x0, double y0, double x1, double y1) noexcept;

    static double column_distance(const ColumnPos& c, double px, double py) noexcept;

    static LodSelection select(
        const vector3d& camera, double render_distance, double detail_distance,
        const LodSettings& lod, const LodLayout& layout, const World& world
    );

private:
    static void add_circle(LodSelection& out, const vector3d& camera, double R, const World& world);

    static void visit(
        LodSelection& out, 
        const LodTileKey& k, 
        const vector3d& camera, 
        double R, 
        double D,
        const LodLayout& layout, 
        const World& world
    );
};

} // namespace voxelspire

#endif // VOXELSPIRE_RENDER_LOD_SELECTION_HPP