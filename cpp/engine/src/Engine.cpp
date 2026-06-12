#include "Engine.h"

#include <algorithm>

namespace osrssim
{

bool Engine::QueuePlayerMoveToSceneCoordinate(
    ActorId actorId,
    SceneCoordinate coordinate)
{
    if (m_World.GetPlayer(actorId) == nullptr ||
        !m_World.CanPlayerUseSceneCoordinateMovementTarget(actorId, coordinate))
    {
        return false;
    }

    m_QueuedPlayerMovementActions.push_back({actorId, coordinate});

    return true;
}

void Engine::Step()
{
    ++m_CurrentTick;
    ProcessQueuedPlayerMovementActions();
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
        m_World.SetPlayerSceneCoordinateMovementTarget(
            action.actorId,
            action.coordinate);
    }

    m_QueuedPlayerMovementActions.clear();
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
