#include "Tile.h"

namespace osrssim
{

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
