#pragma once

#include "DpsService.h"
#include "Types.h"

#include <functional>
#include <memory>
#include <optional>

namespace osrssim
{

class CombatService;
class World;

namespace behavior
{

class NpcBehavior;

class NpcBehaviorContext
{
private:
    using CreateDefaultNpcCallback =
        std::function<std::optional<ActorId>(
            int,
            int,
            CombatComposition)>;
    using CreateNpcWithBehaviorIdCallback =
        std::function<std::optional<ActorId>(
            int,
            int,
            CombatComposition,
            NpcBehaviorId)>;
    using CreateNpcWithOwnedBehaviorCallback =
        std::function<std::optional<ActorId>(
            int,
            int,
            CombatComposition,
            std::unique_ptr<NpcBehavior>)>;
    using RemoveNpcCallback = std::function<bool(ActorId)>;
    using SetNpcBehaviorIdCallback =
        std::function<bool(ActorId, NpcBehaviorId)>;
    using SetNpcOwnedBehaviorCallback =
        std::function<bool(ActorId, std::unique_ptr<NpcBehavior>)>;

    World& m_World;
    CombatService& m_CombatService;
    Tick m_CurrentTick = 0;
    bool (*m_TryHandleActorTargetCombat)(void*, ActorId) = nullptr;
    bool (*m_IsOverlappingActorMovementTarget)(void*, ActorId) = nullptr;
    void* m_HelperOwner = nullptr;
    CreateDefaultNpcCallback m_CreateDefaultNpc;
    CreateNpcWithBehaviorIdCallback m_CreateNpcWithBehaviorId;
    CreateNpcWithOwnedBehaviorCallback m_CreateNpcWithOwnedBehavior;
    RemoveNpcCallback m_RemoveNpc;
    SetNpcBehaviorIdCallback m_SetNpcBehaviorId;
    SetNpcOwnedBehaviorCallback m_SetNpcOwnedBehavior;

public:
    NpcBehaviorContext(
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
        SetNpcOwnedBehaviorCallback setNpcOwnedBehavior);

    World& GetWorld() const;
    CombatService& GetCombatService() const;
    Tick GetCurrentTick() const;
    bool TryHandleActorTargetCombat(ActorId actorId) const;
    bool IsOverlappingActorMovementTarget(ActorId actorId) const;
    std::optional<ActorId> CreateNpc(
        int size,
        int speed,
        CombatComposition combatComposition) const;
    std::optional<ActorId> CreateNpc(
        int size,
        int speed,
        CombatComposition combatComposition,
        NpcBehaviorId behaviorId) const;
    std::optional<ActorId> CreateNpc(
        int size,
        int speed,
        CombatComposition combatComposition,
        std::unique_ptr<NpcBehavior> behavior) const;
    bool RemoveNpc(ActorId actorId) const;
    bool SetNpcBehavior(ActorId actorId, NpcBehaviorId behaviorId) const;
    bool SetNpcBehavior(
        ActorId actorId,
        std::unique_ptr<NpcBehavior> behavior) const;
};

class NpcBehavior
{
public:
    virtual ~NpcBehavior() = default;

    virtual bool CanBeShared() const;
    virtual void OnAttack(NpcBehaviorContext& context, ActorId actorId);
    virtual void OnDamageTaken(
        NpcBehaviorContext& context,
        ActorId actorId,
        ActorId sourceActorId,
        int damage);
    virtual void Update(NpcBehaviorContext& context, ActorId actorId) = 0;
};

}  // namespace behavior
}  // namespace osrssim
