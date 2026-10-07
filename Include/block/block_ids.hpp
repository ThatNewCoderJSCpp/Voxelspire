#ifndef VOXELSPIRE_BLOCK_BLOCK_IDS_HPP
#define VOXELSPIRE_BLOCK_BLOCK_IDS_HPP

#include <string_view>
#include "../core/identifier.hpp"
#include "../core/types.hpp"

namespace voxelspire {

namespace BlockIds {

inline Identifier block(std::string_view name) { return core_id(Kind::Block, name); }
inline Identifier block(std::initializer_list<std::string_view> path) { return core_id(Kind::Block, path); }

inline const Identifier AIR       = block("air");
inline const Identifier GRASS     = block("grass");
inline const Identifier DIRT      = block("dirt");
inline const Identifier STONE     = block("stone");
inline const Identifier BEDROCK   = block("bedrock");
inline const Identifier SAND      = block("sand");
inline const Identifier RED_SAND  = block("red_sand");
inline const Identifier SANDSTONE = block("sandstone");
inline const Identifier GRAVEL    = block("gravel");
inline const Identifier CLAY      = block("clay");
inline const Identifier SNOW      = block("snow");
inline const Identifier MUD       = block("mud");
inline const Identifier ICE       = block("ice");
inline const Identifier GLASS     = block("glass");
inline const Identifier WATER     = block("water");

} // namespace BlockIds

} // namespace voxelspire

#endif // VOXELSPIRE_BLOCK_BLOCK_IDS_HPP