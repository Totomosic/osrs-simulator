#pragma once

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
    std::vector<QueuedPlayerMovementAction> m_QueuedPlayerMovementActions;

public:
    bool QueuePlayerMoveToSceneCoordinate(
        ActorId actorId,
        SceneCoordinate coordinate);
    bool QueuePlayerMoveToActor(ActorId actorId, ActorId targetActorId);
    void Step();
    Tick GetCurrentTick() const;
    World& GetWorld();
    const World& GetWorld() const;

private:
    void ProcessQueuedPlayerMovementActions();
    void UpdatePlayers();
};

}  // namespace osrssim
