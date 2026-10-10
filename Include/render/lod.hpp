#ifndef VOXELSPIRE_RENDER_LOD_HPP
#define VOXELSPIRE_RENDER_LOD_HPP

#include <algorithm>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "../world/world_generator.hpp"
#include "greedy_mesher.hpp"

namespace voxelspire {

struct LodTileKey {
    int level = 0, x = 0, y = 0;
    constexpr bool operator==(const LodTileKey& o) const noexcept { return level == o.level && x == o.x && y == o.y; }
    constexpr bool operator!=(const LodTileKey& o) const noexcept { return !(*this == o); }
};

struct LodTileKeyHash {
    std::size_t operator()(const LodTileKey& k) const noexcept;
};

struct LodLayout {
    int cells = 32;

    int cell_size(int level)   const noexcept { return 1 << level; }
    int tile_blocks(int level) const noexcept { return cells << level; }

    BlockPos origin(const LodTileKey& k) const noexcept { return { k.x * tile_blocks(k.level), k.y * tile_blocks(k.level), 0 }; }

    ColumnPos first_column(const LodTileKey& k) const noexcept {
        const BlockPos o = origin(k);
        return { floor_div(o.x, Chunk::SIZE), floor_div(o.y, Chunk::SIZE) };
    }

    int columns_per_side(int level) const noexcept { return vmax(tile_blocks(level) / Chunk::SIZE, 1); }

    bool contains_column(const LodTileKey& k, const ColumnPos& c) const noexcept;

    LodTileKey parent_at(const ColumnPos& c, int level) const noexcept;
};

struct LodRun {
    std::int32_t  z0 = 0, z1 = 0;
    BlockId       id = AIR_ID;
    std::uint32_t count = 0;
};

struct LodBuildRequest {
    LodTileKey  key;
    LodLayout   layout;
    bool        heightmap = false;
    double      coverage  = 0.5;
    bool        merge     = true;
    bool        cull_void = true;
    int         samples   = 0;
    FaceShading shading;
    int         min_chunk_z = 0, max_chunk_z = 0;
    std::uint64_t epoch = 0;
    std::unordered_map<ChunkPos, std::shared_ptr<const Chunk>, ChunkPosHash> overrides;
};

struct LodTileMesh {
    LodTileKey                  key;
    std::uint64_t               epoch = 0;
    fizmo::graphics::QuadMesh3D quads;
    std::size_t                 faces = 0;
    int                         min_z = 0, max_z = 0;
};

class LodBuilder {
public:
    LodBuilder(const WorldGenerator& generator, const BlockRegistry& registry) : m_generator(generator), m_registry(registry) {}

    LodTileMesh build(const LodBuildRequest& req);

private:
    struct Voxel {
        bool          solid  = false;
        bool          opaque = false;
        std::uint8_t  finish = 0;
        Color         up, side, down;
    };

    struct Tally {
        BlockId       id;
        std::uint64_t r = 0, g = 0, b = 0;
    };

    void gather(const LodBuildRequest& req, int x0, int y0, int x1, int y1, int s, int G);

    void gather_sampled(const LodBuildRequest& req, int x0, int y0, int s, int G);

    std::vector<std::shared_ptr<const Chunk>> column_chunks(const LodBuildRequest& req, const ColumnPos& col) const;

    static void add_run(std::vector<LodRun>& runs, int z0, int z1, BlockId id);

    static void add_column(std::vector<LodRun>& runs, const std::vector<std::shared_ptr<const Chunk>>& column, int lx, int ly);

    void resolve(const LodBuildRequest& req, int per_axis, int G, int Z, int zmin);

    Color average(const std::vector<BlockId>& ids, const std::vector<std::uint32_t>& counts, std::size_t K, int Z, int z, Face f) const;

    const WorldGenerator&            m_generator;
    const BlockRegistry&             m_registry;
    GreedyMesher                     m_greedy;
    std::vector<std::vector<LodRun>> m_cells;
    std::vector<Voxel>               m_voxels;
    std::vector<int>                 m_lowest;
};

} // namespace voxelspire

#endif // VOXELSPIRE_RENDER_LOD_HPP