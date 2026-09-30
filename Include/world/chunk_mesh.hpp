#ifndef VOXELSPIRE_WORLD_CHUNK_MESH_HPP
#define VOXELSPIRE_WORLD_CHUNK_MESH_HPP

#include <array>
#include <bitset>
#include <cstring>
#include <memory>
#include <vector>
#include "../render/greedy_mesher.hpp"
#include "world.hpp"

namespace voxelspire {

struct ChunkSnapshot {
    static constexpr int N      = Chunk::SIZE + 2;
    static constexpr int VOLUME = N * N * N;

    ChunkPos      pos;
    std::uint64_t revision = 0;
    bool          has_floor = false;
    bool          has_light = false;
    std::array<std::int32_t, ChunkColumn::AREA> floor{};
    std::array<BlockId, VOLUME> ids{};
    std::array<std::uint16_t, LightBox::VOLUME> light{};

    static_assert(LightBox::N == N, "the light box must cover the same cells as the snapshot");

    static constexpr std::size_t index(int x, int y, int z) noexcept {
        return static_cast<std::size_t>(((y + 1) * N + (z + 1)) * N + (x + 1));
    }

    BlockId at(int x, int y, int z) const noexcept { return ids[index(x, y, z)]; }
    std::uint16_t light_at(int x, int y, int z) const noexcept { return has_light ? light[index(x, y, z)] : PackedLight::OPEN_SKY; }
    BlockPos origin() const noexcept { return { pos.x * Chunk::SIZE, pos.y * Chunk::SIZE, pos.z * Chunk::SIZE }; }

    static std::unique_ptr<ChunkSnapshot> capture(const World& world, const Chunk& chunk) {
        constexpr int S = Chunk::SIZE;
        auto snap = std::make_unique<ChunkSnapshot>();
        snap->pos = chunk.pos();
        snap->revision = chunk.revision();
        snap->ids.fill(AIR_ID);
        const BlockId* src = chunk.data();

        for (int y = 0; y < S; ++y)
            for (int z = 0; z < S; ++z)
                std::memcpy(&snap->ids[index(0, y, z)], &src[Chunk::index(0, y, z)], sizeof(BlockId) * S);

        const ChunkPos p = chunk.pos();

        if (const Chunk* c = world.chunk_at({ p.x - 1, p.y, p.z })) for (int y = 0; y < S; ++y) for (int z = 0; z < S; ++z) snap->ids[index(-1, y, z)] = c->get(S - 1, y, z);
        if (const Chunk* c = world.chunk_at({ p.x + 1, p.y, p.z })) for (int y = 0; y < S; ++y) for (int z = 0; z < S; ++z) snap->ids[index(S, y, z)] = c->get(0, y, z);
        if (const Chunk* c = world.chunk_at({ p.x, p.y - 1, p.z })) for (int x = 0; x < S; ++x) for (int z = 0; z < S; ++z) snap->ids[index(x, -1, z)] = c->get(x, S - 1, z);
        if (const Chunk* c = world.chunk_at({ p.x, p.y + 1, p.z })) for (int x = 0; x < S; ++x) for (int z = 0; z < S; ++z) snap->ids[index(x, S, z)] = c->get(x, 0, z);
        if (const Chunk* c = world.chunk_at({ p.x, p.y, p.z - 1 })) for (int x = 0; x < S; ++x) for (int y = 0; y < S; ++y) snap->ids[index(x, y, -1)] = c->get(x, y, S - 1);
        if (const Chunk* c = world.chunk_at({ p.x, p.y, p.z + 1 })) for (int x = 0; x < S; ++x) for (int y = 0; y < S; ++y) snap->ids[index(x, y, S)] = c->get(x, y, 0);

        if (const ChunkColumn* col = world.column(World::column_of(p))) {
            snap->has_floor = true;
            snap->floor = col->floor;
        }

        if (const LightEngine* light = world.lighting()) {
            snap->has_light = true;
            light->sample_box(p, snap->light.data());
        }

        return snap;
    }
};

struct ChunkConnectivity {
    static constexpr std::uint64_t ALL = (std::uint64_t(1) << (FACE_COUNT * FACE_COUNT)) - 1;

    std::uint64_t bits = ALL;

    bool connected(Face a, Face b) const noexcept { return (bits >> (static_cast<int>(a) * FACE_COUNT + static_cast<int>(b))) & 1u; }

    void connect(Face a, Face b) noexcept {
        bits |= std::uint64_t(1) << (static_cast<int>(a) * FACE_COUNT + static_cast<int>(b));
        bits |= std::uint64_t(1) << (static_cast<int>(b) * FACE_COUNT + static_cast<int>(a));
    }

