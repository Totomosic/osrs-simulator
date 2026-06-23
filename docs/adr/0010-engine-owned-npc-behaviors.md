# Engine-Owned NPC Behaviors

## Status

Accepted

NPC behavior is live decision logic, while `World` remains the owner of actor and scene state. NPCs store an `NpcBehaviorId`, but `Engine` owns the behavior instances, resolves behavior IDs during NPC processing, and performs behavior cleanup as part of actor lifecycle APIs.

## Considered Options

- Store behavior objects directly on NPCs in `World`.
- Store raw behavior pointers on NPCs while `Engine` owns behavior objects.
- Store stable `NpcBehaviorId` values on NPCs and resolve them through an engine-owned behavior table.

## Decision

NPCs store stable `NpcBehaviorId` values. `Engine` owns a dense behavior table, reserves behavior ID `0` for the shared `DefaultNpcBehavior`, and updates each NPC's behavior during NPC processing. `NpcBehavior::CanBeShared()` controls whether a behavior instance can be assigned to multiple NPCs; invalid sharing of a non-shareable behavior is a programmer error.

`World` owns `PlayerIndex` and `NpcIndex` allocation as live actor state. These indices use the range `0` through `65535`, are unique among live actors of the same kind, wrap to `0` after `65535`, and drive player/NPC update order. `ActorId` remains the stable simulator identity used by movement targets, combat targets, scene membership, and external references.

## Consequences

Encounter and behavior code should create and remove live actors through `Engine` APIs when lifecycle cleanup matters. `World::CreatePlayer`, `World::CreateNpc`, and `World::RemoveActor` remain low-level state APIs, but direct world removal bypasses engine-owned behavior cleanup. Queued removals caused by combat death must eventually be drained and applied through `Engine` so dedicated NPC behaviors do not leak.
