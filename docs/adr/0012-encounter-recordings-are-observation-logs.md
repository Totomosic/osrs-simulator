# Encounter Recordings Are Observation Logs

## Status

Accepted

Encounter recordings capture what was observed during one active encounter, not the inputs and random seed needed to re-run it. A recording stores the initial scene and placed actor state, then one contiguous entry per completed encounter runner tick after `AfterEngineTick`, with changed facts stored as absolute values rather than deltas.

## Considered Options

- Record player inputs, RNG seed, and enough state to reproduce the encounter by re-running the engine.
- Record every full world snapshot on every tick.
- Record sparse observed facts, plus semantic events that cannot be inferred from snapshots.

## Decision

Encounter recordings are sparse observation logs. Actor and scene state changes are recorded as absolute facts, only placed actors are included, scene entities are recorded rather than tile flags, and the per-tick projectile bucket contains the full visible projectile list so playback does not need to reconstruct combat queues.

## Consequences

Recordings are useful for inspection, debugging, regression tests, and web playback, but they are not simulation replay logs and cannot be used to resume an active encounter. Encounter-specific private phase state is out of scope unless a future optional debug-state provider is designed.
