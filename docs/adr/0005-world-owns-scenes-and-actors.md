# World Owns Scenes and Actors

## Status

Accepted

The engine uses a `World` as the simulation state container that owns loaded scenes, actors, and actor scene membership. `Engine` advances ticks and delegates world state changes to `World`, while `Scene` remains focused on tiles, scene entities, and tile flags; this preserves a clear ownership boundary for future multi-scene support without treating the loaded simulation state as the persistent world map.
