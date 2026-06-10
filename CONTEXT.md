# OSRS Simulator

This context describes the core Old School RuneScape simulation concepts used by the engine.

## Language

**Scene**:
A loaded area of the game world containing tiles across one or more planes. A scene is the simulation's current loaded scope, not the entire persistent world. Map is reserved for a future persistent-world concept.
_Avoid_: Map, world map

**Scene Coordinate**:
A tile position inside the loaded scene, identified by plane, x, and y. Scene coordinates currently follow OSRS scene bounds, but they are distinct from persistent world coordinates.
_Avoid_: Map coordinate

**Compass Direction**:
A scene-relative direction such as north, east, south, west, or a diagonal between them. Compass directions describe tile sides and movement steps.
_Avoid_: Left, right, top, bottom

**Cardinal Direction**:
A non-diagonal compass direction: north, east, south, or west. Cardinal directions describe wall object and game object orientation.
_Avoid_: Orientation integer

**Tile**:
A single cell within a scene. A tile is the place where occupancy, movement or line-of-sight blocking, and scene content are expressed.
_Avoid_: Coordinate

**Tile Flag**:
A property on a tile that affects simulation rules such as occupancy, compass-directional movement blocking, or compass-directional line-of-sight blocking.
_Avoid_: Tile state

**Scene Entity**:
A non-actor thing placed in a scene, such as a wall object or game object. A scene entity can cover one or more tiles.
_Avoid_: Object, tile content

**Wall Object**:
A scene entity that occupies one or two cardinal compass-oriented edges of a tile.
_Avoid_: Wall, boundary

**Game Object**:
A scene entity that occupies one or more whole tiles in a scene.
_Avoid_: Object

**Object Definition**:
The reusable definition of an OSRS object, separate from any specific placement of that object in a scene.
_Avoid_: Scene entity

**Collision Profile**:
The object-local movement and line-of-sight blocking shape applied when a scene entity is placed in a scene.
_Avoid_: Tile flag

**Actor**:
A moving participant in the simulation, such as a player or NPC. Actors can occupy tiles, but they are not scene entities.
_Avoid_: Scene entity

**Pathing**:
The rules used to decide whether movement through a scene is possible. Pathing interprets tile blocking and occupancy rather than owning scene data.
_Avoid_: Map traversal
