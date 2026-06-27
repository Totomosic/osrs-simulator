#pragma once

#include "CombatQueue.h"
#include "DpsService.h"
#include "ActorMovement.h"
#include "Scene.h"
#include "Types.h"

#include <functional>
#include <optional>
#include <set>
#include <unordered_map>
#include <vector>

namespace osrssim
{

struct ActorCore
{
    ActorId id = 0;
    int size = 1;
    int speed = 0;
    CombatComposition combatComposition;
    CombatQueue combatQueue;
    int attackTimer = 0;
};

enum class MovementTargetKind
{
    SceneCoordinate,
    Actor,
};

struct MovementTarget
{
    MovementTargetKind kind = MovementTargetKind::SceneCoordinate;
    SceneCoordinate sceneCoordinate;
    ActorId actorId = 0;
};

struct Player
{
    ActorCore actor;
    PlayerIndex playerIndex = 0;
    std::optional<MovementTarget> movementTarget;
};

struct Npc
{
    ActorCore actor;
    NpcIndex npcIndex = 0;
    NpcBehaviorId behaviorId = 0;
    std::optional<MovementTarget> movementTarget;
};

struct SceneMembership
{
    SceneId sceneId = 0;
    SceneCoordinate coordinate;
};

class World
{
private:
    SceneId m_DefaultSceneId = 1;
    ActorId m_NextActorId = 1;
    int m_NextPlayerIndex = 0;
    int m_NextNpcIndex = 0;
    Scene m_Scene;
    std::unordered_map<ActorId, Player> m_Players;
    std::unordered_map<ActorId, Npc> m_Npcs;
    std::set<PlayerIndex> m_LivePlayerIndices;
    std::set<NpcIndex> m_LiveNpcIndices;
    std::unordered_map<PlayerIndex, ActorId> m_PlayerActorsByIndex;
    std::unordered_map<NpcIndex, ActorId> m_NpcActorsByIndex;
    std::unordered_map<ActorId, SceneMembership> m_SceneMemberships;
    std::vector<ActorId> m_QueuedActorRemovals;
    std::optional<Tick> m_CurrentTick;

public:
    void SetCurrentTick(Tick currentTick);
    std::optional<Tick> GetCurrentTick() const;
    SceneId GetDefaultSceneId() const;
    Scene* TryGetScene(SceneId sceneId);
    const Scene* TryGetScene(SceneId sceneId) const;
    std::optional<ActorId> CreatePlayer(
        int size,
        int speed,
        CombatComposition combatComposition);
    std::optional<ActorId> CreateNpc(
        int size,
        int speed,
        CombatComposition combatComposition);
    const std::unordered_map<ActorId, Player>& GetPlayers() const;
    const std::unordered_map<ActorId, Npc>& GetNpcs() const;
    const std::unordered_map<ActorId, SceneMembership>& GetSceneMemberships() const;
    const Player* GetPlayer(ActorId actorId) const;
    const Npc* GetNpc(ActorId actorId) const;
    std::optional<ActorId> GetNextPlayerActorIdAfterIndex(
        int playerIndex) const;
    std::optional<ActorId> GetNextNpcActorIdAfterIndex(int npcIndex) const;
    const ActorCore* GetActorCore(ActorId actorId) const;
    const SceneMembership* GetSceneMembership(ActorId actorId) const;
    bool AreActorFootprintsOverlapping(
        ActorId firstActorId,
        ActorId secondActorId) const;
    const CombatComposition* GetActorCombatComposition(ActorId actorId) const;
    CombatQueue* GetActorCombatQueue(ActorId actorId);
    const CombatQueue* GetActorCombatQueue(ActorId actorId) const;
    bool QueueActorCombatEvent(
        ActorId actorId,
        int ticksRemaining,
        std::function<void()> callback);
    bool QueueActorCombatEvent(
        ActorId actorId,
        int ticksRemaining,
        std::function<void()> callback,
        ProjectileMetadata projectile);
    std::vector<ProjectileSnapshot> GetProjectileSnapshots() const;
    ScenePosition GetActorFootprintCenter(ActorId actorId) const;
    bool SetActorCombatComposition(
        ActorId actorId,
        CombatComposition combatComposition);
    int GetActorAttackTimer(ActorId actorId) const;
    bool SetActorAttackTimer(ActorId actorId, int attackTimer);
    bool SetActorSpeed(ActorId actorId, int speed);
    bool SetNpcBehaviorId(ActorId actorId, NpcBehaviorId behaviorId);
    bool PlaceActor(ActorId actorId, SceneId sceneId, SceneCoordinate coordinate);
    bool RemoveActorSceneMembership(ActorId actorId);
    bool RemoveActor(ActorId actorId);
    bool QueueActorRemoval(ActorId actorId);
    std::vector<ActorId> TakeQueuedActorRemovals();
    void FlushQueuedActorRemovals();
    bool MoveActorByDelta(ActorId actorId, int dx, int dy);
    bool CanActorUseSceneCoordinateMovementTarget(
        ActorId actorId,
        SceneCoordinate coordinate) const;
    bool CanPlayerUseSceneCoordinateMovementTarget(
        ActorId actorId,
        SceneCoordinate coordinate) const;
    bool SetActorSceneCoordinateMovementTarget(
        ActorId actorId,
        SceneCoordinate coordinate);
    bool SetPlayerSceneCoordinateMovementTarget(
        ActorId actorId,
        SceneCoordinate coordinate);
    bool ClearActorMovementTarget(ActorId actorId);
    bool SetActorMovementTarget(ActorId actorId, ActorId targetActorId);
    bool UpdateActorMovement(ActorId actorId, Tick currentTick = 0);
    bool UpdatePlayerMovement(ActorId actorId, Tick currentTick = 0);

private:
    enum class ActorKind
    {
        Player,
        Npc,
    };

