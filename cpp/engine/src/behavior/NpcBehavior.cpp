#include "behavior/NpcBehavior.h"

#include "CombatService.h"
#include "World.h"

namespace osrssim::behavior
{

NpcBehaviorContext::NpcBehaviorContext(
    World& world,
    CombatService& combatService,
    Tick currentTick,
    void* helperOwner,
    bool (*tryHandleActorTargetCombat)(void*, ActorId),
    bool (*isOverlappingActorMovementTarget)(void*, ActorId))
    : m_World(world),
      m_CombatService(combatService),
      m_CurrentTick(currentTick),
      m_TryHandleActorTargetCombat(tryHandleActorTargetCombat),
      m_IsOverlappingActorMovementTarget(isOverlappingActorMovementTarget),
      m_HelperOwner(helperOwner)
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

bool NpcBehavior::CanBeShared() const
{
    return false;
}

}  // namespace osrssim::behavior
