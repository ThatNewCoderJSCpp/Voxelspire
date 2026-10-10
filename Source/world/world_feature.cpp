#include "world/world_feature.hpp"

namespace voxelspire {

void FeatureWriter::fill(const BlockPos& lo, const BlockPos& hi, BlockId block) {
    for (int x = lo.x; x < hi.x; ++x)
        for (int y = lo.y; y < hi.y; ++y)
            for (int z = lo.z; z < hi.z; ++z) set({ x, y, z }, block);
}

void BlockBox::generate(FeatureWriter& out, const FeatureArea& area) const {
    const BlockPos lo{ vmax(m_min.x, area.min_x), vmax(m_min.y, area.min_y), m_min.z };
    const BlockPos hi{ vmin(m_max.x, area.max_x), vmin(m_max.y, area.max_y), m_max.z };
    if (lo.x >= hi.x || lo.y >= hi.y || lo.z >= hi.z) return;
    out.fill(lo, hi, out.resolve(m_block));
}

Structure::Structure(Identifier id, int anchor_x, int anchor_y, std::vector<StructurePart> parts) : m_id(id), m_x(anchor_x), m_y(anchor_y), m_parts(std::move(parts)) {
    for (const StructurePart& p : m_parts) {
        m_low  = vmin(m_low, p.min.z);
        m_high = vmax(m_high, p.max.z - 1);
    }
}

void Structure::generate(FeatureWriter& out, const FeatureArea& area) const {
    for (const StructurePart& p : m_parts) {
        const BlockPos lo{ vmax(m_x + p.min.x, area.min_x), vmax(m_y + p.min.y, area.min_y), area.surface_z + p.min.z };
        const BlockPos hi{ vmin(m_x + p.max.x, area.max_x), vmin(m_y + p.max.y, area.max_y), area.surface_z + p.max.z };
        if (lo.x >= hi.x || lo.y >= hi.y || lo.z >= hi.z) continue;
        out.fill(lo, hi, out.resolve(p.block));
    }
}

} // namespace voxelspire
