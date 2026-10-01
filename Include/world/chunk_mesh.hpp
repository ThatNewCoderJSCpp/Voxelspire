#ifndef VOXELSPIRE_WORLD_CHUNK_MESH_HPP
#define VOXELSPIRE_WORLD_CHUNK_MESH_HPP

#include <array>
#include <bitset>
#include <cstring>
#include <memory>
#include <vector>
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
        capture_border(world, *snap);
        capture_levels(world, chunk, *snap);

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

private:
    static void capture_border(const World& world, ChunkSnapshot& snap) {
        constexpr int S = Chunk::SIZE;
        const ChunkPos p = snap.pos;

        for (int dz = -1; dz <= 1; ++dz)
            for (int dy = -1; dy <= 1; ++dy)
                for (int dx = -1; dx <= 1; ++dx) {
                    if (dx == 0 && dy == 0 && dz == 0) continue;
                    const Chunk* c = world.chunk_at({ p.x + dx, p.y + dy, p.z + dz });
                    if (!c) continue;
                    const int x0 = dx < 0 ? -1 : (dx > 0 ? S : 0), x1 = dx == 0 ? S : x0 + 1;
                    const int y0 = dy < 0 ? -1 : (dy > 0 ? S : 0), y1 = dy == 0 ? S : y0 + 1;
                    const int z0 = dz < 0 ? -1 : (dz > 0 ? S : 0), z1 = dz == 0 ? S : z0 + 1;

                    for (int y = y0; y < y1; ++y)
                        for (int z = z0; z < z1; ++z)
                            for (int x = x0; x < x1; ++x)
                                snap.ids[index(x, y, z)] = c->get(floor_mod(x, S), floor_mod(y, S), floor_mod(z, S));
                }
    }

    static void capture_levels(const World& world, const Chunk& chunk, ChunkSnapshot& snap) {
        const BlockTraits* traits = world.blocks().traits_table();
        const FluidRules& rules = world.fluid_rules();
        const BlockPos o = snap.origin();

        for (int y = -1; y <= Chunk::SIZE; ++y)
            for (int z = -1; z <= Chunk::SIZE; ++z)
                for (int x = -1; x <= Chunk::SIZE; ++x) {
                    if (!traits[snap.at(x, y, z)].fluid) continue;
                    const bool inside = Chunk::in_bounds(x, y, z);
                    const std::uint8_t state = inside ? chunk.state(x, y, z) : world.fluid_state({ o.x + x, o.y + y, o.z + z });
                    const double level = rules.level(state);
                    snap.levels[index(x, y, z)] = static_cast<std::uint8_t>(vclamp(std::lround(level * LEVEL_SCALE), 0L, static_cast<long>(LEVEL_SCALE)));
                }
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
            FluidSurface surface;

            if (t.fluid) {
                surface = fluid_surface(snap, traits, id, x, y, z);
                if (surface.lowered && f != Face::Down && (f != Face::Up || !surface.flat)) return cell;
            }

            const BlockPos o = face_offset(f);
            const int ax = x + o.x, ay = y + o.y, az = z + o.z;
            const BlockId nid = snap.at(ax, ay, az);
            if (!BlockTraits::face_visible(id, t, nid, traits[nid])) return cell;
            if (f == Face::Down && options.cull_void_faces && snap.has_floor && origin.z + z == snap.floor[ChunkColumn::cell(x, y)]) return cell;
            cell.key = face_key(id, f, options.shading);
            if (t.fluid && f == Face::Up) cell.key = FaceKey::with_surface(cell.key, surface.lowered ? surface.height[0] : 1.0, surface.flow);
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
            emit_scaled_quad(FaceKey::translucent(r.key) ? out.translucent : out.quads, r, vector3d{ zero.x, zero.y, zero.z - FaceKey::surface_drop(r.key) }, unit);
        });

        out.faces += emit_shaped(snap, options, traits, out);
        out.faces += emit_fluids(snap, options, traits, out);
        out.quads.finalize();
        out.translucent.finalize();
        out.connectivity = ChunkConnectivity::compute(snap, m_registry);
        return out;
    }

