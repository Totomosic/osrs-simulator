# Tick Update Flow

This note describes the current live simulation tick path for players and NPCs.
`EncounterRunner` owns encounter orchestration, while `Engine` owns generic tick
execution.

## Encounter Step

```mermaid
flowchart TD
    Start["EncounterRunner::Step()"] --> Started{"Started?"}
    Started -- "no" --> Throw["throw logic_error"]
    Started -- "yes" --> Context["Create EncounterContext"]
    Context --> Complete{"ActiveEncounter::IsComplete(context)?"}
    Complete -- "yes" --> Stop["return false"]
    Complete -- "no" --> Before["ActiveEncounter::BeforeEngineTick(context)"]
    Before --> EngineStep["Engine::Step()"]
    EngineStep --> After["ActiveEncounter::AfterEngineTick(context)"]
    After --> Recorder{"Recorder attached?"}
    Recorder -- "yes" --> Record["EncounterRecorder::RecordCompletedTick(engine)"]
    Recorder -- "no" --> Done["return true"]
    Record --> Done
```

## Engine Tick Order

```mermaid
flowchart TD
    Step["Engine::Step()"] --> Increment["Increment m_CurrentTick"]
    Increment --> WorldTick["World::SetCurrentTick(m_CurrentTick)"]
    WorldTick --> PlayerActions["ProcessQueuedPlayerMovementActions()"]
    PlayerActions --> ApplyAction{"Queued player action kind"}
    ApplyAction -- "SceneCoordinate" --> SetCoord["World::SetActorSceneCoordinateMovementTarget(player, coordinate)"]
    ApplyAction -- "Actor" --> SetActor["World::SetActorMovementTarget(player, targetActor)"]
    SetCoord --> ClearActions["Clear queued player movement actions"]
    SetActor --> ClearActions
    ClearActions --> Timers["CombatService::DecrementAttackTimers(world)"]
    Timers --> Npcs["UpdateNpcs() in ascending NpcIndex"]
    Npcs --> Players["UpdatePlayers() in ascending PlayerIndex"]
    Players --> Done["Tick complete"]
```

## NPC Update

Each live NPC is selected by `World::GetNextNpcActorIdAfterIndex`, so
`NpcIndex` controls NPC update order.

```mermaid
flowchart TD
    NextNpc["Next NPC by NpcIndex"] --> Exists{"NPC still exists?"}
    Exists -- "no" --> MoreNpc["Continue to next NPC"]
    Exists -- "yes" --> Queue["ProcessActorCombatQueue(actorId)"]
    Queue --> CombatQueue["CombatQueue::Process(World::GetCurrentTick())"]
    CombatQueue --> Removals["DrainQueuedActorRemovals()"]
    Removals --> Removed{"Actor removed by queued combat/death?"}
    Removed -- "yes" --> MoreNpc
    Removed -- "no" --> Behavior["UpdateNpcBehavior(actorId)"]
    Behavior --> Resolve["Resolve npc.behaviorId in Engine-owned behavior table"]
    Resolve --> Context["Build NpcBehaviorContext"]
    Context --> BehaviorUpdate["NpcBehavior::Update(context, actorId)"]
    BehaviorUpdate --> Custom{"Behavior implementation"}
    Custom -- "DefaultNpcBehavior" --> DefaultNpc["Default target-combat movement flow"]
    Custom -- "Custom behavior" --> CustomCalls["May use context World, CombatService, NPC lifecycle, or behavior assignment callbacks"]
    DefaultNpc --> TryAttack1["context.TryHandleActorTargetCombat(actorId)"]
    TryAttack1 --> CanAttack1{"Movement target is actor and CombatService::CanAttackActorTarget(...)"}
    CanAttack1 -- "yes, attack timer ready" --> Dispatch1["CombatService::DispatchAttack(world, actorId, targetId, tick)"]
    CanAttack1 -- "yes, attack timer not ready" --> MoreNpc
    Dispatch1 --> QueueDamage1["Queue damage/projectile via World::QueueActorCombatEvent and reset attack timer"]
    QueueDamage1 --> MoreNpc
    CanAttack1 -- "no" --> Overlap["Engine::IsOverlappingActorMovementTarget(actorId)"]
    Overlap --> Move["World::UpdateActorMovement(actorId, tick)"]
    Move --> Moved{"Moved and did not start overlapped?"}
    Moved -- "yes" --> TryAttack2["TryHandleActorTargetCombat(actorId) again"]
    TryAttack2 --> MoreNpc
    Moved -- "no" --> MoreNpc
    CustomCalls --> MoreNpc
```

## Player Update