    static ChunkConnectivity compute(const ChunkSnapshot& snap, const BlockRegistry& reg) {
        const BlockTraits* traits = reg.traits_table();
        constexpr int S = Chunk::SIZE;
        constexpr int V = Chunk::VOLUME;
        ChunkConnectivity out;
        out.bits = 0;
        std::bitset<V> open, seen;
        int open_count = 0;

        for (int y = 0; y < S; ++y)
            for (int z = 0; z < S; ++z)
                for (int x = 0; x < S; ++x)
                    if (!traits[snap.at(x, y, z)].opaque) { open.set(Chunk::index(x, y, z)); ++open_count; }

        if (open_count == V) { out.bits = ALL; return out; }
        if (open_count == 0) return out;

        std::vector<int> stack;
        stack.reserve(V);

        for (int start = 0; start < V; ++start) {
            if (!open.test(static_cast<std::size_t>(start)) || seen.test(static_cast<std::size_t>(start))) continue;
            unsigned int touched = 0;
            stack.clear();
            stack.push_back(start);
            seen.set(static_cast<std::size_t>(start));

            while (!stack.empty()) {
                const int i = stack.back();
                stack.pop_back();
                const int x = i % S, z = (i / S) % S, y = i / (S * S);
                if (x == 0) touched |= 1u << static_cast<int>(Face::West);
                if (x == S - 1) touched |= 1u << static_cast<int>(Face::East);
                if (y == 0) touched |= 1u << static_cast<int>(Face::South);
                if (y == S - 1) touched |= 1u << static_cast<int>(Face::North);
                if (z == 0) touched |= 1u << static_cast<int>(Face::Down);
                if (z == S - 1) touched |= 1u << static_cast<int>(Face::Up);

                for (Face f : ALL_FACES) {
                    const BlockPos o = face_offset(f);
                    const int nx = x + o.x, ny = y + o.y, nz = z + o.z;
                    if (!Chunk::in_bounds(nx, ny, nz)) continue;
                    const int n = static_cast<int>(Chunk::index(nx, ny, nz));
                    if (!open.test(static_cast<std::size_t>(n)) || seen.test(static_cast<std::size_t>(n))) continue;
                    seen.set(static_cast<std::size_t>(n));
                    stack.push_back(n);
                }
            }

            for (Face a : ALL_FACES)
                for (Face b : ALL_FACES)
                    if ((touched >> static_cast<int>(a) & 1u) && (touched >> static_cast<int>(b) & 1u)) out.connect(a, b);

            if (out.bits == ALL) break;
        }

        return out;
    }
};

struct ChunkMeshOptions {
    bool        merge_faces       = true;
    bool        cull_void_faces   = true;
    bool        smooth_lighting   = true;
    double      ambient_occlusion = 1.0;
    double      occlusion_step    = 0.2;
    FaceShading shading;

    bool operator==(const ChunkMeshOptions& o) const noexcept {
        return merge_faces == o.merge_faces && cull_void_faces == o.cull_void_faces && smooth_lighting == o.smooth_lighting
            && ambient_occlusion == o.ambient_occlusion && occlusion_step == o.occlusion_step
            && shading.strength == o.shading.strength && shading.factors.up == o.shading.factors.up
            && shading.factors.down == o.shading.factors.down && shading.factors.north_south == o.shading.factors.north_south
            && shading.factors.east_west == o.shading.factors.east_west;
    }

    bool operator!=(const ChunkMeshOptions& o) const noexcept { return !(*this == o); }
};

struct ChunkMeshData {
    ChunkPos                    pos;
    std::uint64_t               revision = 0;
    fizmo::graphics::QuadMesh3D quads;
    fizmo::graphics::QuadMesh3D translucent;
    std::size_t                 faces = 0;
    ChunkConnectivity           connectivity;
};

class ChunkMesher {
public:
    explicit ChunkMesher(const BlockRegistry& registry) : m_registry(registry) { build_corner_table(); }

