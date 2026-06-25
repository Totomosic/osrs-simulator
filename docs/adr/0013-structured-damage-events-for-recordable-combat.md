# Structured Damage Events For Recordable Combat

## Status

Accepted

Combat recordings need to explain which attack produced which delayed damage, but actor-owned combat queues currently store opaque callbacks that cannot be inspected. Recordable damage should therefore be represented as structured queued damage events linked to attack records, while opaque combat callbacks remain available for behavior that is not recordable as damage.

## Considered Options

- Infer attacks and damage from hitpoint changes at tick boundaries.
- Inspect queued callback lambdas to discover damage.
- Add structured queued damage events and observer notifications for attacks.

## Decision

CombatService should expose observational attack and damage notifications that cannot affect combat behavior. Standard attacks record automatically, named attack callbacks preserve their callback name, structured damage events carry launch-time hitpoint-clamped damage, and damage applications are recorded separately when those events apply.

## Consequences

Multiple queued damage events can belong to one attack, and damage event IDs link launch records to later application records. Custom callbacks that need recordable damage must use the structured damage API; arbitrary callback side effects are not treated as damage by the recorder.
