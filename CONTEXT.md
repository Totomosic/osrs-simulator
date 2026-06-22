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
A moving participant in the simulation, such as a player or NPC. Actors can occupy tiles, move between loaded scenes, and have a combat composition, but they are not scene entities.
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

**Tile Distance**:
The Chebyshev distance between the closest tiles in two same-plane actor footprints. Overlapping footprints have tile distance zero, and edge-adjacent footprints have tile distance one.
_Avoid_: Anchor distance, Manhattan distance

**Actor Speed**:
The maximum number of per-tile movement steps an actor may take in one tick. A movement attempt may use less than the actor's speed.
_Avoid_: Run energy, movement mode

**Weapon**:
A combat composition's equipped attack profile, whether or not it represents a physical item. Each combat composition has exactly one weapon.
_Avoid_: Attack style, attack profile

**Weapon ID**:
A stable simulator-local numeric identifier for a weapon, used to distinguish generic weapons from weapons with custom attack behavior.
_Avoid_: Callback key, attack type

**Weapon Name**:
The display name associated with a weapon record.
_Avoid_: Weapon ID

**Unarmed**:
The built-in default weapon. Unarmed has weapon ID 0, weapon range 1, and weapon speed 4.
_Avoid_: No weapon

**Weapon Definition**:
The equipped weapon data carried by a combat composition, including weapon identity and basic weapon stats.
_Avoid_: Weapon instance, item definition

**Weapon Record**:
A reusable weapon catalogue entry that pairs a weapon definition with simulator-specific weapon behavior and metadata.
_Avoid_: Weapon definition, equipment piece

**Equipment Piece**:
A single equippable item with a human-readable name, stable numeric ID, equipment slot, equipment bonuses, and optionally a weapon ID when the item equips a weapon.
_Avoid_: Item, gear item

**Equipment Piece ID**:
A stable numeric identifier for an equipment piece. Equipment piece ID is distinct from weapon ID, even when the equipment piece is a weapon.
_Avoid_: Weapon ID, item index

**Equipment Slot**:
The position in an equipment set where an equipment piece can be equipped, such as head, amulet, body, legs, weapon, shield, ring, or ammo.
_Avoid_: Gear slot, inventory slot, necklace

**Equipment Set**:
A collection of equipment pieces occupying equipment slots whose bonuses can be combined into a combat composition.
_Avoid_: Gear setup, loadout

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

**Combat Style**:
The selected attack mode that contributes small effective-level bonuses to combat rolls, such as accurate, aggressive, controlled, defensive, rapid, longrange, or defensive casting.
_Avoid_: Style bonus, attack type

**Combat Stats**:
An actor's current combat skill values used by combat calculations, separate from equipment.
_Avoid_: Skill levels, base stats

**Equipment Bonuses**:
The offensive, defensive, and damage bonuses contributed by equipment.
_Avoid_: Gear stats, item stats

**Combat Composition**:
A complete combat loadout, including combat stats, equipment bonuses, selected attack type, magic base maximum hit, and a weapon definition, that always contains enough data to produce both attack-side and defence-side composition for combat calculations.
_Avoid_: Combat setup, stat block

**Combat Composition ID**:
A stable simulator-local identifier for a combat composition, shared by built-in actor compositions and user-saved compositions.
_Avoid_: NPC setup ID, preset ID

**Saved Combat Composition**:
A combat composition saved by the user for reuse after it has been created or edited in the calculator.
_Avoid_: Preset, saved setup

**Attack Composition**:
The attack-side part of a combat composition, including the selected attack type, combat stats, equipment bonuses, and weapon definition used to calculate offensive rolls and maximum hit.
_Avoid_: Offensive stats

**Defence Composition**:
The defence-side part of a combat composition, including combat stats and equipment bonuses used to calculate defensive rolls.
_Avoid_: Defensive stats

**Maximum Hit**:
The highest damage value a successful damage roll can produce before post-roll reductions. Maximum hit is distinct from expected damage.
_Avoid_: Damage, expected damage

**Magic Base Maximum Hit**:
The base maximum hit used by a magic combat composition before contextual damage modifiers are applied.
_Avoid_: Spell damage, magic damage bonus

**Expected Damage**:
The average damage implied by combat rolls without sampling a specific hit. Expected damage accounts for hit chance and maximum hit.
_Avoid_: Average hit, deterministic damage

**Damage Roll**:
The sampled damage result for an attack after accuracy has been evaluated. A successful accuracy check can still produce a zero damage roll.
_Avoid_: Expected damage, max hit

**Death**:
A combat outcome where an actor's hitpoints reach zero and the actor stops participating in the current simulation.
_Avoid_: Despawn, deletion

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

**Player Attack Setup**:
A user-facing calculator setup used to compare one player attack option against the same defender. A player attack setup produces an attack composition plus calculator-specific comparison metadata.
_Avoid_: Attack composition, offensive stats, player stat block

**Baseline Player Attack Setup**:
The first player attack setup in a comparison, used as the reference for percentage difference in calculated combat outcomes.
_Avoid_: Control setup, default setup

**NPC**:
An actor controlled by the simulation rather than by the user.
_Avoid_: Non-player character

**NPC Name**:
The display name associated with an NPC definition.
_Avoid_: Combat composition name

**NPC ID**:
A stable simulator-local numeric identifier for an NPC definition. NPC ID is distinct from actor ID because an NPC definition is catalogue data, not a live actor.
_Avoid_: Actor ID

**NPC Definition**:
A reusable definition of an NPC's non-combat actor traits and associated combat composition identity.
_Avoid_: NPC instance, NPC combat setup

**NPC Defence Setup**:
A user-facing calculator setup used as the shared NPC defender for one or more player attack setups. An NPC defence setup produces a defence composition plus calculator-specific defender metadata.
_Avoid_: Defence composition, defensive stats, NPC stat block

**Pathing**:
The rules used to decide whether movement through a scene is possible. Pathing interprets movement blocking and actor occupancy rather than owning scene data.
_Avoid_: Map traversal

**Large NPC Diagonal Squeeze**:
The movement rule where an NPC with an actor footprint larger than one tile may move diagonally around whole-tile blockers or actor occupancy that would block either cardinal side of the diagonal, provided its final actor footprint is clear and directional movement blocking still allows the diagonal crossing.
_Avoid_: Clipping, diagonal phasing
