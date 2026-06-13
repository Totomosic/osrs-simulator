#include "Engine.h"

#include <algorithm>

namespace osrssim
{

bool Engine::SetPlayerSceneCoordinateMovementTarget(
    ActorId actorId,
    SceneCoordinate coordinate)
{
    return m_World.SetPlayerSceneCoordinateMovementTarget(actorId, coordinate);
}

bool Engine::QueuePlayerMoveToSceneCoordinate(
    ActorId actorId,
    SceneCoordinate coordinate)
{
    if (m_World.GetPlayer(actorId) == nullptr)
    {
        return false;
    }

    m_QueuedPlayerMovementActions.push_back(
        {actorId, {MovementTargetKind::SceneCoordinate, coordinate, 0}});

    return true;
}

bool Engine::QueuePlayerMoveToActor(ActorId actorId, ActorId targetActorId)
{
    if (m_World.GetPlayer(actorId) == nullptr ||
        m_World.GetActorCore(targetActorId) == nullptr ||
        actorId == targetActorId)
    {
        return false;
    }

    m_QueuedPlayerMovementActions.push_back(
        {actorId, {MovementTargetKind::Actor, {}, targetActorId}});

    return true;
}

void Engine::Step()
{
    ++m_CurrentTick;
    ProcessQueuedPlayerMovementActions();
    UpdateNpcs();
    UpdatePlayers();
}

Tick Engine::GetCurrentTick() const
{
    return m_CurrentTick;
}

World& Engine::GetWorld()
{
    return m_World;
}

const World& Engine::GetWorld() const
{
    return m_World;
}

void Engine::ProcessQueuedPlayerMovementActions()
{
    for (const QueuedPlayerMovementAction& action :
         m_QueuedPlayerMovementActions)
    {
        if (action.movementTarget.kind == MovementTargetKind::SceneCoordinate)
        {
            m_World.SetActorSceneCoordinateMovementTarget(
                action.actorId,
                action.movementTarget.sceneCoordinate);
        }
        else
        {
            m_World.SetActorMovementTarget(
                action.actorId,
                action.movementTarget.actorId);
        }
    }

    m_QueuedPlayerMovementActions.clear();
}

void Engine::UpdateNpcs()
{
    std::vector<ActorId> npcIds;
    npcIds.reserve(m_World.GetNpcs().size());

    for (const auto& [actorId, npc] : m_World.GetNpcs())
    {
        npcIds.push_back(actorId);
    }

    std::sort(npcIds.begin(), npcIds.end());

    for (ActorId actorId : npcIds)
    {
        m_World.UpdateActorMovement(actorId);
    }
}

void Engine::UpdatePlayers()
{
    std::vector<ActorId> playerIds;
    playerIds.reserve(m_World.GetPlayers().size());

    for (const auto& [actorId, player] : m_World.GetPlayers())
    {
        playerIds.push_back(actorId);
    }

    std::sort(playerIds.begin(), playerIds.end());

    for (ActorId actorId : playerIds)
    {
        m_World.UpdatePlayerMovement(actorId);
    }
}

}  // namespace osrssim
