#ifndef VOXELSPIRE_WORLD_CHUNK_HPP
#define VOXELSPIRE_WORLD_CHUNK_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>
#include "../block/block.hpp"
#include "../core/settings.hpp"

namespace voxelspire {

struct ChunkPos {
    int x = 0, y = 0, z = 0;
    constexpr bool operator==(const ChunkPos& o) const noexcept { return x == o.x && y == o.y && z == o.z; }
    constexpr bool operator!=(const ChunkPos& o) const noexcept { return !(*this == o); }
};

struct ChunkPosHash {
    std::size_t operator()(const ChunkPos& p) const noexcept { return BlockPosHash{}(BlockPos{ p.x, p.y, p.z }); }
};

struct ColumnPos {
    int x = 0, y = 0;
    constexpr bool operator==(const ColumnPos& o) const noexcept { return x == o.x && y == o.y; }
    constexpr bool operator!=(const ColumnPos& o) const noexcept { return !(*this == o); }
};

struct ColumnPosHash {
    std::size_t operator()(const ColumnPos& p) const noexcept;
};

class Chunk {
public:
    static constexpr int SIZE   = EngineLimits::CHUNK_SIZE;
    static constexpr int VOLUME = SIZE * SIZE * SIZE;

    explicit Chunk(ChunkPos pos) noexcept : m_pos(pos) { m_blocks.fill(AIR_ID); }

    const ChunkPos& pos() const noexcept { return m_pos; }
    BlockPos origin() const noexcept { return { m_pos.x * SIZE, m_pos.y * SIZE, m_pos.z * SIZE }; }

    static constexpr bool in_bounds(int lx, int ly, int lz) noexcept {
        return lx >= 0 && ly >= 0 && lz >= 0 && lx < SIZE && ly < SIZE && lz < SIZE;
    }

    BlockId get(int lx, int ly, int lz) const noexcept { return m_blocks[index(lx, ly, lz)]; }

    std::uint8_t state(int lx, int ly, int lz) const noexcept { return m_states.empty() ? 0 : m_states[index(lx, ly, lz)]; }

    bool set(int lx, int ly, int lz, BlockId id, std::uint8_t state = 0) noexcept;

    std::size_t fill(int x0, int x1, int y0, int y1, int z0, int z1, BlockId id) noexcept;

    bool modified() const noexcept { return m_modified; }
    void mark_pristine() noexcept { m_modified = false; }
    void mark_modified() noexcept { m_modified = true; }

    bool                has_states() const noexcept { return !m_states.empty(); }
    const std::uint8_t* states()     const noexcept { return m_states.data(); }

    void set_blocks(const std::vector<BlockId>& blocks) noexcept;

    void set_states(const std::vector<std::uint8_t>& states) {
        m_states = states;
        m_states.resize(VOLUME, 0);
        ++m_revision;
    }

    const BlockId* data() const noexcept { return m_blocks.data(); }

    static constexpr std::size_t index(int lx, int ly, int lz) noexcept {
        return static_cast<std::size_t>((ly * SIZE + lz) * SIZE + lx);
    }

    bool         empty()         const noexcept { return m_non_air == 0; }
    std::size_t  non_air_count() const noexcept { return m_non_air; }

    std::uint64_t revision() const noexcept { return m_revision; }

private:
    ChunkPos                    m_pos;
    std::array<BlockId, VOLUME> m_blocks{};
    std::vector<std::uint8_t>   m_states;
    std::size_t                 m_non_air   = 0;
    bool                        m_modified  = false;
    std::uint64_t               m_revision  = 0;
};

} // namespace voxelspire

#endif // VOXELSPIRE_WORLD_CHUNK_HPP