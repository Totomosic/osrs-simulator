#include "Tile.h"

#include <cstddef>

namespace osrssim
{

bool WallObject::HasDirection(CardinalDirection direction) const
{
    for (int index = 0; index < directionCount; ++index)
    {
        if (directions[static_cast<std::size_t>(index)] == direction)
        {
            return true;
        }
    }

    return false;
}

bool Tile::HasFlag(TileFlag flag) const
{
    return (flags & flag) != 0;
}

void Tile::AddFlag(TileFlag flag)
{
    flags = flags | flag;
}

void Tile::RemoveFlag(TileFlag flag)
{
    flags &= ~ToTileFlags(flag);
}

bool Tile::IsOccupied() const
{
    return HasFlag(TileFlag::Occupied);
}

}  // namespace osrssim
