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
    DecrementAttackTimers();
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

CombatService& Engine::GetCombatService()
{
    return m_CombatService;
}

const CombatService& Engine::GetCombatService() const
{
    return m_CombatService;
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

void Engine::DecrementAttackTimers()
{
    m_CombatService.DecrementAttackTimers(m_World);
}

void Engine::ProcessActorCombatQueue(ActorId actorId)
{
    CombatQueue* combatQueue = m_World.GetActorCombatQueue(actorId);

    if (combatQueue != nullptr)
    {
        combatQueue->Process();
    }
}

bool Engine::TryHandleActorTargetCombat(ActorId actorId)
{
    const Player* player = m_World.GetPlayer(actorId);
    const Npc* npc = m_World.GetNpc(actorId);
    const MovementTarget* movementTarget = nullptr;

    if (player != nullptr && player->movementTarget.has_value())
    {
        movementTarget = &player->movementTarget.value();
    }
    else if (npc != nullptr && npc->movementTarget.has_value())
    {
        movementTarget = &npc->movementTarget.value();
    }

    if (movementTarget == nullptr ||
        movementTarget->kind != MovementTargetKind::Actor ||
        !m_CombatService.CanAttackActorTarget(
            m_World,
            actorId,
            movementTarget->actorId))
    {
        return false;
    }

    if (m_World.GetActorAttackTimer(actorId) <= 0)
    {
        m_CombatService.DispatchAttack(
            m_World,
            actorId,
            movementTarget->actorId,
            m_CurrentTick);
    }

    return true;
}

bool Engine::IsOverlappingActorMovementTarget(ActorId actorId) const
{
    const Player* player = m_World.GetPlayer(actorId);
    const Npc* npc = m_World.GetNpc(actorId);
    const MovementTarget* movementTarget = nullptr;

    if (player != nullptr && player->movementTarget.has_value())
    {
        movementTarget = &player->movementTarget.value();
    }
    else if (npc != nullptr && npc->movementTarget.has_value())
    {
        movementTarget = &npc->movementTarget.value();
    }

    return movementTarget != nullptr &&
           movementTarget->kind == MovementTargetKind::Actor &&
           m_World.AreActorFootprintsOverlapping(
               actorId,
               movementTarget->actorId);
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
        ProcessActorCombatQueue(actorId);

        if (m_World.GetActorCore(actorId) == nullptr)
        {
            continue;
        }

        if (!TryHandleActorTargetCombat(actorId))
        {
            const bool startedFromOverlap =
                IsOverlappingActorMovementTarget(actorId);
            const bool moved =
                m_World.UpdateActorMovement(actorId, m_CurrentTick);

            if (moved && !startedFromOverlap)
            {
                TryHandleActorTargetCombat(actorId);
            }
        }
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
        ProcessActorCombatQueue(actorId);

        if (m_World.GetActorCore(actorId) == nullptr)
        {
            continue;
        }

        if (!TryHandleActorTargetCombat(actorId))
        {
            const bool startedFromOverlap =
                IsOverlappingActorMovementTarget(actorId);
            const bool moved =
                m_World.UpdatePlayerMovement(actorId, m_CurrentTick);

            if (moved && !startedFromOverlap)
            {
                TryHandleActorTargetCombat(actorId);
            }
        }
    }
}

}  // namespace osrssim