    static int ClampSize(int size);
    static int ClampSpeed(int speed);
    static int ClampDelta(int delta, int speed);
    static bool IsWholeTileMovementBlocked(const Tile& tile);
    static bool CanUseLargeNpcDiagonalSqueeze(
        ActorKind actorKind,
        const ActorCore& actor);
    static DiagonalSideFootprintRule GetDiagonalSideFootprintRule(
        ActorKind actorKind,
        const ActorCore& actor);

    Player* TryGetPlayer(ActorId actorId);
    const Player* TryGetPlayer(ActorId actorId) const;
    std::optional<PlayerIndex> AllocatePlayerIndex();
    std::optional<NpcIndex> AllocateNpcIndex();
    ActorCore* TryGetActorCore(ActorId actorId);
    const ActorCore* TryGetActorCore(ActorId actorId) const;
    std::optional<MovementTarget>* TryGetMovementTarget(ActorId actorId);
    const std::optional<MovementTarget>* TryGetMovementTarget(
        ActorId actorId) const;
    ActorKind GetActorKind(ActorId actorId) const;
    bool HasActor(ActorId actorId) const;
    bool CanUseSceneCoordinateMovementTarget(
        const Scene& scene,
        SceneCoordinate coordinate,
        int actorSize) const;
    bool IsActorAtSceneCoordinateMovementTarget(
        SceneCoordinate actorCoordinate,
        SceneCoordinate target) const;
    bool AreActorFootprintsEdgeAdjacent(
        const ActorCore& mover,
        SceneCoordinate moverCoordinate,
        const ActorCore& target,
        SceneCoordinate targetCoordinate) const;
    bool AreActorFootprintsOverlapping(
        const ActorCore& mover,
        SceneCoordinate moverCoordinate,
        const ActorCore& target,
        SceneCoordinate targetCoordinate) const;
    bool AreActorFootprintsCornerContact(
        const ActorCore& mover,
        SceneCoordinate moverCoordinate,
        const ActorCore& target,
        SceneCoordinate targetCoordinate) const;
    int GetMovementDeltaForAxis(int anchor, int size, int target, int speed) const;
    bool HasNpcDiagonalSideOccupancyConflict(
        const Scene& scene,
        SceneCoordinate current,
        SceneCoordinate destination,
        int actorSize) const;
    bool TryGetActorTargetEdgeAdjacentMovementDelta(
        const Scene& scene,
        ActorKind actorKind,
        const ActorCore& actor,
        SceneCoordinate current,
        int requestedDx,
        int requestedDy,
        const ActorCore& target,
        SceneCoordinate targetCoordinate,
        int& edgeAdjacentDx,
        int& edgeAdjacentDy) const;
    bool TryGetActorTargetOverlapEscapeMovementDelta(
        const Scene& scene,
        ActorKind actorKind,
        const ActorCore& actor,
        SceneCoordinate current,
        ActorId targetActorId,
        Tick currentTick,
        int& escapeDx,
        int& escapeDy) const;
    bool CanStandOnMovementBlockers(
        const Scene& scene,
        SceneCoordinate coordinate,
        int actorSize) const;
    bool HasFinalNpcOccupancyConflict(
        const Scene& scene,
        SceneCoordinate current,
        SceneCoordinate destination,
        int actorSize) const;
    bool IsFinalNpcOccupancyOnlyBlock(
        const Scene& scene,
        const ActorCore& actor,
        SceneCoordinate current,
        SceneCoordinate destination) const;
    bool TryResolveMovementDelta(
        const Scene& scene,
        ActorKind actorKind,
        const ActorCore& actor,
        SceneCoordinate current,
        int requestedDx,
        int requestedDy,
        int& resolvedDx,
        int& resolvedDy) const;
    void UpdateProjectilesTargetingActor(ActorId actorId);
    void AddActorOccupancy(
        Scene& scene,
        SceneCoordinate coordinate,
        int actorSize);
    void RemoveActorOccupancy(
        Scene& scene,
        SceneCoordinate coordinate,
        int actorSize);
};

}  // namespace osrssim
