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

struct ActorMovementTargetResult
{
    bool moved = false;
    bool clearTarget = false;
};

struct ActorMovementTargetActor
{
    ActorId id = 0;
    SceneCoordinate coordinate;
    int size = 1;
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
    ActorMovementTargetResult PursueSceneCoordinateTarget(
        ActorMovementAccess& access,
        SceneCoordinate target) const;
    ActorMovementTargetResult PursueActorTarget(
        ActorMovementAccess& access,
        const ActorMovementTargetActor& target,
        ActorId actorId,
        Tick currentTick) const;

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
    static int ClampDelta(int delta, int speed);
    static int GetMovementDeltaForAxis(
        int anchor,
        int size,
        int target,
        int speed);
    static int Sign(int value);
    static int DeterministicCardinalStart(
        ActorId actorId,
        ActorId targetActorId,
        Tick currentTick);
    static TileFlag GetSourceMovementFlag(int dx, int dy);
    static TileFlag GetDestinationMovementFlag(int dx, int dy);
    static bool AreActorFootprintsEdgeAdjacent(
        int moverSize,
        SceneCoordinate moverCoordinate,
        int targetSize,
        SceneCoordinate targetCoordinate);
    static bool AreActorFootprintsOverlapping(
        int moverSize,
        SceneCoordinate moverCoordinate,
        int targetSize,
        SceneCoordinate targetCoordinate);
    static bool AreActorFootprintsCornerContact(
        int moverSize,
        SceneCoordinate moverCoordinate,
        int targetSize,
        SceneCoordinate targetCoordinate);

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
    bool IsFinalNpcOccupancyOnlyBlock(
        const ActorMovementAccess& access,
        SceneCoordinate current,
        SceneCoordinate destination) const;
    bool HasNpcDiagonalSideOccupancyConflict(
        SceneCoordinate current,
        SceneCoordinate destination,
        int actorSize) const;
    bool TryGetActorTargetEdgeAdjacentMovementDelta(
        ActorMovementKind kind,
        SceneCoordinate current,
        int actorSize,
        int actorSpeed,
        int requestedDx,
        int requestedDy,
        const ActorMovementTargetActor& target,
        int& edgeAdjacentDx,
        int& edgeAdjacentDy) const;
    bool TryGetActorTargetOverlapEscapeMovementDelta(
        ActorMovementKind kind,
        SceneCoordinate current,
        int actorSize,
        ActorId actorId,
        ActorId targetActorId,
        Tick currentTick,
        int& escapeDx,
        int& escapeDy) const;
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
