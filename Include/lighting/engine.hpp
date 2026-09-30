#ifndef VOXELSPIRE_LIGHTING_LIGHT_ENGINE_HPP
#define VOXELSPIRE_LIGHTING_LIGHT_ENGINE_HPP

#include <cstddef>
#include <cstdint>
#include <vector>
#include "../core/function_ref.hpp"
#include "../world/chunk.hpp"
#include "format.hpp"

namespace voxelspire {

struct LightStats {
    LightFormat format          = LightFormat::None;
    std::size_t columns         = 0;
    std::size_t sections        = 0;
    std::size_t bytes           = 0;
    std::size_t pending_columns = 0;
    std::size_t emitters        = 0;
    std::size_t cells_changed   = 0;
    double      update_ms       = 0.0;
};

struct LightBox {
    static constexpr int MARGIN = 1;
    static constexpr int N      = Chunk::SIZE + 2 * MARGIN;
    static constexpr int VOLUME = N * N * N;

    static constexpr std::size_t index(int x, int y, int z) noexcept {
        return static_cast<std::size_t>(((y + MARGIN) * N + (z + MARGIN)) * N + (x + MARGIN));
    }
};

struct EmitterBlock {
    BlockPos      pos;
    LightEmission light;
};

using ChunkChangeSink = FunctionRef<void(const ChunkPos&)>;

class LightEngine {
public:
    virtual ~LightEngine() = default;

    virtual LightFormat format() const noexcept = 0;

    virtual void column_loaded(const ColumnPos& c) = 0;
    virtual void column_unloaded(const ColumnPos& c) = 0;
    virtual void block_changed(const BlockPos& p, BlockId before, BlockId after) = 0;
    virtual void update(double budget_ms, ChunkChangeSink changed) = 0;

    virtual bool settled(const ColumnPos& c) const noexcept = 0;
    virtual bool idle() const noexcept = 0;

    virtual LightLevel level_at(const BlockPos& p) const noexcept = 0;
    virtual void sample_box(const ChunkPos& chunk, std::uint16_t* out) const = 0;
    virtual void collect_emitters(const vector3d& center, double radius, std::vector<EmitterBlock>& out) const = 0;

    virtual LightStats stats() const = 0;
};

} // namespace voxelspire

#endif // VOXELSPIRE_LIGHTING_LIGHT_ENGINE_HPP