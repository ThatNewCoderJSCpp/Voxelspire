#include "block/block_registry.hpp"

namespace voxelspire {

std::optional<BlockId> BlockRegistry::find(const Identifier& identifier) const {
    auto it = m_by_id.find(identifier);
    if (it == m_by_id.end()) return std::nullopt;
    return it->second;
}

std::optional<BlockId> BlockRegistry::find(std::string_view text) const {
    const Identifier identifier = Identifier::find(Kind::Block, text);
    if (!identifier) return std::nullopt;
    return find(identifier);
}

BlockId BlockRegistry::require(const Identifier& identifier) const {
    const auto id = find(identifier);
    if (!id) throw std::runtime_error("unknown block: " + (identifier ? identifier.str() : std::string("(none)")));
    return *id;
}

} // namespace voxelspire
