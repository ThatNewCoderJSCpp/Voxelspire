#ifndef VOXELSPIRE_BLOCK_REGISTRY_HPP
#define VOXELSPIRE_BLOCK_REGISTRY_HPP

#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
#include "block.hpp"

namespace voxelspire {

class BlockRegistry {
public:
    BlockRegistry() { add<AirBlock>(); }
    BlockRegistry(const BlockRegistry&) = delete;
    BlockRegistry& operator=(const BlockRegistry&) = delete;

    template <typename T, typename... Args>
    BlockId add(Args&&... args) {
        auto block = std::make_unique<T>(std::forward<Args>(args)...);
        if (m_by_name.count(block->name())) throw std::runtime_error("block already registered: " + block->name());
        if (m_blocks.size() > 0xFFFF) throw std::runtime_error("too many block types");
        const BlockId id = static_cast<BlockId>(m_blocks.size());
        block->m_id = id;
        m_by_name.emplace(block->name(), id);
        m_blocks.push_back(std::move(block));
        return id;
    }

    const Block& get(BlockId id) const noexcept {
        return id < m_blocks.size() ? *m_blocks[id] : *m_blocks[AIR_ID];
    }

    std::optional<BlockId> find(const std::string& name) const {
        auto it = m_by_name.find(name);
        if (it == m_by_name.end()) return std::nullopt;
        return it->second;
    }

    std::size_t size() const noexcept { return m_blocks.size(); }

private:
    std::vector<std::unique_ptr<Block>>      m_blocks;
    std::unordered_map<std::string, BlockId> m_by_name;
};

} // namespace voxelspire

#endif // VOXELSPIRE_BLOCK_REGISTRY_HPP