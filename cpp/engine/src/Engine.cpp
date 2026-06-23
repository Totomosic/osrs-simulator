#include "Engine.h"

#include "behavior/DefaultNpcBehavior.h"

#include <optional>
#include <stdexcept>

namespace osrssim
{

Engine::Engine()
{
    m_NpcBehaviors.push_back(
        {std::make_unique<behavior::DefaultNpcBehavior>(), 0, false});
}

std::optional<ActorId> Engine::CreatePlayer(
    int size,
    int speed,
    CombatComposition combatComposition)
{
    return m_World.CreatePlayer(size, speed, combatComposition);
}

std::optional<ActorId> Engine::CreateNpc(
    int size,
    int speed,
    CombatComposition combatComposition)
{
    return m_World.CreateNpc(size, speed, combatComposition);
}

std::optional<ActorId> Engine::CreateNpc(
    int size,
    int speed,
    CombatComposition combatComposition,
    NpcBehaviorId behaviorId)
{
    std::optional<ActorId> actorId =
        m_World.CreateNpc(size, speed, combatComposition);

    if (!actorId.has_value())
    {
        return std::nullopt;
    }

    try
    {
        SetNpcBehavior(actorId.value(), behaviorId);
    }
    catch (...)
    {
        m_World.RemoveActor(actorId.value());
        throw;
    }

    return actorId;
}

std::optional<ActorId> Engine::CreateNpc(
    int size,
    int speed,
    CombatComposition combatComposition,
    std::unique_ptr<behavior::NpcBehavior> behavior)
{
    std::optional<ActorId> actorId =
        m_World.CreateNpc(size, speed, combatComposition);

    if (!actorId.has_value())
    {
        return std::nullopt;
    }

    try
    {
        SetNpcBehavior(actorId.value(), std::move(behavior));
    }
    catch (...)
    {
        m_World.RemoveActor(actorId.value());
        throw;
    }

    return actorId;
}

bool Engine::RemovePlayer(ActorId actorId)
{
    if (m_World.GetPlayer(actorId) == nullptr)
    {
        return false;
    }

    return m_World.RemoveActor(actorId);
}

bool Engine::RemoveNpc(ActorId actorId)
{
    const Npc* npc = m_World.GetNpc(actorId);

    if (npc == nullptr)
    {
        return false;
    }

    const NpcBehaviorId behaviorId = npc->behaviorId;
    const bool removed = m_World.RemoveActor(actorId);

    if (removed)
    {
        ReleaseNpcBehaviorAssignment(behaviorId);
    }

    return removed;
}

NpcBehaviorId Engine::RegisterNpcBehavior(
    std::unique_ptr<behavior::NpcBehavior> behavior)
{
    return AddNpcBehavior(std::move(behavior), false);
}

bool Engine::SetNpcBehavior(ActorId actorId, NpcBehaviorId behaviorId)
{
    const Npc* npc = m_World.GetNpc(actorId);

    if (npc == nullptr)
    {
        return false;
    }

    const NpcBehaviorId oldBehaviorId = npc->behaviorId;

    if (oldBehaviorId == behaviorId)
    {
        return true;
    }

    ValidateNpcBehaviorForAssignment(behaviorId);
    ++m_NpcBehaviors[behaviorId].assignmentCount;

    if (!m_World.SetNpcBehaviorId(actorId, behaviorId))
    {
        ReleaseNpcBehaviorAssignment(behaviorId);
        return false;
    }

    ReleaseNpcBehaviorAssignment(oldBehaviorId);

    return true;
}

bool Engine::SetNpcBehavior(
    ActorId actorId,
    std::unique_ptr<behavior::NpcBehavior> behavior)
{
    const Npc* npc = m_World.GetNpc(actorId);

    if (npc == nullptr)
    {
        return false;
    }

    const NpcBehaviorId behaviorId = AddNpcBehavior(std::move(behavior), true);

    try
    {
        if (!SetNpcBehavior(actorId, behaviorId))
        {
            ReleaseNpcBehaviorAssignment(behaviorId);
            return false;
        }
    }
    catch (...)
    {
        ReleaseNpcBehaviorAssignment(behaviorId);
        throw;
    }

    return true;
}

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

const behavior::NpcBehavior* Engine::GetNpcBehavior(
    NpcBehaviorId behaviorId) const
{
    if (behaviorId >= m_NpcBehaviors.size())
    {
        return nullptr;
    }

    return m_NpcBehaviors[behaviorId].behavior.get();
}

int Engine::GetNpcBehaviorCount() const
{
    return static_cast<int>(m_NpcBehaviors.size());
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

void Engine::ValidateNpcBehaviorForAssignment(NpcBehaviorId behaviorId) const
{
    if (behaviorId >= m_NpcBehaviors.size() ||
        m_NpcBehaviors[behaviorId].behavior == nullptr)
    {
        throw std::logic_error("NPC behavior ID is not registered");
    }

    const NpcBehaviorSlot& slot = m_NpcBehaviors[behaviorId];

    if (!slot.behavior->CanBeShared() && slot.assignmentCount > 0)
    {
        throw std::logic_error("NPC behavior cannot be shared");
    }
}

NpcBehaviorId Engine::AddNpcBehavior(
    std::unique_ptr<behavior::NpcBehavior> behavior,
    bool dedicated)
{
    if (behavior == nullptr)
    {
        throw std::invalid_argument("NPC behavior cannot be null");
    }

    const NpcBehaviorId behaviorId =
        static_cast<NpcBehaviorId>(m_NpcBehaviors.size());
    m_NpcBehaviors.push_back({std::move(behavior), 0, dedicated});

    return behaviorId;
}

void Engine::ReleaseNpcBehaviorAssignment(NpcBehaviorId behaviorId)
{
    if (behaviorId >= m_NpcBehaviors.size())
    {
        return;
    }

    NpcBehaviorSlot& slot = m_NpcBehaviors[behaviorId];

    if (slot.assignmentCount > 0)
    {
        --slot.assignmentCount;
    }

    if (slot.dedicated && slot.assignmentCount == 0)
    {
        slot.behavior.reset();
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

void Engine::UpdateNpcBehavior(ActorId actorId)
{
    const Npc* npc = m_World.GetNpc(actorId);

    if (npc == nullptr)
    {
        return;
    }

    behavior::NpcBehavior* behavior = nullptr;

    if (npc->behaviorId < m_NpcBehaviors.size())
    {
        behavior = m_NpcBehaviors[npc->behaviorId].behavior.get();
    }

    if (behavior == nullptr)
    {
        throw std::logic_error("NPC behavior ID is not registered");
    }

    behavior::NpcBehaviorContext context(
        m_World,
        m_CombatService,
        m_CurrentTick,
        this,
        [](void* owner, ActorId callbackActorId)
        {
            return static_cast<Engine*>(owner)->TryHandleActorTargetCombat(
                callbackActorId);
        },
        [](void* owner, ActorId callbackActorId)
        {
            return static_cast<Engine*>(owner)
                ->IsOverlappingActorMovementTarget(callbackActorId);
        });

    behavior->Update(context, actorId);
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

        UpdateNpcBehavior(actorId);
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