Each live player is selected by `World::GetNextPlayerActorIdAfterIndex`, so
`PlayerIndex` controls player update order. Players do not resolve an
`NpcBehavior`; after combat queue processing they run the same target-combat
then movement pattern directly in `Engine::UpdatePlayers`.

```mermaid
flowchart TD
    NextPlayer["Next player by PlayerIndex"] --> Exists{"Player still exists?"}
    Exists -- "no" --> MorePlayer["Continue to next player"]
    Exists -- "yes" --> Queue["ProcessActorCombatQueue(actorId)"]
    Queue --> CombatQueue["CombatQueue::Process(World::GetCurrentTick())"]
    CombatQueue --> Removals["DrainQueuedActorRemovals()"]
    Removals --> Removed{"Actor removed by queued combat/death?"}
    Removed -- "yes" --> MorePlayer
    Removed -- "no" --> TryAttack1["TryHandleActorTargetCombat(actorId)"]
    TryAttack1 --> CanAttack1{"Movement target is actor and CombatService::CanAttackActorTarget(...)"}
    CanAttack1 -- "yes, attack timer ready" --> Dispatch1["CombatService::DispatchAttack(world, actorId, targetId, tick)"]
    CanAttack1 -- "yes, attack timer not ready" --> MorePlayer
    Dispatch1 --> QueueDamage1["Queue damage/projectile via World::QueueActorCombatEvent and reset attack timer"]
    QueueDamage1 --> MorePlayer
    CanAttack1 -- "no" --> Overlap["Engine::IsOverlappingActorMovementTarget(actorId)"]
    Overlap --> Move["World::UpdatePlayerMovement(actorId, tick)"]
    Move --> ActorMove["World::UpdateActorMovement(actorId, tick)"]
    ActorMove --> Moved{"Moved and did not start overlapped?"}
    Moved -- "yes" --> TryAttack2["TryHandleActorTargetCombat(actorId) again"]
    TryAttack2 --> MorePlayer
    Moved -- "no" --> MorePlayer
```

## Service Calls By Purpose

| Purpose | Service or owner | Called when |
| --- | --- | --- |
| Encounter lifecycle and tick hooks | `EncounterRunner`, `ActiveEncounter`, `EncounterContext` | Around each engine tick: completion check, `BeforeEngineTick`, `AfterEngineTick`, optional recording |
| Current tick state | `Engine`, `World` | At the start of `Engine::Step`, before any queued actions or actor updates |
| Player input/action staging | `Engine::QueuePlayerMoveToSceneCoordinate`, `Engine::QueuePlayerMoveToActor` | Before a tick; applied during `ProcessQueuedPlayerMovementActions` |
| Movement target storage | `World` | Queued player actions and behavior code set actor or scene-coordinate movement targets |
| Attack timer progression | `CombatService::DecrementAttackTimers` | Once per tick, after queued player movement actions and before any actor update |
| NPC update order | `World::GetNextNpcActorIdAfterIndex` | During `UpdateNpcs`; ascending `NpcIndex` |
| Player update order | `World::GetNextPlayerActorIdAfterIndex` | During `UpdatePlayers`; ascending `PlayerIndex` |
| Delayed combat effects | `CombatQueue` owned by each actor | At the start of that actor's NPC/player update |
| Death cleanup | `World::QueueActorRemoval`, `Engine::DrainQueuedActorRemovals` | After an actor combat queue is processed |
| NPC decision logic | `NpcBehavior` through `NpcBehaviorContext` | After an NPC combat queue is processed and the NPC still exists |
| Target combat eligibility | `CombatService::CanAttackActorTarget` | Before movement, and again after successful movement when the actor did not start overlapped |
| Attack dispatch | `CombatService::DispatchAttack` | When target combat is possible and the actor attack timer is ready |
| Damage rolls and delayed damage | `CombatService`, `DpsService`, `World::QueueActorCombatEvent` | During standard attack dispatch |
| Movement execution | `World::UpdateActorMovement`; players enter via `World::UpdatePlayerMovement` | When target combat cannot currently be handled |

## Timing Notes

- `Engine::Step` updates NPCs before players.
- Attack timers are decremented before any actor's combat queue or movement is
  processed for the tick.
- An actor processes only the combat events that are eligible at the start of
  its combat queue processing. Combat events created during the same current
  tick are retained for a later update.
- Standard attacks queue delayed damage on the target actor's `CombatQueue`.
  Damage application can queue a death-removal event, and the engine removes the
  actor when that removal is later drained through `DrainQueuedActorRemovals`.
- `DefaultNpcBehavior` and player updates both try target combat before moving,
  then try again after a successful movement if the actor did not start the tick
  with footprint overlap against its actor movement target.
