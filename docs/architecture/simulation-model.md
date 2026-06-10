# Simulation Model

The engine should be deterministic for a given initial state, input stream, and RNG seed.

Core engine concepts should model OSRS terms directly:

- ticks
- tiles and planes
- maps and collision
- entities
- player actions
- NPC movement and behavior
- combat state
- random rolls

The simulator layer should treat the engine as a deterministic component and add repetition, sampling, statistics, optimization, and persistence.
