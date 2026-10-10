#include "item/item.hpp"

namespace voxelspire {

Item::Item(Identifier id, ItemProperties props) : m_id(id), m_props(std::move(props)) {
    if (m_id.kind() != Kind::Item) throw std::invalid_argument("item identifiers must be of kind item: " + m_id.str());
    if (m_props.name.empty()) m_props.name = pretty(m_id.name());
}

std::string Item::pretty(const std::string& raw) {
    std::string out;
    bool start = true;

    for (char ch : raw) {
        if (ch == '_' || ch == '-' || ch == '.') { out += ' '; start = true; continue; }
        out += start ? static_cast<char>(std::toupper(static_cast<unsigned char>(ch))) : ch;
        start = false;
    }

    return out;
}

const Item& ItemRegistry::block_item(const BlockRegistry& blocks, BlockId block, std::string category) {
    const Block& b = blocks.get(block);
    const BlockTraits& t = blocks.traits(block);
    const Identifier bid = b.identifier();
    std::vector<std::string> path{ BLOCKS };
    for (const std::string& p : bid.path()) path.push_back(p);
    ItemProperties props;
    props.block    = block;
    props.weight   = b.properties().weight;
    props.look     = ItemLook::Cube;
    props.category = std::move(category);
    props.color    = t.face(Face::Up).base;
    props.side     = t.face(Face::East).base;
    props.front    = t.face(Face::South).base;
    return add(Identifier(Kind::Item, bid.owner(), path), props);
}

} // namespace voxelspire
