#ifndef VOXELSPIRE_BLOCK_BLOCK_HPP
#define VOXELSPIRE_BLOCK_BLOCK_HPP

#include <cstdint>
#include <string>
#include <utility>
#include "../core/types.hpp"

namespace voxelspire {

class World;

using BlockId = std::uint16_t;
constexpr BlockId AIR_ID = 0;

struct BlockProperties {
    bool solid   = true;
    bool opaque  = true;
    bool visible = true;
    bool fluid   = false;
};

class Block {
public:
    Block(std::string name, BlockProperties props) : m_name(std::move(name)), m_props(props) {}
    virtual ~Block() = default;

    Block(const Block&) = delete;
    Block& operator=(const Block&) = delete;

    BlockId                id()         const noexcept { return m_id; }
    const std::string&     name()       const noexcept { return m_name; }
    const BlockProperties& properties() const noexcept { return m_props; }

    bool is_solid()   const noexcept { return m_props.solid; }
    bool is_opaque()  const noexcept { return m_props.opaque; }
    bool is_visible() const noexcept { return m_props.visible; }
    bool is_fluid()   const noexcept { return m_props.fluid; }

    virtual Color face_color(Face face, const BlockPos& pos) const = 0;

    virtual bool is_face_visible_against(const Block& neighbor) const noexcept {
        return is_visible() && !neighbor.is_opaque();
    }

    virtual AABB collision_box(const BlockPos& pos) const noexcept { return AABB::unit_block(pos); }

    virtual void on_placed(World&, const BlockPos&) {}
    virtual void on_removed(World&, const BlockPos&) {}

protected:
    static Color vary(const Color& base, const BlockPos& pos, int amount) noexcept {
        std::uint32_t h = static_cast<std::uint32_t>(pos.x) * 374761393u
                        + static_cast<std::uint32_t>(pos.y) * 668265263u
                        + static_cast<std::uint32_t>(pos.z) * 2147483647u;
        h = (h ^ (h >> 13)) * 1274126177u;
        h ^= h >> 16;
        const int delta = static_cast<int>(h % static_cast<std::uint32_t>(2 * amount + 1)) - amount;
        auto ch = [delta](std::uint8_t c) { return static_cast<std::uint8_t>(vclamp(int(c) + delta, 0, 255)); };
        return Color(ch(base.red()), ch(base.green()), ch(base.blue()), base.alpha());
    }

private:
    friend class BlockRegistry;
    BlockId         m_id = AIR_ID;
    std::string     m_name;
    BlockProperties m_props;
};

class AirBlock final : public Block {
public:
    AirBlock() : Block("air", BlockProperties{ false, false, false }) {}
    Color face_color(Face, const BlockPos&) const override { return Color(0, 0, 0, 0); }
};

} // namespace voxelspire

#endif // VOXELSPIRE_BLOCK_BLOCK_HPP