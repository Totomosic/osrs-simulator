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

bool Pathing::CanMove(
    SceneCoordinate from,
    SceneCoordinate to,
    int actorSpeed,
    int actorSize) const
{
    if (actorSpeed <= 0 || actorSize <= 0 || from.plane != to.plane ||
        !CanStand(from, actorSize, true) || !CanStand(to, actorSize, true))
    {
        return false;
    }

    const int dx = to.x - from.x;
    const int dy = to.y - from.y;

    if (dx == 0 && dy == 0)
    {
        return false;
    }

    if (Abs(dx) > actorSpeed || Abs(dy) > actorSpeed)
    {
        return false;
    }

    return CanMoveMonotonicRoute(
        from,
        to,
        actorSpeed,
        actorSize,
        true,
        DiagonalSideFootprintRule::RequireClear);
}

bool Pathing::CanMoveIgnoringActorOccupancy(
    SceneCoordinate from,
    SceneCoordinate to,
    int actorSpeed,
    int actorSize) const
{
    if (actorSpeed <= 0 || actorSize <= 0 || from.plane != to.plane ||
        !CanStand(from, actorSize, false) || !CanStand(to, actorSize, false))
    {
        return false;
    }

    const int dx = to.x - from.x;
    const int dy = to.y - from.y;

    if (dx == 0 && dy == 0)
    {
        return false;
    }

    if (Abs(dx) > actorSpeed || Abs(dy) > actorSpeed)
    {
        return false;
    }

    return CanMoveIgnoringActorOccupancy(
        from,
        to,
        actorSpeed,
        actorSize,
        DiagonalSideFootprintRule::RequireClear);
}

bool Pathing::CanMoveIgnoringActorOccupancy(
    SceneCoordinate from,
    SceneCoordinate to,
    int actorSpeed,
    int actorSize,
    DiagonalSideFootprintRule diagonalSideFootprintRule) const
{
    if (actorSpeed <= 0 || actorSize <= 0 || from.plane != to.plane ||
        !CanStand(from, actorSize, false) || !CanStand(to, actorSize, false))
    {
        return false;
    }

    const int dx = to.x - from.x;
    const int dy = to.y - from.y;

    if (dx == 0 && dy == 0)
    {
        return false;
    }

    if (Abs(dx) > actorSpeed || Abs(dy) > actorSpeed)
    {
        return false;
    }

    return CanMoveMonotonicRoute(
        from,
        to,
        actorSpeed,
        actorSize,
        false,
        diagonalSideFootprintRule);
}

bool Pathing::IsAdjacentStep(int dx, int dy)
{
    return Abs(dx) <= 1 && Abs(dy) <= 1 && (dx != 0 || dy != 0);
}

bool Pathing::IsDiagonalStep(int dx, int dy)
{
    return dx != 0 && dy != 0;
}

bool Pathing::IsDestinationBlocked(const Tile& tile, bool includeActorOccupancy)
{
    return (includeActorOccupancy && tile.HasFlag(TileFlag::Occupied)) ||
           tile.HasFlag(TileFlag::BlockMovementFull) ||
           tile.HasFlag(TileFlag::BlockMovementObject) ||
           tile.HasFlag(TileFlag::BlockMovementFloor) ||
           tile.HasFlag(TileFlag::BlockMovementFloorDecoration);
}

