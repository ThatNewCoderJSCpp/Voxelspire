#ifndef VOXELSPIRE_BLOCK_REGISTRY_HPP
#define VOXELSPIRE_BLOCK_REGISTRY_HPP

#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include "block.hpp"

namespace voxelspire {

class BlockRegistry {
public:
    static constexpr std::size_t MAX_BLOCKS = 0x10000;

    BlockRegistry() { add<AirBlock>(); }
    BlockRegistry(const BlockRegistry&) = delete;
    BlockRegistry& operator=(const BlockRegistry&) = delete;

    template <typename T, typename... Args>
    BlockId add(Args&&... args) {
        if (m_locked) throw std::runtime_error("blocks must be registered before the world starts");
        auto block = std::make_unique<T>(std::forward<Args>(args)...);
        if (m_by_id.count(block->identifier())) throw std::runtime_error("block already registered: " + block->identifier().str());
        if (m_blocks.size() >= MAX_BLOCKS) throw std::runtime_error("too many block types");
        const BlockId id = static_cast<BlockId>(m_blocks.size());
        block->m_handle = id;
        m_by_id.emplace(block->identifier(), id);
        m_traits.push_back(block->traits());
        m_blocks.push_back(std::move(block));
        return id;
    }

    const Block& get(BlockId id) const noexcept {
        return id < m_blocks.size() ? *m_blocks[id] : *m_blocks[AIR_ID];
    }

    const BlockTraits& traits(BlockId id) const noexcept {
        return id < m_traits.size() ? m_traits[id] : m_traits[AIR_ID];
    }

    const BlockTraits* traits_table() const noexcept { return m_traits.data(); }

    std::optional<BlockId> find(const Identifier& identifier) const;

    std::optional<BlockId> find(std::string_view text) const;

    BlockId require(const Identifier& identifier) const;

    Identifier identifier(BlockId id) const { return get(id).identifier(); }

    std::size_t size() const noexcept { return m_blocks.size(); }

    void lock() noexcept { m_locked = true; }
    bool locked() const noexcept { return m_locked; }

private:
    std::vector<std::unique_ptr<Block>>                     m_blocks;
    std::vector<BlockTraits>                                m_traits;
    std::unordered_map<Identifier, BlockId, IdentifierHash> m_by_id;
    bool                                                    m_locked = false;
};

} // namespace voxelspire

#endif // VOXELSPIRE_BLOCK_REGISTRY_HPP