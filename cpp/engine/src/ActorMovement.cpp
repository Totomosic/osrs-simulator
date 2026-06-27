#include "ActorMovement.h"

namespace osrssim
{
namespace
{
int Abs(int value)
{
    return value < 0 ? -value : value;
}

int Max(int lhs, int rhs)
{
    return lhs > rhs ? lhs : rhs;
}

int RemainingChebyshev(int requestedDx, int requestedDy, int dx, int dy)
{
    return Max(Abs(requestedDx - dx), Abs(requestedDy - dy));
}

int RemainingManhattan(int requestedDx, int requestedDy, int dx, int dy)
{
    return Abs(requestedDx - dx) + Abs(requestedDy - dy);
}

int RatioError(int requestedDx, int requestedDy, int dx, int dy)
{
    return Abs(Abs(dx) * Abs(requestedDy) - Abs(dy) * Abs(requestedDx));
}

int CardinalPriority(int dx, int dy)
{
    if (dx < 0 && dy == 0)
    {
        return 0;
    }

    if (dx > 0 && dy == 0)
    {
        return 1;
    }

    if (dx == 0 && dy < 0)
    {
        return 2;
    }

    if (dx == 0 && dy > 0)
    {
        return 3;
    }

    return 4;
}
}  // namespace

ActorMovementAccess::ActorMovementAccess(
    Scene& scene,
    SceneCoordinate& coordinate,
    ActorMovementKind kind,
    int size,
    int speed)
    : m_Scene(scene),
      m_Coordinate(coordinate),
      m_Kind(kind),
      m_Size(size),
      m_Speed(speed)
{
}

const Scene& ActorMovementAccess::GetScene() const
{
    return m_Scene;
}

ActorMovementKind ActorMovementAccess::GetKind() const
{
    return m_Kind;
}

SceneCoordinate ActorMovementAccess::GetCoordinate() const
{
    return m_Coordinate;
}

int ActorMovementAccess::GetSize() const
{
    return m_Size;
}

int ActorMovementAccess::GetSpeed() const
{
    return m_Speed;
}

void ActorMovementAccess::MoveTo(SceneCoordinate destination)
{
    RemoveOccupancy(m_Coordinate);
    m_Coordinate = destination;
    AddOccupancy(m_Coordinate);
}

void ActorMovementAccess::AddOccupancy(SceneCoordinate coordinate)
{
    for (int offsetX = 0; offsetX < m_Size; ++offsetX)
    {
        for (int offsetY = 0; offsetY < m_Size; ++offsetY)
        {
            Tile* tile = m_Scene.TryGetTile(
                {coordinate.x + offsetX,
                 coordinate.y + offsetY,
                 coordinate.plane});

            if (tile != nullptr)
            {
                tile->AddFlag(TileFlag::Occupied);
            }
        }
    }
}

void ActorMovementAccess::RemoveOccupancy(SceneCoordinate coordinate)
{
    for (int offsetX = 0; offsetX < m_Size; ++offsetX)
    {
        for (int offsetY = 0; offsetY < m_Size; ++offsetY)
        {
            Tile* tile = m_Scene.TryGetTile(
                {coordinate.x + offsetX,
                 coordinate.y + offsetY,
                 coordinate.plane});

            if (tile != nullptr)
            {
                tile->RemoveFlag(TileFlag::Occupied);
            }
        }
    }
}

ActorMovement::ActorMovement(const Scene& scene)
    : m_Scene(scene)
{
}

bool ActorMovement::CanMove(SceneCoordinate from, SceneCoordinate to) const
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

bool ActorMovement::CanMove(
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

bool ActorMovement::CanMoveIgnoringActorOccupancy(
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

bool ActorMovement::CanMoveIgnoringActorOccupancy(
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

ActorMovementResult ActorMovement::MoveByDelta(
    ActorMovementAccess& access,
    int requestedDx,
    int requestedDy) const
{
    if (&access.GetScene() != &m_Scene)
    {
        return {};
    }

    int resolvedDx = 0;
    int resolvedDy = 0;
    const SceneCoordinate current = access.GetCoordinate();

    if (!TryResolveMovementDelta(
            access.GetKind(),
            current,
            access.GetSize(),
            access.GetSpeed(),
            requestedDx,
            requestedDy,
            resolvedDx,
            resolvedDy))
    {
        return {};
    }

    access.MoveTo(
        {current.x + resolvedDx, current.y + resolvedDy, current.plane});

    return {true};
}

ActorMovementTargetResult ActorMovement::PursueSceneCoordinateTarget(
    ActorMovementAccess& access,
    SceneCoordinate target) const
{
    if (&access.GetScene() != &m_Scene)
    {
        return {false, true};
    }

    const SceneCoordinate current = access.GetCoordinate();

    if (!m_Scene.Contains(target) || target.plane != current.plane)
    {
        return {false, true};
    }

    if (target.x == current.x && target.y == current.y)
    {
        return {false, true};
    }

    const int dx =
        GetMovementDeltaForAxis(current.x, 1, target.x, access.GetSpeed());
    const int dy =
        GetMovementDeltaForAxis(current.y, 1, target.y, access.GetSpeed());
    const ActorMovementResult movementResult = MoveByDelta(access, dx, dy);

    if (!movementResult.moved)
    {
        const SceneCoordinate attemptedDestination{
            current.x + dx,
            current.y + dy,
            current.plane};
        const bool keepDynamicNpcTarget = IsFinalNpcOccupancyOnlyBlock(
            access,
            current,
            attemptedDestination);

        return {false, !keepDynamicNpcTarget};
    }

    const SceneCoordinate updated = access.GetCoordinate();
    const bool reachedTarget =
        target.plane == updated.plane && target.x == updated.x &&
        target.y == updated.y;

    return {true, reachedTarget};
}

bool ActorMovement::IsAdjacentStep(int dx, int dy)
{
    return Abs(dx) <= 1 && Abs(dy) <= 1 && (dx != 0 || dy != 0);
}

bool ActorMovement::IsDiagonalStep(int dx, int dy)
{
    return dx != 0 && dy != 0;
}

bool ActorMovement::IsDestinationBlocked(const Tile& tile, bool includeActorOccupancy)
{
    return (includeActorOccupancy && tile.HasFlag(TileFlag::Occupied)) ||
           tile.HasFlag(TileFlag::BlockMovementFull) ||
           tile.HasFlag(TileFlag::BlockMovementObject) ||
           tile.HasFlag(TileFlag::BlockMovementFloor) ||
           tile.HasFlag(TileFlag::BlockMovementFloorDecoration);
}

bool ActorMovement::CanUseLargeNpcDiagonalSqueeze(
    ActorMovementKind kind,
    int actorSize)
{
    return kind == ActorMovementKind::Npc && actorSize > 1;
}

DiagonalSideFootprintRule ActorMovement::GetDiagonalSideFootprintRule(
    ActorMovementKind kind,
    int actorSize)
{
    return CanUseLargeNpcDiagonalSqueeze(kind, actorSize)
               ? DiagonalSideFootprintRule::AllowBlocked
               : DiagonalSideFootprintRule::RequireClear;
}

bool ActorMovement::IsBetterResolvedDelta(
    int requestedDx,
    int requestedDy,
    int candidateDx,
    int candidateDy,
    int bestDx,
    int bestDy)
{
    const int candidateRemaining =
        RemainingChebyshev(requestedDx, requestedDy, candidateDx, candidateDy);
    const int bestRemaining =
        RemainingChebyshev(requestedDx, requestedDy, bestDx, bestDy);

    if (candidateRemaining != bestRemaining)
    {
        return candidateRemaining < bestRemaining;
    }

    const bool requestUsesBothAxes = requestedDx != 0 && requestedDy != 0;
    const bool candidateUsesBothAxes = candidateDx != 0 && candidateDy != 0;
    const bool bestUsesBothAxes = bestDx != 0 && bestDy != 0;

    if (requestUsesBothAxes && candidateUsesBothAxes != bestUsesBothAxes)
    {
        return candidateUsesBothAxes;
    }

    if (requestUsesBothAxes && candidateUsesBothAxes && bestUsesBothAxes)
    {
        const int candidateRatioError =
            RatioError(requestedDx, requestedDy, candidateDx, candidateDy);
        const int bestRatioError =
            RatioError(requestedDx, requestedDy, bestDx, bestDy);

        if (candidateRatioError != bestRatioError)
        {
            return candidateRatioError < bestRatioError;
        }
    }

    const int candidateManhattan =
        RemainingManhattan(requestedDx, requestedDy, candidateDx, candidateDy);
    const int bestManhattan =
        RemainingManhattan(requestedDx, requestedDy, bestDx, bestDy);

    if (candidateManhattan != bestManhattan)
    {
        return candidateManhattan < bestManhattan;
    }

    const int candidateCardinalPriority =
        CardinalPriority(candidateDx, candidateDy);
    const int bestCardinalPriority = CardinalPriority(bestDx, bestDy);

    if (candidateCardinalPriority != bestCardinalPriority)
    {
        return candidateCardinalPriority < bestCardinalPriority;
    }

    return Abs(candidateDx) + Abs(candidateDy) > Abs(bestDx) + Abs(bestDy);
}

int ActorMovement::ClampDelta(int delta, int speed)
{
    if (delta > speed)
    {
        return speed;
    }

    if (delta < -speed)
    {
        return -speed;
    }

    return delta;
}

int ActorMovement::GetMovementDeltaForAxis(
    int anchor,
    int size,
    int target,
    int speed)
{
    if (target < anchor)
    {
        return ClampDelta(target - anchor, speed);
    }

    if (target >= anchor + size)
    {
        return ClampDelta(target - (anchor + size - 1), speed);
    }

    return 0;
}

int ActorMovement::Sign(int value)
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

TileFlag ActorMovement::GetSourceMovementFlag(int dx, int dy)
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

TileFlag ActorMovement::GetDestinationMovementFlag(int dx, int dy)
{
    return GetSourceMovementFlag(-dx, -dy);
}

bool ActorMovement::CanStand(
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

bool ActorMovement::CanMoveFootprintStep(
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

bool ActorMovement::CanMoveFootprintCardinal(
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

bool ActorMovement::CanMoveFootprintDiagonal(
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

bool ActorMovement::CanMoveMonotonicRoute(
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

bool ActorMovement::HasFinalNpcOccupancyConflict(
    SceneCoordinate current,
    SceneCoordinate destination,
    int actorSize) const
{
    for (int offsetX = 0; offsetX < actorSize; ++offsetX)
    {
        for (int offsetY = 0; offsetY < actorSize; ++offsetY)
        {
            SceneCoordinate coordinate{
                destination.x + offsetX,
                destination.y + offsetY,
                destination.plane};
            const Tile* tile = m_Scene.TryGetTile(coordinate);

            if (tile == nullptr)
            {
                return true;
            }

            const bool isCurrentFootprint =
                coordinate.plane == current.plane &&
                coordinate.x >= current.x &&
                coordinate.x < current.x + actorSize &&
                coordinate.y >= current.y &&
                coordinate.y < current.y + actorSize;

            if (!isCurrentFootprint && tile->IsOccupied())
            {
                return true;
            }
        }
    }

    return false;
}

bool ActorMovement::IsFinalNpcOccupancyOnlyBlock(
    const ActorMovementAccess& access,
    SceneCoordinate current,
    SceneCoordinate destination) const
{
    if (access.GetKind() != ActorMovementKind::Npc)
    {
        return false;
    }

    const DiagonalSideFootprintRule diagonalSideFootprintRule =
        GetDiagonalSideFootprintRule(access.GetKind(), access.GetSize());

    return CanMoveIgnoringActorOccupancy(
               current,
               destination,
               access.GetSpeed(),
               access.GetSize(),
               diagonalSideFootprintRule) &&
           HasFinalNpcOccupancyConflict(
               current,
               destination,
               access.GetSize());
}

bool ActorMovement::HasNpcDiagonalSideOccupancyConflict(
    SceneCoordinate current,
    SceneCoordinate destination,
    int actorSize) const
{
    const int dx = Sign(destination.x - current.x);
    const int dy = Sign(destination.y - current.y);

    if (dx == 0 || dy == 0)
    {
        return false;
    }

    SceneCoordinate horizontal{current.x + dx, current.y, current.plane};
    SceneCoordinate vertical{current.x, current.y + dy, current.plane};

    return HasFinalNpcOccupancyConflict(current, horizontal, actorSize) ||
           HasFinalNpcOccupancyConflict(current, vertical, actorSize);
}

bool ActorMovement::TryResolveMovementDelta(
    ActorMovementKind kind,
    SceneCoordinate current,
    int actorSize,
    int actorSpeed,
    int requestedDx,
    int requestedDy,
    int& resolvedDx,
    int& resolvedDy) const
{
    if ((requestedDx == 0 && requestedDy == 0) ||
        Abs(requestedDx) > actorSpeed || Abs(requestedDy) > actorSpeed)
    {
        return false;
    }

    const int stepX = Sign(requestedDx);
    const int stepY = Sign(requestedDy);
    const int absRequestedDx = Abs(requestedDx);
    const int absRequestedDy = Abs(requestedDy);
    const DiagonalSideFootprintRule diagonalSideFootprintRule =
        GetDiagonalSideFootprintRule(kind, actorSize);
    const bool canUseLargeNpcDiagonalSqueeze =
        CanUseLargeNpcDiagonalSqueeze(kind, actorSize);
    bool found = false;
    int bestDx = 0;
    int bestDy = 0;

    for (int candidateAbsDx = 0; candidateAbsDx <= absRequestedDx;
         ++candidateAbsDx)
    {
        for (int candidateAbsDy = 0; candidateAbsDy <= absRequestedDy;
             ++candidateAbsDy)
        {
            const int candidateDx = candidateAbsDx * stepX;
            const int candidateDy = candidateAbsDy * stepY;

            if (candidateDx == 0 && candidateDy == 0)
            {
                continue;
            }

            SceneCoordinate candidate{
                current.x + candidateDx,
                current.y + candidateDy,
                current.plane};

            if (!CanMoveIgnoringActorOccupancy(
                    current,
                    candidate,
                    actorSpeed,
                    actorSize,
                    diagonalSideFootprintRule))
            {
                continue;
            }

            if (kind == ActorMovementKind::Npc &&
                !canUseLargeNpcDiagonalSqueeze &&
                HasNpcDiagonalSideOccupancyConflict(
                    current,
                    candidate,
                    actorSize))
            {
                continue;
            }

            if (kind == ActorMovementKind::Npc &&
                HasFinalNpcOccupancyConflict(current, candidate, actorSize))
            {
                continue;
            }

            if (!found || IsBetterResolvedDelta(
                              requestedDx,
                              requestedDy,
                              candidateDx,
                              candidateDy,
                              bestDx,
                              bestDy))
            {
                found = true;
                bestDx = candidateDx;
                bestDy = candidateDy;
            }
        }
    }

    if (!found)
    {
        return false;
    }

    resolvedDx = bestDx;
    resolvedDy = bestDy;

    return true;
}

bool ActorMovement::CanMoveCardinal(SceneCoordinate from, SceneCoordinate to) const
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

bool ActorMovement::CanMoveDiagonal(SceneCoordinate from, SceneCoordinate to) const
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