    ChunkMeshData build(const ChunkSnapshot& snap, const ChunkMeshOptions& options) {
        ChunkMeshData out;
        out.pos = snap.pos;
        out.revision = snap.revision;
        const int dims[3] = { Chunk::SIZE, Chunk::SIZE, Chunk::SIZE };
        const BlockPos origin = snap.origin();
        const vector3d zero{ 0.0, 0.0, 0.0 }, unit{ 1.0, 1.0, 1.0 };

        const BlockTraits* traits = m_registry.traits_table();

        const bool occlusion = options.ambient_occlusion > 0.0;
        const bool smooth = options.smooth_lighting && snap.has_light;
        std::array<double, OCCLUSION_LEVELS> occlusion_factor{};
        for (int k = 0; k < OCCLUSION_LEVELS; ++k) occlusion_factor[static_cast<std::size_t>(k)] = vmax(0.0, 1.0 - options.ambient_occlusion * options.occlusion_step * k);

        auto key_at = [&](Face f, int x, int y, int z) -> FaceCell {
            FaceCell cell;
            const BlockId id = snap.at(x, y, z);
            const BlockTraits& t = traits[id];
            if (!t.visible || !t.full_cube) return cell;
            const BlockPos o = face_offset(f);
            const int ax = x + o.x, ay = y + o.y, az = z + o.z;
            const BlockId nid = snap.at(ax, ay, az);
            if (!BlockTraits::face_visible(id, t, nid, traits[nid])) return cell;
            if (f == Face::Down && options.cull_void_faces && snap.has_floor && origin.z + z == snap.floor[ChunkColumn::cell(x, y)]) return cell;
            cell.key = face_key(id, f, options.shading);
            const std::uint8_t shade = FaceKey::shade(cell.key);
            const std::uint16_t flat = snap.light_at(ax, ay, az);

            for (int k = 0; k < FaceCell::CORNERS; ++k) {
                const CornerSamples& cs = m_corners[static_cast<std::size_t>(f)][static_cast<std::size_t>(k)];
                const bool side_a = traits[snap.at(ax + cs.a.x, ay + cs.a.y, az + cs.a.z)].opaque;
                const bool side_b = traits[snap.at(ax + cs.b.x, ay + cs.b.y, az + cs.b.z)].opaque;
                const bool corner = !(side_a && side_b) && traits[snap.at(ax + cs.c.x, ay + cs.c.y, az + cs.c.z)].opaque;
                const int blocked = side_a && side_b ? OCCLUSION_LEVELS - 1 : int(side_a) + int(side_b) + int(corner);
                const double shade_k = occlusion ? shade * occlusion_factor[static_cast<std::size_t>(blocked)] : shade;
                cell.shade[static_cast<std::size_t>(k)] = static_cast<std::uint8_t>(vclamp(std::lround(shade_k), 0L, 255L));

                LightSum sum;
                sum.add(flat);

                if (smooth) {
                    if (!side_a) sum.add(snap.light_at(ax + cs.a.x, ay + cs.a.y, az + cs.a.z));
                    if (!side_b) sum.add(snap.light_at(ax + cs.b.x, ay + cs.b.y, az + cs.b.z));
                    if (!side_a || !side_b) if (!corner) sum.add(snap.light_at(ax + cs.c.x, ay + cs.c.y, az + cs.c.z));
                }

                cell.light[static_cast<std::size_t>(k)] = sum.baked(t.emission);
            }

            return cell;
        };

        out.faces = m_greedy.mesh(dims, options.merge_faces, key_at, [&](const GreedyRect& r) {
            emit_scaled_quad(FaceKey::translucent(r.key) ? out.translucent : out.quads, r, zero, unit);
        });

        out.faces += emit_shaped(snap, options, traits, out);
        out.quads.finalize();
        out.translucent.finalize();
        out.connectivity = ChunkConnectivity::compute(snap, m_registry);
        return out;
    }

private:
    static constexpr int    OCCLUSION_LEVELS = 4;
    static constexpr double CELL_MIDPOINT    = 0.5;

    struct CornerSamples { BlockPos a, b, c; };

    std::size_t emit_shaped(const ChunkSnapshot& snap, const ChunkMeshOptions& options, const BlockTraits* traits, ChunkMeshData& out) {
        constexpr int S = Chunk::SIZE;
        std::size_t faces = 0;

        for (int y = 0; y < S; ++y)
            for (int z = 0; z < S; ++z)
                for (int x = 0; x < S; ++x) {
                    const BlockId id = snap.at(x, y, z);
                    const BlockTraits& t = traits[id];
                    if (!t.visible || t.full_cube) continue;
                    const vector3d lo = vector3d{ double(x), double(y), double(z) } + t.shape.min;
                    const vector3d hi = vector3d{ double(x), double(y), double(z) } + t.shape.max;

                    for (Face f : ALL_FACES) {
                        std::uint16_t light = snap.light_at(x, y, z);

                        if (t.shape.touches(f)) {
                            const BlockPos o = face_offset(f);
                            const BlockId nid = snap.at(x + o.x, y + o.y, z + o.z);
                            if (!BlockTraits::face_visible(id, t, nid, traits[nid])) continue;
                            light = snap.light_at(x + o.x, y + o.y, z + o.z);
                        }

                        LightSum sum;
                        sum.add(light);
                        const std::uint64_t key = face_key(id, f, options.shading);
                        emit_box_face(FaceKey::translucent(key) ? out.translucent : out.quads, lo, hi, f, key, sum.baked(t.emission));
                        ++faces;
                    }
                }

        return faces;
    }

