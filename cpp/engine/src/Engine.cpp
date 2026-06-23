#include "Engine.h"

#include <optional>

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
    m_World.SetCurrentTick(m_CurrentTick);
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
        combatQueue->Process(m_World.GetCurrentTick());
        m_World.FlushQueuedActorRemovals();
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
    int npcIndex = -1;

    while (true)
    {
        const std::optional<ActorId> nextActorId =
            m_World.GetNextNpcActorIdAfterIndex(npcIndex);

        if (!nextActorId.has_value())
        {
            break;
        }

        const ActorId actorId = nextActorId.value();
        const Npc* npc = m_World.GetNpc(actorId);

        if (npc == nullptr)
        {
            continue;
        }

        npcIndex = static_cast<int>(npc->npcIndex);
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
    int playerIndex = -1;

    while (true)
    {
        const std::optional<ActorId> nextActorId =
            m_World.GetNextPlayerActorIdAfterIndex(playerIndex);

        if (!nextActorId.has_value())
        {
            break;
        }

        const ActorId actorId = nextActorId.value();
        const Player* player = m_World.GetPlayer(actorId);

        if (player == nullptr)
        {
            continue;
        }

        playerIndex = static_cast<int>(player->playerIndex);
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