private:
    static constexpr int    OCCLUSION_LEVELS = 4;
    static constexpr double CELL_MIDPOINT    = 0.5;

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

        double at(int cx, int cy) const noexcept {
            for (std::size_t k = 0; k < SURFACE_CORNERS.size(); ++k)
                if (SURFACE_CORNERS[k][0] == cx && SURFACE_CORNERS[k][1] == cy) return height[k];
            return 1.0;
        }
    };

    static double cell_height(const ChunkSnapshot& snap, BlockId id, int x, int y, int z) noexcept {
        return snap.at(x, y, z + 1) == id ? 1.0 : snap.level_at(x, y, z);
    }

    static double corner_height(const ChunkSnapshot& snap, BlockId id, int x, int y, int z, int cx, int cy) noexcept {
        double sum = 0.0;
        int count = 0;

        for (int oy = cy - 1; oy <= cy; ++oy)
            for (int ox = cx - 1; ox <= cx; ++ox) {
                if (snap.at(x + ox, y + oy, z) != id) continue;
                if (snap.at(x + ox, y + oy, z + 1) == id) return 1.0;
                sum += snap.level_at(x + ox, y + oy, z);
                ++count;
            }

        return count > 0 ? sum / count : snap.level_at(x, y, z);
    }

    static std::uint8_t flow_code(const ChunkSnapshot& snap, const BlockTraits* traits, BlockId id, int x, int y, int z) noexcept {
        const double h = cell_height(snap, id, x, y, z);
        double fx = 0.0, fy = 0.0;

        for (Face f : SIDE_FACES) {
            const BlockPos o = face_offset(f);
            const BlockId nid = snap.at(x + o.x, y + o.y, z);
            double hn = 0.0;
            if (nid == id) hn = cell_height(snap, id, x + o.x, y + o.y, z);
            else if (traits[nid].solid) continue;
            fx += o.x * (h - hn);
            fy += o.y * (h - hn);
        }

        const double len = std::sqrt(fx * fx + fy * fy);
        const int speed = static_cast<int>(std::lround(vmin(len, 1.0) * FLOW_STEPS));
        if (speed == 0) return 0;
        const int angle = static_cast<int>(std::lround(std::atan2(fy, fx) / (2.0 * PI) * FLOW_ANGLES)) & (static_cast<int>(FLOW_ANGLES) - 1);
        return static_cast<std::uint8_t>((angle << FLOW_ANGLE_SHIFT) | speed);
    }

    static FluidSurface fluid_surface(const ChunkSnapshot& snap, const BlockTraits* traits, BlockId id, int x, int y, int z) noexcept {
        FluidSurface s;
        if (snap.at(x, y, z + 1) == id) return s;

        for (std::size_t k = 0; k < SURFACE_CORNERS.size(); ++k) {
            s.height[k] = corner_height(snap, id, x, y, z, SURFACE_CORNERS[k][0], SURFACE_CORNERS[k][1]);
            if (s.height[k] < 1.0 - LEVEL_EPSILON) s.lowered = true;
            if (std::fabs(s.height[k] - s.height[0]) > LEVEL_EPSILON) s.flat = false;
        }

        s.flow = flow_code(snap, traits, id, x, y, z);
        return s;
    }

    std::size_t emit_fluids(const ChunkSnapshot& snap, const ChunkMeshOptions& options, const BlockTraits* traits, ChunkMeshData& out) {
        constexpr int S = Chunk::SIZE;
        std::size_t faces = 0;

        for (int y = 0; y < S; ++y)
            for (int z = 0; z < S; ++z)
                for (int x = 0; x < S; ++x) {
                    const BlockId id = snap.at(x, y, z);
                    const BlockTraits& t = traits[id];
                    if (!t.fluid || !t.visible) continue;
                    const FluidSurface surface = fluid_surface(snap, traits, id, x, y, z);
                    if (!surface.lowered) continue;
                    const BlockId above = snap.at(x, y, z + 1);

                    if (!surface.flat && BlockTraits::face_visible(id, t, above, traits[above])) {
                        const std::uint64_t key = FaceKey::with_surface(face_key(id, Face::Up, options.shading), 1.0, surface.flow);
                        emit_fluid_face(out, x, y, z, Face::Up, key, snap.light_at(x, y, z + 1), t.emission, surface);
                        ++faces;
                    }

                    for (Face f : SIDE_FACES) {
                        const BlockPos o = face_offset(f);
                        const BlockId nid = snap.at(x + o.x, y + o.y, z);
                        if (nid == id || !BlockTraits::face_visible(id, t, nid, traits[nid])) continue;
                        emit_fluid_face(out, x, y, z, f, face_key(id, f, options.shading), snap.light_at(x + o.x, y + o.y, z), t.emission, surface);
                        ++faces;
                    }
                }

        return faces;
    }

    static void emit_fluid_face(ChunkMeshData& out, int x, int y, int z, Face f, std::uint64_t key, std::uint16_t light, const LightEmission& own, const FluidSurface& surface) {
        const vector3d lo{ double(x), double(y), double(z) };
        std::array<vector3d, 4> c;
        face_corners(lo, lo + vector3d{ 1.0, 1.0, 1.0 }, f, c);
        const double top = z + 1.0;

        for (vector3d& p : c)
            if (p.z >= top) p.z = z + surface.at(static_cast<int>(std::lround(p.x - x)), static_cast<int>(std::lround(p.y - y)));

        LightSum sum;
        sum.add(light);
        using V = fizmo::graphics::CompactVertex3D;
        const Color col = FaceKey::color(key);
        const auto cf = static_cast<fizmo::graphics::CellFace>(f);
        const std::uint8_t var = FaceKey::variation(key), flags = FaceKey::vertex_flags(key), shade = FaceKey::shade(key);
        const fizmo::graphics::BakedLight baked = fizmo::graphics::BakedLight::unpack(sum.baked(own));
        auto vertex = [&](int k) { return V(c[static_cast<std::size_t>(k)], col, cf, shade, var, flags, baked); };
        (FaceKey::translucent(key) ? out.translucent : out.quads).add_quad(vertex(0), vertex(1), vertex(2), vertex(3));
    }

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