    static void emit_box_face(fizmo::graphics::QuadMesh3D& mesh, const vector3d& lo, const vector3d& hi, Face f, std::uint64_t key, std::uint32_t light) {
        std::array<vector3d, 4> c;
        face_corners(lo, hi, f, c);
        using V = fizmo::graphics::CompactVertex3D;
        const Color col = FaceKey::color(key);
        const auto cf = static_cast<fizmo::graphics::CellFace>(f);
        const std::uint8_t var = FaceKey::variation(key), flags = FaceKey::vertex_flags(key), shade = FaceKey::shade(key);
        const fizmo::graphics::BakedLight baked = fizmo::graphics::BakedLight::unpack(light);
        auto vertex = [&](int k) { return V(c[static_cast<std::size_t>(k)], col, cf, shade, var, flags, baked); };
        mesh.add_quad(vertex(0), vertex(1), vertex(2), vertex(3));
    }

    struct LightSum {
        int sky = 0, red = 0, green = 0, blue = 0, count = 0;

        void add(std::uint16_t v) noexcept {
            sky   += PackedLight::sky(v);
            red   += PackedLight::channel(v, 0);
            green += PackedLight::channel(v, 1);
            blue  += PackedLight::channel(v, 2);
            ++count;
        }

        std::uint32_t baked(const LightEmission& own) const noexcept {
            auto scale = [this](int total, int floor_level) {
                const int avg = (total * LightLevel::BYTE_SCALE + count / 2) / count;
                return static_cast<std::uint8_t>(vmax(avg, floor_level * LightLevel::BYTE_SCALE));
            };
            return fizmo::graphics::BakedLight(scale(red, own.red), scale(green, own.green), scale(blue, own.blue), scale(sky, 0)).packed();
        }
    };

    void build_corner_table() {
        for (Face f : ALL_FACES) {
            std::array<vector3d, 4> corners;
            face_corners(vector3d{ 0.0, 0.0, 0.0 }, vector3d{ 1.0, 1.0, 1.0 }, f, corners);
            const int a = face_axis(f), u = (a + 1) % 3, v = (a + 2) % 3;

            for (int k = 0; k < FaceCell::CORNERS; ++k) {
                const vector3d& p = corners[static_cast<std::size_t>(k)];
                int du[3] = { 0, 0, 0 }, dv[3] = { 0, 0, 0 };
                du[u] = component(p, u) > CELL_MIDPOINT ? 1 : -1;
                dv[v] = component(p, v) > CELL_MIDPOINT ? 1 : -1;
                CornerSamples& cs = m_corners[static_cast<std::size_t>(f)][static_cast<std::size_t>(k)];
                cs.a = { du[0], du[1], du[2] };
                cs.b = { dv[0], dv[1], dv[2] };
                cs.c = { du[0] + dv[0], du[1] + dv[1], du[2] + dv[2] };
            }
        }
    }

    std::uint64_t face_key(BlockId id, Face f, const FaceShading& shading) {
        if (id >= m_keys.size()) m_keys.resize(static_cast<std::size_t>(id) + 1, {});
        auto& row = m_keys[id];
        std::uint64_t& k = row[static_cast<std::size_t>(f)];

        if (k == FaceKey::NONE) {
            const BlockTraits& t = m_registry.traits(id);
            const FaceAppearance& a = t.face(f);
            k = FaceKey::make(a.base, a.variation, shading.level(f), t.layer == RenderLayer::Translucent, a.plain ? 0 : FaceKey::finish_flags(t.finish));
        }

        return k;
    }

    const BlockRegistry&                            m_registry;
    GreedyMesher                                    m_greedy;
    std::array<std::array<CornerSamples, FaceCell::CORNERS>, FACE_COUNT> m_corners{};
    std::vector<std::array<std::uint64_t, FACE_COUNT>> m_keys;
};

} // namespace voxelspire

#endif // VOXELSPIRE_WORLD_CHUNK_MESH_HPP