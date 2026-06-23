#pragma once

#include "Types.h"

namespace osrssim
{

class CombatService;
class World;

namespace behavior
{

class NpcBehaviorContext
{
private:
    World& m_World;
    CombatService& m_CombatService;
    Tick m_CurrentTick = 0;
    bool (*m_TryHandleActorTargetCombat)(void*, ActorId) = nullptr;
    bool (*m_IsOverlappingActorMovementTarget)(void*, ActorId) = nullptr;
    void* m_HelperOwner = nullptr;

public:
    NpcBehaviorContext(
        World& world,
        CombatService& combatService,
        Tick currentTick,
        void* helperOwner,
        bool (*tryHandleActorTargetCombat)(void*, ActorId),
        bool (*isOverlappingActorMovementTarget)(void*, ActorId));

    World& GetWorld() const;
    CombatService& GetCombatService() const;
    Tick GetCurrentTick() const;
    bool TryHandleActorTargetCombat(ActorId actorId) const;
    bool IsOverlappingActorMovementTarget(ActorId actorId) const;
};

class NpcBehavior
{
public:
    virtual ~NpcBehavior() = default;

    virtual bool CanBeShared() const;
    virtual void Update(NpcBehaviorContext& context, ActorId actorId) = 0;
};

}  // namespace behavior
}  // namespace osrssim
