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

inline constexpr const char* LAMP   = "lamp";
inline constexpr const char* MIRROR = "mirror";

inline Identifier lamp(std::string_view color) { return block({ LAMP, color }); }
inline Identifier mirror(Face facing)          { return block({ MIRROR, face_name(facing) }); }

inline const Identifier WARM_LAMP  = lamp("warm");
inline const Identifier BLUE_LAMP  = lamp("blue");
inline const Identifier RED_LAMP   = lamp("red");
inline const Identifier GREEN_LAMP = lamp("green");

} // namespace BlockIds

} // namespace voxelspire

#endif // VOXELSPIRE_BLOCK_BLOCK_IDS_HPP