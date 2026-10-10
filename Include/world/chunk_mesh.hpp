#ifndef VOXELSPIRE_WORLD_CHUNK_MESH_HPP
#define VOXELSPIRE_WORLD_CHUNK_MESH_HPP

#include <array>
#include <bitset>
#include <cstring>
#include <memory>
#include <vector>
#include "../physics/waves.hpp"
#include "../render/greedy_mesher.hpp"
#include "world.hpp"
#include <cmath>

namespace voxelspire {

struct ChunkSnapshot {
    static constexpr int N      = Chunk::SIZE + 2;
    static constexpr int VOLUME = N * N * N;
    static constexpr double LEVEL_SCALE = 255.0;

    ChunkPos      pos;
    std::uint64_t revision = 0;
    bool          has_floor = false;
    bool          has_light = false;
    std::array<std::int32_t, ChunkColumn::AREA> floor{};
    std::array<BlockId, VOLUME> ids{};
    std::array<std::uint8_t, VOLUME> levels{};
    std::array<std::uint16_t, LightBox::VOLUME> light{};

    static_assert(LightBox::N == N, "the light box must cover the same cells as the snapshot");

    static constexpr std::size_t index(int x, int y, int z) noexcept {
        return static_cast<std::size_t>(((y + 1) * N + (z + 1)) * N + (x + 1));
    }

    BlockId at(int x, int y, int z) const noexcept { return ids[index(x, y, z)]; }
    std::uint16_t light_at(int x, int y, int z) const noexcept { return has_light ? light[index(x, y, z)] : PackedLight::OPEN_SKY; }
    double level_at(int x, int y, int z) const noexcept { return levels[index(x, y, z)] / LEVEL_SCALE; }
    BlockPos origin() const noexcept { return { pos.x * Chunk::SIZE, pos.y * Chunk::SIZE, pos.z * Chunk::SIZE }; }

    static std::unique_ptr<ChunkSnapshot> capture(const World& world, const Chunk& chunk);

private:
    static void capture_border(const World& world, ChunkSnapshot& snap);

    static void capture_levels(const World& world, const Chunk& chunk, ChunkSnapshot& snap);
};

struct ChunkConnectivity {
    static constexpr std::uint64_t ALL = (std::uint64_t(1) << (FACE_COUNT * FACE_COUNT)) - 1;

    std::uint64_t bits = ALL;

    bool connected(Face a, Face b) const noexcept { return (bits >> (static_cast<int>(a) * FACE_COUNT + static_cast<int>(b))) & 1u; }

    void connect(Face a, Face b) noexcept;

    static ChunkConnectivity compute(const ChunkSnapshot& snap, const BlockRegistry& reg);
};

struct ChunkMeshOptions {
    bool          merge_faces       = true;
    bool          cull_void_faces   = true;
    bool          smooth_lighting   = true;
    double        ambient_occlusion = 1.0;
    double        occlusion_step    = 0.2;
    FaceShading   shading;
    WaterLookPtr  water;
    bool          waves             = false;
    bool          split_tops        = false;

    bool operator==(const ChunkMeshOptions& o) const noexcept;

    bool operator!=(const ChunkMeshOptions& o) const noexcept { return !(*this == o); }
};

struct ChunkMeshData {
    ChunkPos                    pos;
    std::uint64_t               revision = 0;
    fizmo::graphics::QuadMesh3D quads;
    fizmo::graphics::QuadMesh3D translucent;
    std::size_t                 faces = 0;
    ChunkConnectivity           connectivity;
    bool                        fluid_tops  = false;
    bool                        split       = false;
    bool                        sunlit      = true;
    std::uint64_t               opaque_hash = 0;
};

class ChunkMesher {
public:
    explicit ChunkMesher(const BlockRegistry& registry) : m_registry(registry) { build_corner_table(); }

    ChunkMeshData build(const ChunkSnapshot& snap, const ChunkMeshOptions& options);

private:
    static bool reaches_sky(const ChunkSnapshot& snap) noexcept;

    static std::uint64_t hash_of(const fizmo::graphics::QuadMesh3D& mesh) noexcept;

    static constexpr std::uint64_t HASH_SEED        = 0xCBF29CE484222325ull;
    static constexpr std::uint64_t HASH_PRIME       = 0x100000001B3ull;
    static constexpr int           HASH_SHIFT       = 29;
    static constexpr int           OCCLUSION_LEVELS = 4;
    static constexpr double        CELL_MIDPOINT    = 0.5;
    static constexpr double        HALF_CELL        = 0.5;
    static constexpr int           WATER_STEP       = 4;
    static constexpr int           WATER_GRID       = Chunk::SIZE / WATER_STEP + 1;
    static constexpr int           TINT_STEP        = 4;

