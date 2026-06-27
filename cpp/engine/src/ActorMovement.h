#pragma once

#include "Scene.h"
#include "Types.h"

namespace osrssim
{

enum class DiagonalSideFootprintRule
{
    RequireClear,
    AllowBlocked,
};

enum class ActorMovementKind
{
    Player,
    Npc,
};

struct ActorMovementResult
{
    bool moved = false;
};

class ActorMovementAccess
{
private:
    Scene& m_Scene;
    SceneCoordinate& m_Coordinate;
    ActorMovementKind m_Kind;
    int m_Size = 1;
    int m_Speed = 0;

public:
    ActorMovementAccess(
        Scene& scene,
        SceneCoordinate& coordinate,
        ActorMovementKind kind,
        int size,
        int speed);

    const Scene& GetScene() const;
    ActorMovementKind GetKind() const;
    SceneCoordinate GetCoordinate() const;
    int GetSize() const;
    int GetSpeed() const;
    void MoveTo(SceneCoordinate destination);

private:
    void AddOccupancy(SceneCoordinate coordinate);
    void RemoveOccupancy(SceneCoordinate coordinate);
};

class ActorMovement
{
private:
    const Scene& m_Scene;

public:
    explicit ActorMovement(const Scene& scene);

    bool CanMove(SceneCoordinate from, SceneCoordinate to) const;
    bool CanMove(
        SceneCoordinate from,
        SceneCoordinate to,
        int actorSpeed,
        int actorSize) const;
    bool CanMoveIgnoringActorOccupancy(
        SceneCoordinate from,
        SceneCoordinate to,
        int actorSpeed,
        int actorSize) const;
    bool CanMoveIgnoringActorOccupancy(
        SceneCoordinate from,
        SceneCoordinate to,
        int actorSpeed,
        int actorSize,
        DiagonalSideFootprintRule diagonalSideFootprintRule) const;
    ActorMovementResult MoveByDelta(
        ActorMovementAccess& access,
        int requestedDx,
        int requestedDy) const;

private:
    static bool IsAdjacentStep(int dx, int dy);
    static bool IsDiagonalStep(int dx, int dy);
    static bool IsDestinationBlocked(const Tile& tile, bool includeActorOccupancy);
    static bool CanUseLargeNpcDiagonalSqueeze(
        ActorMovementKind kind,
        int actorSize);
    static DiagonalSideFootprintRule GetDiagonalSideFootprintRule(
        ActorMovementKind kind,
        int actorSize);
    static bool IsBetterResolvedDelta(
        int requestedDx,
        int requestedDy,
        int candidateDx,
        int candidateDy,
        int bestDx,
        int bestDy);
    static int Sign(int value);
    static TileFlag GetSourceMovementFlag(int dx, int dy);
    static TileFlag GetDestinationMovementFlag(int dx, int dy);

    bool CanStand(
        SceneCoordinate anchor,
        int actorSize,
        bool includeActorOccupancy) const;
    bool CanMoveFootprintStep(
        SceneCoordinate from,
        SceneCoordinate to,
        int actorSize,
        bool includeActorOccupancy,
        DiagonalSideFootprintRule diagonalSideFootprintRule) const;
    bool CanMoveFootprintCardinal(
        SceneCoordinate from,
        SceneCoordinate to,
        int actorSize) const;
    bool CanMoveFootprintDiagonal(
        SceneCoordinate from,
        SceneCoordinate to,
        int actorSize,
        bool includeActorOccupancy,
        DiagonalSideFootprintRule diagonalSideFootprintRule) const;
    bool CanMoveMonotonicRoute(
        SceneCoordinate current,
        SceneCoordinate to,
        int remainingSteps,
        int actorSize,
        bool includeActorOccupancy,
        DiagonalSideFootprintRule diagonalSideFootprintRule) const;
    bool HasFinalNpcOccupancyConflict(
        SceneCoordinate current,
        SceneCoordinate destination,
        int actorSize) const;
    bool HasNpcDiagonalSideOccupancyConflict(
        SceneCoordinate current,
        SceneCoordinate destination,
        int actorSize) const;
    bool TryResolveMovementDelta(
        ActorMovementKind kind,
        SceneCoordinate current,
        int actorSize,
        int actorSpeed,
        int requestedDx,
        int requestedDy,
        int& resolvedDx,
        int& resolvedDy) const;
    bool CanMoveCardinal(SceneCoordinate from, SceneCoordinate to) const;
    bool CanMoveDiagonal(SceneCoordinate from, SceneCoordinate to) const;
};

}  // namespace osrssim
