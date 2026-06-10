# Coarse Actor Occupancy

## Status

Accepted

Actor occupancy is represented as a coarse tile state rather than an exact count or set of actor IDs. This means overlapping actors may share a tile, actors may be created within already occupied tiles, actor occupancy does not block later scene entity placement, and when one actor leaves that tile, the tile can become unoccupied even if another actor is still physically there; this intentionally matches the simple movement-blocking flag model used by pathing instead of introducing exact overlap accounting.
