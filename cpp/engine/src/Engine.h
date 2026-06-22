#pragma once

#include "CombatService.h"
#include "Types.h"
#include "World.h"

#include <vector>

namespace osrssim
{

class Engine
{
private:
    struct QueuedPlayerMovementAction
    {
        ActorId actorId = 0;
        MovementTarget movementTarget;
    };

    Tick m_CurrentTick = 0;
    World m_World;
    CombatService m_CombatService;
    std::vector<QueuedPlayerMovementAction> m_QueuedPlayerMovementActions;

public:
    bool SetPlayerSceneCoordinateMovementTarget(
        ActorId actorId,
        SceneCoordinate coordinate);
    bool QueuePlayerMoveToSceneCoordinate(
        ActorId actorId,
        SceneCoordinate coordinate);
    bool QueuePlayerMoveToActor(ActorId actorId, ActorId targetActorId);
    void Step();
    Tick GetCurrentTick() const;
    World& GetWorld();
    const World& GetWorld() const;
    CombatService& GetCombatService();
    const CombatService& GetCombatService() const;

private:
    void ProcessQueuedPlayerMovementActions();
    void DecrementAttackTimers();
    void ProcessActorCombatQueue(ActorId actorId);
    bool TryHandleActorTargetCombat(ActorId actorId);
    bool IsOverlappingActorMovementTarget(ActorId actorId) const;
    void UpdateNpcs();
    void UpdatePlayers();
};

}  // namespace osrssim