    struct CornerSamples { BlockPos a, b, c; };

    static constexpr int    FLUID_SIDES     = 4;
    static constexpr double LEVEL_EPSILON   = 1e-3;
    static constexpr double FLOW_ANGLES     = 16.0;
    static constexpr double FLOW_STEPS      = 15.0;
    static constexpr int    FLOW_ANGLE_SHIFT = 4;
    static constexpr std::array<Face, FLUID_SIDES> SIDE_FACES{ Face::West, Face::East, Face::South, Face::North };
    static constexpr std::array<std::array<int, 2>, FaceCell::CORNERS> SURFACE_CORNERS{ { { 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, 1 } } };

    struct FluidSurface {
        bool                                   lowered = false;
        bool                                   flat    = true;
        std::array<double, FaceCell::CORNERS>  height{ 1.0, 1.0, 1.0, 1.0 };
        std::uint8_t                           flow    = 0;

        double at(int cx, int cy) const noexcept;
    };

    static double cell_height(const ChunkSnapshot& snap, BlockId id, int x, int y, int z) noexcept {
        return snap.at(x, y, z + 1) == id ? 1.0 : snap.level_at(x, y, z);
    }

    static double corner_height(const ChunkSnapshot& snap, BlockId id, int x, int y, int z, int cx, int cy) noexcept;

    static std::uint8_t flow_code(const ChunkSnapshot& snap, const BlockTraits* traits, BlockId id, int x, int y, int z) noexcept;

    static FluidSurface fluid_surface(const ChunkSnapshot& snap, const BlockTraits* traits, BlockId id, int x, int y, int z) noexcept;

    static bool open_corner(const ChunkSnapshot& snap, const BlockTraits* traits, BlockId id, int cx, int cy, int z) noexcept;

    void fill_water(const ChunkSnapshot& snap, const ChunkMeshOptions& options, bool& filled);

    WaterSample water_at(double x, double y) const noexcept;

    static Color quantized(const Color& c, std::uint8_t alpha) noexcept;

    std::array<std::uint8_t, FaceCell::CORNERS> swell_bits(const ChunkSnapshot& snap, const ChunkMeshOptions& options, const BlockTraits* traits, BlockId id, int x, int y, int z, bool& filled);

    std::array<Color, FaceCell::CORNERS> corner_tints(const ChunkSnapshot& snap, const ChunkMeshOptions& options, int x, int y, std::uint8_t alpha, bool& filled);

    std::size_t emit_fluids(const ChunkSnapshot& snap, const ChunkMeshOptions& options, const BlockTraits* traits, ChunkMeshData& out, bool& water_ready);

    static void emit_fluid_face(ChunkMeshData& out, int x, int y, int z, Face f, std::uint64_t key, std::uint16_t light, const LightEmission& own, const FluidSurface& surface,
                                const std::array<std::uint8_t, FaceCell::CORNERS>& swell = {}, const std::array<Color, FaceCell::CORNERS>* tints = nullptr);

    std::size_t emit_shaped(const ChunkSnapshot& snap, const ChunkMeshOptions& options, const BlockTraits* traits, ChunkMeshData& out);

    static void emit_box_face(fizmo::graphics::QuadMesh3D& mesh, const vector3d& lo, const vector3d& hi, Face f, std::uint64_t key, std::uint32_t light);

    struct LightSum {
        int sky = 0;
        int red = 0;
        int green = 0;
        int blue = 0;
        int count = 0;

        void add(std::uint16_t v) noexcept;

        std::uint32_t baked(const LightEmission& own) const noexcept;
    };

    void build_corner_table();

    std::uint64_t face_key(BlockId id, Face f, const FaceShading& shading);

    const BlockRegistry&                            m_registry;
    GreedyMesher                                    m_greedy;
    std::array<WaterSample, WATER_GRID * WATER_GRID> m_water{};
    std::array<std::array<CornerSamples, FaceCell::CORNERS>, FACE_COUNT> m_corners{};
    std::vector<std::array<std::uint64_t, FACE_COUNT>> m_keys;
};

} // namespace voxelspire

#endif // VOXELSPIRE_WORLD_CHUNK_MESH_HPP