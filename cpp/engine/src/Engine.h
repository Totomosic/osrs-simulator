#pragma once

#include "CombatService.h"
#include "Types.h"
#include "World.h"
#include "behavior/NpcBehavior.h"

#include <memory>
#include <optional>
#include <vector>

namespace osrssim
{

class Engine
{
private:
    struct NpcBehaviorSlot
    {
        std::unique_ptr<behavior::NpcBehavior> behavior;
        int assignmentCount = 0;
        bool dedicated = false;
        bool destroyWhenSafe = false;
    };

    struct QueuedPlayerMovementAction
    {
        ActorId actorId = 0;
        MovementTarget movementTarget;
    };

    Tick m_CurrentTick = 0;
    World m_World;
    CombatService m_CombatService;
    std::vector<NpcBehaviorSlot> m_NpcBehaviors;
    std::vector<QueuedPlayerMovementAction> m_QueuedPlayerMovementActions;
    std::optional<NpcBehaviorId> m_UpdatingNpcBehaviorId;

public:
    Engine();

    std::optional<ActorId> CreatePlayer(
        int size,
        int speed,
        CombatComposition combatComposition);
    std::optional<ActorId> CreateNpc(
        int size,
        int speed,
        CombatComposition combatComposition);
    std::optional<ActorId> CreateNpc(
        int size,
        int speed,
        CombatComposition combatComposition,
        NpcBehaviorId behaviorId);
    std::optional<ActorId> CreateNpc(
        int size,
        int speed,
        CombatComposition combatComposition,
        std::unique_ptr<behavior::NpcBehavior> behavior);
    bool RemovePlayer(ActorId actorId);
    bool RemoveNpc(ActorId actorId);
    NpcBehaviorId RegisterNpcBehavior(
        std::unique_ptr<behavior::NpcBehavior> behavior);
    bool SetNpcBehavior(ActorId actorId, NpcBehaviorId behaviorId);
    bool SetNpcBehavior(
        ActorId actorId,
        std::unique_ptr<behavior::NpcBehavior> behavior);
    bool SetPlayerSceneCoordinateMovementTarget(
        ActorId actorId,
        SceneCoordinate coordinate);
    bool QueuePlayerMoveToSceneCoordinate(
        ActorId actorId,
        SceneCoordinate coordinate);
    bool QueuePlayerMoveToActor(ActorId actorId, ActorId targetActorId);
    void Step();
    Tick GetCurrentTick() const;
    World& GetWorld();
    const World& GetWorld() const;
    CombatService& GetCombatService();
    const CombatService& GetCombatService() const;
    const behavior::NpcBehavior* GetNpcBehavior(
        NpcBehaviorId behaviorId) const;
    int GetNpcBehaviorCount() const;

private:
    void ProcessQueuedPlayerMovementActions();
    void DecrementAttackTimers();
    void ProcessActorCombatQueue(ActorId actorId);
    void DrainQueuedActorRemovals();
    void ValidateNpcBehaviorForAssignment(NpcBehaviorId behaviorId) const;
    NpcBehaviorId AddNpcBehavior(
        std::unique_ptr<behavior::NpcBehavior> behavior,
        bool dedicated);
    void ReleaseNpcBehaviorAssignment(NpcBehaviorId behaviorId);
    void DestroyNpcBehaviorWhenSafe(NpcBehaviorId behaviorId);
    bool TryHandleActorTargetCombat(ActorId actorId);
    bool IsOverlappingActorMovementTarget(ActorId actorId) const;
    void UpdateNpcBehavior(ActorId actorId);
    void UpdateNpcs();
    void UpdatePlayers();
};

}  // namespace osrssim