int Pathing::Sign(int value)
{
    if (value < 0)
    {
        return -1;
    }

    if (value > 0)
    {
        return 1;
    }

    return 0;
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

bool Pathing::CanStand(
    SceneCoordinate anchor,
    int actorSize,
    bool includeActorOccupancy) const
{
    for (int dx = 0; dx < actorSize; ++dx)
    {
        for (int dy = 0; dy < actorSize; ++dy)
        {
            const Tile* tile =
                m_Scene.TryGetTile({anchor.x + dx, anchor.y + dy, anchor.plane});

            if (tile == nullptr ||
                IsDestinationBlocked(*tile, includeActorOccupancy))
            {
                return false;
            }
        }
    }

    return true;
}

bool Pathing::CanMoveFootprintStep(
    SceneCoordinate from,
    SceneCoordinate to,
    int actorSize,
    bool includeActorOccupancy,
    DiagonalSideFootprintRule diagonalSideFootprintRule) const
{
    if (!m_Scene.Contains(from) || !m_Scene.Contains(to) || from.plane != to.plane)
    {
        return false;
    }

    const int dx = to.x - from.x;
    const int dy = to.y - from.y;

    if (!IsAdjacentStep(dx, dy) ||
        !CanStand(to, actorSize, includeActorOccupancy))
    {
        return false;
    }

    if (IsDiagonalStep(dx, dy))
    {
        return CanMoveFootprintDiagonal(
            from,
            to,
            actorSize,
            includeActorOccupancy,
            diagonalSideFootprintRule);
    }

    return CanMoveFootprintCardinal(from, to, actorSize);
}

bool Pathing::CanMoveFootprintCardinal(
    SceneCoordinate from,
    SceneCoordinate to,
    int actorSize) const
{
    const int dx = to.x - from.x;
    const int dy = to.y - from.y;

    if (dx > 0)
    {
        const int sourceX = from.x + actorSize - 1;
        const int destinationX = to.x + actorSize - 1;

        for (int offsetY = 0; offsetY < actorSize; ++offsetY)
        {
            const Tile* sourceTile =
                m_Scene.TryGetTile({sourceX, from.y + offsetY, from.plane});
            const Tile* destinationTile =
                m_Scene.TryGetTile({destinationX, to.y + offsetY, to.plane});

            if (sourceTile == nullptr || destinationTile == nullptr ||
                sourceTile->HasFlag(TileFlag::BlockMovementEast) ||
                destinationTile->HasFlag(TileFlag::BlockMovementWest))
            {
                return false;
            }
        }

        return true;
    }

    if (dx < 0)
    {
        const int sourceX = from.x;
        const int destinationX = to.x;

        for (int offsetY = 0; offsetY < actorSize; ++offsetY)
        {
            const Tile* sourceTile =
                m_Scene.TryGetTile({sourceX, from.y + offsetY, from.plane});
            const Tile* destinationTile =
                m_Scene.TryGetTile({destinationX, to.y + offsetY, to.plane});

            if (sourceTile == nullptr || destinationTile == nullptr ||
                sourceTile->HasFlag(TileFlag::BlockMovementWest) ||
                destinationTile->HasFlag(TileFlag::BlockMovementEast))
            {
                return false;
            }
        }

        return true;
    }

    if (dy > 0)
    {
        const int sourceY = from.y + actorSize - 1;
        const int destinationY = to.y + actorSize - 1;

        for (int offsetX = 0; offsetX < actorSize; ++offsetX)
        {
            const Tile* sourceTile =
                m_Scene.TryGetTile({from.x + offsetX, sourceY, from.plane});
            const Tile* destinationTile =
                m_Scene.TryGetTile({to.x + offsetX, destinationY, to.plane});

            if (sourceTile == nullptr || destinationTile == nullptr ||
                sourceTile->HasFlag(TileFlag::BlockMovementNorth) ||
                destinationTile->HasFlag(TileFlag::BlockMovementSouth))
            {
                return false;
            }
        }

        return true;
    }

    if (dy < 0)
    {
        const int sourceY = from.y;
        const int destinationY = to.y;

        for (int offsetX = 0; offsetX < actorSize; ++offsetX)
        {
            const Tile* sourceTile =
                m_Scene.TryGetTile({from.x + offsetX, sourceY, from.plane});
            const Tile* destinationTile =
                m_Scene.TryGetTile({to.x + offsetX, destinationY, to.plane});

            if (sourceTile == nullptr || destinationTile == nullptr ||
                sourceTile->HasFlag(TileFlag::BlockMovementSouth) ||
                destinationTile->HasFlag(TileFlag::BlockMovementNorth))
            {
                return false;
            }
        }

        return true;
    }

    return false;
}

bool Pathing::CanMoveFootprintDiagonal(
    SceneCoordinate from,
    SceneCoordinate to,
    int actorSize,
    bool includeActorOccupancy,
    DiagonalSideFootprintRule diagonalSideFootprintRule) const
{
    const int dx = to.x - from.x;
    const int dy = to.y - from.y;
    SceneCoordinate horizontal{from.x + dx, from.y, from.plane};
    SceneCoordinate vertical{from.x, from.y + dy, from.plane};

    if (diagonalSideFootprintRule ==
            DiagonalSideFootprintRule::RequireClear &&
        (!CanStand(horizontal, actorSize, includeActorOccupancy) ||
         !CanStand(vertical, actorSize, includeActorOccupancy)))
    {
        return false;
    }

    for (int offsetX = 0; offsetX < actorSize; ++offsetX)
    {
        for (int offsetY = 0; offsetY < actorSize; ++offsetY)
        {
            const Tile* sourceTile = m_Scene.TryGetTile(
                {from.x + offsetX, from.y + offsetY, from.plane});
            const Tile* destinationTile = m_Scene.TryGetTile(
                {to.x + offsetX, to.y + offsetY, to.plane});

            if (sourceTile == nullptr || destinationTile == nullptr ||
                sourceTile->HasFlag(GetSourceMovementFlag(dx, dy)) ||
                destinationTile->HasFlag(GetDestinationMovementFlag(dx, dy)))
            {
                return false;
            }
        }
    }

    if (diagonalSideFootprintRule ==
        DiagonalSideFootprintRule::AllowBlocked)
    {
        return true;
    }

    return CanMoveFootprintCardinal(from, horizontal, actorSize) &&
           CanMoveFootprintCardinal(from, vertical, actorSize) &&
           CanMoveFootprintCardinal(horizontal, to, actorSize) &&
           CanMoveFootprintCardinal(vertical, to, actorSize);
}

bool Pathing::CanMoveMonotonicRoute(
    SceneCoordinate current,
    SceneCoordinate to,
    int remainingSteps,
    int actorSize,
    bool includeActorOccupancy,
    DiagonalSideFootprintRule diagonalSideFootprintRule) const
{
    if (current == to)
    {
        return true;
    }

    if (remainingSteps <= 0)
    {
        return false;
    }

    const int dx = to.x - current.x;
    const int dy = to.y - current.y;

    if (Abs(dx) > remainingSteps || Abs(dy) > remainingSteps)
    {
        return false;
    }

    const int stepX = Sign(dx);
    const int stepY = Sign(dy);

    SceneCoordinate candidates[3]{
        {current.x + stepX, current.y + stepY, current.plane},
        {current.x + stepX, current.y, current.plane},
        {current.x, current.y + stepY, current.plane}};

    for (const SceneCoordinate& candidate : candidates)
    {
        if (candidate == current)
        {
            continue;
        }

        if (CanMoveFootprintStep(
                current,
                candidate,
                actorSize,
                includeActorOccupancy,
                diagonalSideFootprintRule) &&
            CanMoveMonotonicRoute(
                candidate,
                to,
                remainingSteps - 1,
                actorSize,
                includeActorOccupancy,
                diagonalSideFootprintRule))
        {
            return true;
        }
    }

    return false;
}

bool Pathing::CanMoveCardinal(SceneCoordinate from, SceneCoordinate to) const
{
    const Tile* fromTile = m_Scene.TryGetTile(from);
    const Tile* toTile = m_Scene.TryGetTile(to);

    if (fromTile == nullptr || toTile == nullptr ||
        IsDestinationBlocked(*toTile, true))
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

    if (fromTile == nullptr || toTile == nullptr ||
        IsDestinationBlocked(*toTile, true))
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
