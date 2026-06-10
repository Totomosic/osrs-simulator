#pragma once

#include "Types.h"

#include <cstdint>

namespace osrssim
{

enum class TileFlag : std::uint32_t
{
    None = 0,
    Occupied = 1u << 0,

    BlockMovementNorthWest = 1u << 1,
    BlockMovementNorth = 1u << 2,
    BlockMovementNorthEast = 1u << 3,
    BlockMovementEast = 1u << 4,
    BlockMovementSouthEast = 1u << 5,
    BlockMovementSouth = 1u << 6,
    BlockMovementSouthWest = 1u << 7,
    BlockMovementWest = 1u << 8,
    BlockMovementFull = 1u << 9,
    BlockMovementObject = 1u << 10,
    BlockMovementFloor = 1u << 11,
    BlockMovementFloorDecoration = 1u << 12,

    BlockLineOfSightNorth = 1u << 13,
    BlockLineOfSightEast = 1u << 14,
    BlockLineOfSightSouth = 1u << 15,
    BlockLineOfSightWest = 1u << 16,
    BlockLineOfSightFull = 1u << 17,
};

using TileFlags = std::uint32_t;

constexpr TileFlags ToTileFlags(TileFlag flag)
{
    return static_cast<TileFlags>(flag);
}

constexpr TileFlags operator|(TileFlag lhs, TileFlag rhs)
{
    return ToTileFlags(lhs) | ToTileFlags(rhs);
}

constexpr TileFlags operator|(TileFlags lhs, TileFlag rhs)
{
    return lhs | ToTileFlags(rhs);
}

constexpr TileFlags operator&(TileFlags lhs, TileFlag rhs)
{
    return lhs & ToTileFlags(rhs);
}

struct Tile
{
    SceneCoordinate coordinate;
    TileFlags flags = ToTileFlags(TileFlag::None);

    bool HasFlag(TileFlag flag) const;
    void AddFlag(TileFlag flag);
    void RemoveFlag(TileFlag flag);
    bool IsOccupied() const;
};

}  // namespace osrssim
