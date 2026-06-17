# OSRS Simulator

This context describes the core Old School RuneScape simulation concepts used by the engine.

## Language

**Scene**:
A loaded area of the game world containing tiles across one or more planes. A scene is the simulation's current loaded scope, not the entire persistent world. Map is reserved for a future persistent-world concept.
_Avoid_: Map, world map

**Scene ID**:
A stable identifier for a loaded scene within a world.
_Avoid_: Map ID

**World**:
The simulation container that owns loaded scenes, actors, and actor scene membership. A world is active simulation state, not the persistent world map.
_Avoid_: Map, world map

**Scene Coordinate**:
A tile position inside the loaded scene, identified by plane, x, and y. Scene coordinates currently follow OSRS scene bounds, but they are distinct from persistent world coordinates.
_Avoid_: Map coordinate

**Camera**:
A viewer perspective centered on a scene coordinate in a loaded scene, with a field of view measured in tiles and an optional actor follow target. A camera is not simulation state.
_Avoid_: Viewport, map view

**Field of View**:
The number of scene tiles visible through the camera. Changing field of view zooms the viewer in or out without changing simulation tile size.
_Avoid_: Scale, pixel zoom

**Tick**:
A discrete simulation step. A tick is distinct from a rendered frame or real-time playback interval.
_Avoid_: Frame, update loop

**Seconds Per Tick**:
The real-time duration represented by one OSRS simulation tick. OSRS combat timing uses 0.6 seconds per tick.
_Avoid_: Frame duration, playback interval

**Scene Membership**:
The relationship between an actor and the loaded scene it currently occupies. Scene membership can change without changing the actor's identity.
_Avoid_: Actor scene ownership

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

**Line of Sight**:
A same-plane visibility relationship from an actor footprint to a target scene coordinate or another actor footprint, limited by range and blocked by line-of-sight tile flags rather than actor occupancy.
_Avoid_: Visibility, raycast

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
A moving participant in the simulation, such as a player or NPC. Actors can occupy tiles and move between loaded scenes, but they are not scene entities.
_Avoid_: Scene entity

**Actor ID**:
A stable identifier for an actor across movement and scene membership changes.
_Avoid_: Entity ID

**Actor Footprint**:
The square set of tiles occupied by an actor, anchored at the actor's south-west scene coordinate. Actor size is measured as the side length of this footprint in tiles.
_Avoid_: Actor bounds, actor area

**Edge Adjacency**:
The relationship between two actor footprints on the same plane when they touch along a cardinal edge and overlap along the perpendicular axis. Diagonal corner contact is not edge adjacency.
_Avoid_: Diagonal adjacency, overlap

**Corner Contact**:
The relationship between two actor footprints on the same plane when they touch only across a diagonal corner and do not share an edge or tile.
_Avoid_: Diagonal adjacency

**Footprint Overlap**:
The relationship between two actor footprints on the same plane when they share at least one tile.
_Avoid_: Underneath, covered by

**Actor Speed**:
The maximum number of per-tile movement steps an actor may take in one tick. A movement attempt may use less than the actor's speed.
_Avoid_: Run energy, movement mode

**Weapon**:
An actor's equipped attack profile, whether or not it represents a physical item. Each actor has exactly one weapon equipped.
_Avoid_: Attack style, attack profile

**Weapon ID**:
A stable simulator-local numeric identifier for a weapon, used to distinguish generic weapons from weapons with custom attack behavior.
_Avoid_: Callback key, attack type

**Weapon Name**:
The display name associated with a weapon definition.
_Avoid_: Weapon ID

**Unarmed**:
The built-in default weapon. Unarmed has weapon ID 0, weapon range 1, and weapon speed 4.
_Avoid_: No weapon

**Weapon Definition**:
The equipped weapon data carried by an actor, including weapon identity and basic weapon stats.
_Avoid_: Weapon instance, item definition

**Weapon Range**:
The maximum line-of-sight range at which a weapon can attack another actor. Weapon range uses actor footprints and same-plane line of sight.
_Avoid_: Distance, radius

**Weapon Speed**:
The number of ticks between attacks for a weapon.
_Avoid_: Attack rate, cooldown

**Attack**:
A weapon-driven interaction from one actor toward another actor. An attack can occur when the attacker has line of sight to the target within weapon range.
_Avoid_: Hit, damage roll

**Attack Type**:
The offensive category used to choose matching attack and defence bonuses for a combat roll, such as stab, slash, crush, magic, or a ranged defence subtype.
_Avoid_: Damage style, weapon type

**Maximum Hit**:
The highest damage value a successful damage roll can produce before post-roll reductions. Maximum hit is distinct from expected damage.
_Avoid_: Damage, expected damage

**Expected Damage**:
The average damage implied by combat rolls without sampling a specific hit. Expected damage accounts for hit chance and maximum hit.
_Avoid_: Average hit, deterministic damage

**Damage Roll**:
The sampled damage result for an attack after accuracy has been evaluated. A successful accuracy check can still produce a zero damage roll.
_Avoid_: Expected damage, max hit

**Random Seed**:
An explicit value used to initialize sampled combat outcomes so they are reproducible across simulation runs. Randomness inside the engine is deterministic when driven from the same random seed and inputs.
_Avoid_: Hidden randomness, ambient random state

**Attack Timer**:
An actor's per-tick countdown until its next attack can occur. Attacking sets the attack timer from the equipped weapon's speed, changing weapons does not reset it, and values at or below zero mean the actor is ready to attack.
_Avoid_: Cooldown, delay

**Movement Target**:
The current destination reference an actor is trying to reach, either a scene coordinate for its own south-west anchor or another actor it pursues until it can interact with the target. An actor has at most one movement target at a time.
_Avoid_: Target, target tile

**Partial Movement**:
A movement result where an actor moves less than the requested delta because pathing blocks the full movement before the actor exhausts its actor speed.
_Avoid_: Failed movement

**Actor Occupancy**:
A tile state indicating that actor movement should treat the tile as covered by an actor. Actor occupancy blocks NPC movement from ending on the occupied tile, but it does not block player movement.
_Avoid_: Character collision

**Player**:
An actor controlled as the user's character in the simulation.
_Avoid_: Character

**NPC**:
An actor controlled by the simulation rather than by the user.
_Avoid_: Non-player character

**Pathing**:
The rules used to decide whether movement through a scene is possible. Pathing interprets movement blocking and actor occupancy rather than owning scene data.
_Avoid_: Map traversal

**Large NPC Diagonal Squeeze**:
The movement rule where an NPC with an actor footprint larger than one tile may move diagonally around whole-tile blockers or actor occupancy that would block either cardinal side of the diagonal, provided its final actor footprint is clear and directional movement blocking still allows the diagonal crossing.
_Avoid_: Clipping, diagonal phasing
