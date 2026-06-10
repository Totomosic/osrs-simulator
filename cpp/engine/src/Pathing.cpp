#include "Pathing.h"

namespace osrssim
{
namespace
{
int Abs(int value)
{
    return value < 0 ? -value : value;
}
}  // namespace

Pathing::Pathing(const Scene& scene)
    : m_Scene(scene)
{
}

bool Pathing::CanMove(SceneCoordinate from, SceneCoordinate to) const
{
    if (!m_Scene.Contains(from) || !m_Scene.Contains(to) || from.plane != to.plane)
    {
        return false;
    }

    const int dx = to.x - from.x;
    const int dy = to.y - from.y;

    if (!IsAdjacentStep(dx, dy))
    {
        return false;
    }

    if (IsDiagonalStep(dx, dy))
    {
        return CanMoveDiagonal(from, to);
    }

    return CanMoveCardinal(from, to);
}

bool Pathing::IsAdjacentStep(int dx, int dy)
{
    return Abs(dx) <= 1 && Abs(dy) <= 1 && (dx != 0 || dy != 0);
}

bool Pathing::IsDiagonalStep(int dx, int dy)
{
    return dx != 0 && dy != 0;
}

bool Pathing::IsDestinationBlocked(const Tile& tile)
{
    return tile.HasFlag(TileFlag::Occupied) ||
           tile.HasFlag(TileFlag::BlockMovementFull) ||
           tile.HasFlag(TileFlag::BlockMovementObject) ||
           tile.HasFlag(TileFlag::BlockMovementFloor) ||
           tile.HasFlag(TileFlag::BlockMovementFloorDecoration);
}

TileFlag Pathing::GetSourceMovementFlag(int dx, int dy)
{
    if (dx == -1 && dy == 1)
    {
        return TileFlag::BlockMovementNorthWest;
    }

    if (dx == 0 && dy == 1)
    {
        return TileFlag::BlockMovementNorth;
    }

    if (dx == 1 && dy == 1)
    {
        return TileFlag::BlockMovementNorthEast;
    }

    if (dx == 1 && dy == 0)
    {
        return TileFlag::BlockMovementEast;
    }

    if (dx == 1 && dy == -1)
    {
        return TileFlag::BlockMovementSouthEast;
    }

    if (dx == 0 && dy == -1)
    {
        return TileFlag::BlockMovementSouth;
    }

    if (dx == -1 && dy == -1)
    {
        return TileFlag::BlockMovementSouthWest;
    }

    return TileFlag::BlockMovementWest;
}

TileFlag Pathing::GetDestinationMovementFlag(int dx, int dy)
{
    return GetSourceMovementFlag(-dx, -dy);
}

bool Pathing::CanMoveCardinal(SceneCoordinate from, SceneCoordinate to) const
{
    const Tile* fromTile = m_Scene.TryGetTile(from);
    const Tile* toTile = m_Scene.TryGetTile(to);

    if (fromTile == nullptr || toTile == nullptr || IsDestinationBlocked(*toTile))
    {
        return false;
    }

    const int dx = to.x - from.x;
    const int dy = to.y - from.y;

    return !fromTile->HasFlag(GetSourceMovementFlag(dx, dy)) &&
           !toTile->HasFlag(GetDestinationMovementFlag(dx, dy));
}

bool Pathing::CanMoveDiagonal(SceneCoordinate from, SceneCoordinate to) const
{
    const Tile* fromTile = m_Scene.TryGetTile(from);
    const Tile* toTile = m_Scene.TryGetTile(to);

    if (fromTile == nullptr || toTile == nullptr || IsDestinationBlocked(*toTile))
    {
        return false;
    }

    const int dx = to.x - from.x;
    const int dy = to.y - from.y;

    SceneCoordinate horizontal{from.x + dx, from.y, from.plane};
    SceneCoordinate vertical{from.x, from.y + dy, from.plane};

    return !fromTile->HasFlag(GetSourceMovementFlag(dx, dy)) &&
           !toTile->HasFlag(GetDestinationMovementFlag(dx, dy)) &&
           CanMoveCardinal(from, horizontal) &&
           CanMoveCardinal(from, vertical);
}

}  // namespace osrssim
