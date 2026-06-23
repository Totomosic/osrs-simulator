#include "behavior/NpcBehavior.h"

#include "CombatService.h"
#include "World.h"

#include <utility>

namespace osrssim::behavior
{

NpcBehaviorContext::NpcBehaviorContext(
    World& world,
    CombatService& combatService,
    Tick currentTick,
    void* helperOwner,
    bool (*tryHandleActorTargetCombat)(void*, ActorId),
    bool (*isOverlappingActorMovementTarget)(void*, ActorId),
    CreateDefaultNpcCallback createDefaultNpc,
    CreateNpcWithBehaviorIdCallback createNpcWithBehaviorId,
    CreateNpcWithOwnedBehaviorCallback createNpcWithOwnedBehavior,
    RemoveNpcCallback removeNpc,
    SetNpcBehaviorIdCallback setNpcBehaviorId,
    SetNpcOwnedBehaviorCallback setNpcOwnedBehavior)
    : m_World(world),
      m_CombatService(combatService),
      m_CurrentTick(currentTick),
      m_TryHandleActorTargetCombat(tryHandleActorTargetCombat),
      m_IsOverlappingActorMovementTarget(isOverlappingActorMovementTarget),
      m_HelperOwner(helperOwner),
      m_CreateDefaultNpc(std::move(createDefaultNpc)),
      m_CreateNpcWithBehaviorId(std::move(createNpcWithBehaviorId)),
      m_CreateNpcWithOwnedBehavior(std::move(createNpcWithOwnedBehavior)),
      m_RemoveNpc(std::move(removeNpc)),
      m_SetNpcBehaviorId(std::move(setNpcBehaviorId)),
      m_SetNpcOwnedBehavior(std::move(setNpcOwnedBehavior))
{
}

World& NpcBehaviorContext::GetWorld() const
{
    return m_World;
}

CombatService& NpcBehaviorContext::GetCombatService() const
{
    return m_CombatService;
}

Tick NpcBehaviorContext::GetCurrentTick() const
{
    return m_CurrentTick;
}

bool NpcBehaviorContext::TryHandleActorTargetCombat(ActorId actorId) const
{
    return m_TryHandleActorTargetCombat(m_HelperOwner, actorId);
}

bool NpcBehaviorContext::IsOverlappingActorMovementTarget(
    ActorId actorId) const
{
    return m_IsOverlappingActorMovementTarget(m_HelperOwner, actorId);
}

std::optional<ActorId> NpcBehaviorContext::CreateNpc(
    int size,
    int speed,
    CombatComposition combatComposition) const
{
    return m_CreateDefaultNpc(size, speed, combatComposition);
}

std::optional<ActorId> NpcBehaviorContext::CreateNpc(
    int size,
    int speed,
    CombatComposition combatComposition,
    NpcBehaviorId behaviorId) const
{
    return m_CreateNpcWithBehaviorId(size, speed, combatComposition, behaviorId);
}

std::optional<ActorId> NpcBehaviorContext::CreateNpc(
    int size,
    int speed,
    CombatComposition combatComposition,
    std::unique_ptr<NpcBehavior> behavior) const
{
    return m_CreateNpcWithOwnedBehavior(
        size,
        speed,
        combatComposition,
        std::move(behavior));
}

bool NpcBehaviorContext::RemoveNpc(ActorId actorId) const
{
    return m_RemoveNpc(actorId);
}

bool NpcBehaviorContext::SetNpcBehavior(
    ActorId actorId,
    NpcBehaviorId behaviorId) const
{
    return m_SetNpcBehaviorId(actorId, behaviorId);
}

bool NpcBehaviorContext::SetNpcBehavior(
    ActorId actorId,
    std::unique_ptr<NpcBehavior> behavior) const
{
    return m_SetNpcOwnedBehavior(actorId, std::move(behavior));
}

bool NpcBehavior::CanBeShared() const
{
    return false;
}

}  // namespace osrssim::behavior
