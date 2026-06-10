# OSRS Scene Model

## Status

Accepted

The engine models the currently loaded OSRS-style area as a `Scene`, not as a `Map`; map remains reserved for a future persistent-world concept. A scene uses fixed OSRS client dimensions of four planes by 104 by 104 tiles, scene coordinates remain local x/y/plane values, and pathing interprets a single tile flag mask containing occupancy, movement blocking, and line-of-sight blocking.

Scene tiles are the query surface for scene content, but scene entities are stored as identities/handles rather than embedded object records. The first supported scene entity categories are wall objects and game objects: wall objects occupy one or two cardinal tile edges, while game objects occupy one or more whole tiles from an origin, size, and cardinal orientation. Multi-tile scene entities should be visible from every tile they cover, while their own record remains the canonical placement.

Scene entity placement and removal own tile-content updates and tile-flag updates together. Placement receives an object-local collision profile rather than asking `Scene` to resolve object definitions directly; object definitions are a separate concept that future loaders or definition services can translate into collision profiles. Collision profiles include movement and line-of-sight blocking, but not actor occupancy.

Deferred concepts include persistent maps, world coordinates, render levels, bridge tiles, ground items, ground objects, decorative objects, tile paint/model data, and direct object-definition lookup inside `Scene`.
