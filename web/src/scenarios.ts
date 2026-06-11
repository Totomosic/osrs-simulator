export type ScenarioId =
    | "empty-world"
    | "actor-occupancy"
    | "npc-movement"
    | "game-object-blocker"
    | "wall-blocker";

export type CardinalDirection = "North" | "East" | "South" | "West";

export type TileFlag =
    | "Occupied"
    | "BlockMovementNorth"
    | "BlockMovementEast"
    | "BlockMovementSouth"
    | "BlockMovementWest"
    | "BlockMovementFull"
    | "BlockMovementObject"
    | "BlockLineOfSightNorth"
    | "BlockLineOfSightEast"
    | "BlockLineOfSightSouth"
    | "BlockLineOfSightWest"
    | "BlockLineOfSightFull";

export interface CollisionProfile {
    blocksMovement: boolean;
    blocksLineOfSight: boolean;
}

export interface SceneCoordinate {
    x: number;
    y: number;
    plane: number;
}

export interface ActorSnapshot {
    id: number;
    kind: "NPC" | "Player";
    size: number;
    speed: number;
    sceneId: number;
    coordinate: SceneCoordinate;
    footprint: SceneCoordinate[];
}

export interface TileSnapshot {
    coordinate: SceneCoordinate;
    flags: TileFlag[];
    wallObject?: WallObjectSnapshot;
    gameObject?: GameObjectSnapshot;
}

export interface WallObjectSnapshot {
    id: number;
    coordinate: SceneCoordinate;
    directions: CardinalDirection[];
    collisionProfiles: CollisionProfile[];
}

export interface GameObjectSnapshot {
    id: number;
    origin: SceneCoordinate;
    direction: CardinalDirection;
    sizeX: number;
    sizeY: number;
    footprint: SceneCoordinate[];
    collisionProfile: CollisionProfile;
}

export interface SceneSnapshot {
    sceneId: number;
    width: number;
    height: number;
    planeCount: number;
    tiles: TileSnapshot[];
}

export interface RenderTile {
    key: string;
    coordinate: SceneCoordinate;
    flags: TileFlag[];
    movementEdges: CardinalDirection[];
    wallEdges: CardinalDirection[];
    actorLabel?: string;
    actorKind?: "NPC" | "Player";
    wallObject?: WallObjectSnapshot;
    gameObject?: GameObjectSnapshot;
}

export interface ScenarioResult {
    id: ScenarioId;
    title: string;
    description: string;
    movementOutcome: string;
    scene: SceneSnapshot;
    actors: ActorSnapshot[];
    wallObjects: WallObjectSnapshot[];
    gameObjects: GameObjectSnapshot[];
    focus: {
        minX: number;
        maxX: number;
        minY: number;
        maxY: number;
    };
}

interface ActorRecord {
    id: number;
    kind: "NPC" | "Player";
    size: number;
    speed: number;
    sceneId?: number;
    coordinate?: SceneCoordinate;
}

interface TileRecord {
    coordinate: SceneCoordinate;
    flags: Set<TileFlag>;
    wallObject?: WallObjectSnapshot;
    gameObject?: GameObjectSnapshot;
}

interface WorldApi {
    getDefaultSceneId(): number;
    createNpc(size: number, speed: number): number;
    createPlayer(size: number, speed: number): number;
    placeActor(actorId: number, sceneId: number, coordinate: SceneCoordinate): boolean;
    moveActorByDelta(actorId: number, dx: number, dy: number): boolean;
    placeGameObject(
        coordinate: SceneCoordinate,
        id: number,
        direction: CardinalDirection,
        sizeX: number,
        sizeY: number,
        collisionProfile: CollisionProfile,
    ): boolean;
    placeWallObject(
        coordinate: SceneCoordinate,
        id: number,
        direction: CardinalDirection,
        collisionProfile: CollisionProfile,
    ): boolean;
    getActors(): ActorSnapshot[];
    getScene(): SceneSnapshot;
    getWallObjects(): WallObjectSnapshot[];
    getGameObjects(): GameObjectSnapshot[];
}

export const cardinalDirections: CardinalDirection[] = [
    "North",
    "East",
    "South",
    "West",
];

export const collisionProfiles = {
    movementBlocker: { blocksMovement: true, blocksLineOfSight: false },
    fullBlocker: { blocksMovement: true, blocksLineOfSight: true },
    neutral: { blocksMovement: false, blocksLineOfSight: false },
} satisfies Record<string, CollisionProfile>;

export const tileFlags: TileFlag[] = [
    "Occupied",
    "BlockMovementNorth",
    "BlockMovementEast",
    "BlockMovementSouth",
    "BlockMovementWest",
    "BlockMovementFull",
    "BlockMovementObject",
    "BlockLineOfSightNorth",
    "BlockLineOfSightEast",
    "BlockLineOfSightSouth",
    "BlockLineOfSightWest",
    "BlockLineOfSightFull",
];

export const scenarioOptions: Array<{ id: ScenarioId; title: string }> = [
    { id: "empty-world", title: "Empty Scene" },
    { id: "actor-occupancy", title: "Actor Occupancy" },
    { id: "npc-movement", title: "NPC Movement" },
    { id: "game-object-blocker", title: "Game Object Blocker" },
    { id: "wall-blocker", title: "Wall Object Blocker" },
];

export function runScenario(id: ScenarioId): ScenarioResult {
    switch (id) {
        case "empty-world":
            return runEmptyWorldScenario();
        case "actor-occupancy":
            return runActorOccupancyScenario();
        case "game-object-blocker":
            return runGameObjectBlockerScenario();
        case "wall-blocker":
            return runWallBlockerScenario();
        case "npc-movement":
        default:
            return runNpcMovementScenario();
    }
}

export function buildRenderTiles(result: ScenarioResult, plane: number): RenderTile[] {
    const actorTiles = new Map<string, { label: string; kind: "NPC" | "Player" }>();

    for (const actor of result.actors) {
        for (const coordinate of actor.footprint) {
            actorTiles.set(getKey(coordinate), { label: actor.kind, kind: actor.kind });
        }
    }

    return result.scene.tiles
        .filter((tile) => tile.coordinate.plane === plane)
        .map((tile) => {
            const actorTile = actorTiles.get(getKey(tile.coordinate));

            return {
                key: getKey(tile.coordinate),
                coordinate: tile.coordinate,
                flags: tile.flags,
                movementEdges: getMovementEdges(tile.flags),
                wallEdges: tile.wallObject?.directions ?? [],
                actorLabel: actorTile?.label,
                actorKind: actorTile?.kind,
                wallObject: tile.wallObject,
                gameObject: tile.gameObject,
            };
        })
        .sort((a, b) => a.coordinate.y - b.coordinate.y || a.coordinate.x - b.coordinate.x);
}

export function getSceneScreenCoordinate(
    coordinate: SceneCoordinate,
    scene: Pick<SceneSnapshot, "height">,
    tileSize: number,
): { x: number; y: number } {
    return {
        x: coordinate.x * tileSize,
        y: (scene.height - coordinate.y - 1) * tileSize,
    };
}

function runEmptyWorldScenario(): ScenarioResult {
    const world = createWorld();

    return buildResult(world, {
        id: "empty-world",
        title: "Empty Scene",
        description: "A neutral loaded Scene with no Actors or Scene Entities.",
        movementOutcome: "No movement attempted.",
        focus: { minX: 0, maxX: 103, minY: 0, maxY: 103 },
    });
}

function runActorOccupancyScenario(): ScenarioResult {
    const world = createWorld();
    const sceneId = world.getDefaultSceneId();

    world.placeActor(world.createPlayer(1, 1), sceneId, { x: 10, y: 10, plane: 0 });
    world.placeActor(world.createNpc(2, 1), sceneId, { x: 13, y: 10, plane: 0 });

    return buildResult(world, {
        id: "actor-occupancy",
        title: "Actor Occupancy",
        description: "A Player and size 2 NPC are placed through world Actor APIs.",
        movementOutcome: "World actor enumeration discovered both Scene Memberships.",
        focus: { minX: 9, maxX: 16, minY: 9, maxY: 13 },
    });
}

function runNpcMovementScenario(): ScenarioResult {
    const world = createWorld();
    const sceneId = world.getDefaultSceneId();
    const npcId = world.createNpc(2, 2);

    world.placeActor(npcId, sceneId, { x: 10, y: 10, plane: 0 });
    const moved = world.moveActorByDelta(npcId, 2, 0);

    return buildResult(world, {
        id: "npc-movement",
        title: "NPC Movement",
        description: "A size 2 NPC moves two tiles east and refreshes its footprint.",
        movementOutcome: moved
            ? "MoveActorByDelta returned true; Actor Footprint and Actor Occupancy moved east."
            : "MoveActorByDelta returned false.",
        focus: { minX: 9, maxX: 15, minY: 9, maxY: 13 },
    });
}

function runGameObjectBlockerScenario(): ScenarioResult {
    const world = createWorld();
    const sceneId = world.getDefaultSceneId();
    const npcId = world.createNpc(1, 2);

    world.placeGameObject(
        { x: 12, y: 10, plane: 0 },
        200,
        "North",
        2,
        2,
        collisionProfiles.fullBlocker,
    );
    world.placeActor(npcId, sceneId, { x: 10, y: 10, plane: 0 });
    const moved = world.moveActorByDelta(npcId, 2, 0);

    return buildResult(world, {
        id: "game-object-blocker",
        title: "Game Object Blocker",
        description: "A blocking Game Object footprint prevents the NPC from crossing.",
        movementOutcome: moved
            ? "MoveActorByDelta returned true."
            : "MoveActorByDelta returned false; the NPC stayed west of the Game Object.",
        focus: { minX: 9, maxX: 14, minY: 9, maxY: 12 },
    });
}

function runWallBlockerScenario(): ScenarioResult {
    const world = createWorld();
    const sceneId = world.getDefaultSceneId();
    const npcId = world.createNpc(1, 1);

    world.placeWallObject(
        { x: 10, y: 10, plane: 0 },
        100,
        "East",
        collisionProfiles.movementBlocker,
    );
    world.placeActor(npcId, sceneId, { x: 10, y: 10, plane: 0 });
    const moved = world.moveActorByDelta(npcId, 1, 0);

    return buildResult(world, {
        id: "wall-blocker",
        title: "Wall Object Blocker",
        description: "A Wall Object blocks eastward movement between adjacent tiles.",
        movementOutcome: moved
            ? "MoveActorByDelta returned true."
            : "MoveActorByDelta returned false; the Wall Object blocked the east step.",
        focus: { minX: 9, maxX: 12, minY: 9, maxY: 11 },
    });
}

function buildResult(
    world: WorldApi,
    options: {
        id: ScenarioId;
        title: string;
        description: string;
        movementOutcome: string;
        focus: ScenarioResult["focus"];
    },
): ScenarioResult {
    return {
        id: options.id,
        title: options.title,
        description: options.description,
        movementOutcome: options.movementOutcome,
        scene: world.getScene(),
        actors: world.getActors(),
        wallObjects: world.getWallObjects(),
        gameObjects: world.getGameObjects(),
        focus: options.focus,
    };
}

function createWorld(): WorldApi {
    return new InMemoryWorldApi();
}

class InMemoryWorldApi implements WorldApi {
    private readonly m_DefaultSceneId = 1;
    private m_NextActorId = 1;
    private readonly m_Actors = new Map<number, ActorRecord>();
    private readonly m_Tiles = new Map<string, TileRecord>();
    private readonly m_WallObjects = new Map<number, WallObjectSnapshot>();
    private readonly m_GameObjects = new Map<number, GameObjectSnapshot>();

    public getDefaultSceneId(): number {
        return this.m_DefaultSceneId;
    }

    public createNpc(size: number, speed: number): number {
        return this.createActor("NPC", size, speed);
    }

    public createPlayer(size: number, speed: number): number {
        return this.createActor("Player", size, speed);
    }

    public placeActor(
        actorId: number,
        sceneId: number,
        coordinate: SceneCoordinate,
    ): boolean {
        const actor = this.m_Actors.get(actorId);

        if (
            actor === undefined ||
            sceneId !== this.m_DefaultSceneId ||
            !this.canStandOnMovementBlockers(coordinate, actor.size)
        ) {
            return false;
        }

        if (actor.coordinate !== undefined) {
            this.setActorOccupancy(actor.coordinate, actor.size, false);
        }

        actor.sceneId = sceneId;
        actor.coordinate = coordinate;
        this.setActorOccupancy(coordinate, actor.size, true);

        return true;
    }

    public moveActorByDelta(actorId: number, dx: number, dy: number): boolean {
        const actor = this.m_Actors.get(actorId);

        if (
            actor === undefined ||
            actor.coordinate === undefined ||
            (dx === 0 && dy === 0) ||
            Math.max(Math.abs(dx), Math.abs(dy)) > actor.speed
        ) {
            return false;
        }

        const destination = {
            x: actor.coordinate.x + dx,
            y: actor.coordinate.y + dy,
            plane: actor.coordinate.plane,
        };

        if (!this.canMove(actor.coordinate, destination, actor.size)) {
            return false;
        }

        if (
            actor.kind === "NPC" &&
            this.hasFinalNpcOccupancyConflict(actor.coordinate, destination, actor.size)
        ) {
            return false;
        }

        this.setActorOccupancy(actor.coordinate, actor.size, false);
        actor.coordinate = destination;
        this.setActorOccupancy(actor.coordinate, actor.size, true);

        return true;
    }

    public placeGameObject(
        coordinate: SceneCoordinate,
        id: number,
        direction: CardinalDirection,
        sizeX: number,
        sizeY: number,
        collisionProfile: CollisionProfile,
    ): boolean {
        const footprint = getObjectFootprint(coordinate, direction, sizeX, sizeY);

        if (
            sizeX <= 0 ||
            sizeY <= 0 ||
            footprint.some((tileCoordinate) => this.getTile(tileCoordinate).gameObject)
        ) {
            return false;
        }

        const gameObject = {
            id,
            origin: coordinate,
            direction,
            sizeX,
            sizeY,
            footprint,
            collisionProfile,
        };

        for (const tileCoordinate of footprint) {
            const tile = this.getTile(tileCoordinate);
            tile.gameObject = gameObject;
            applyGameObjectCollision(tile, collisionProfile);
        }

        this.m_GameObjects.set(id, gameObject);

        return true;
    }

    public placeWallObject(
        coordinate: SceneCoordinate,
        id: number,
        direction: CardinalDirection,
        collisionProfile: CollisionProfile,
    ): boolean {
        const adjacent = getAdjacentCoordinate(coordinate, direction);
        const tile = this.getTile(coordinate);
        const adjacentTile = this.getTile(adjacent);

        if (tile.wallObject !== undefined) {
            return false;
        }

        const wallObject = {
            id,
            coordinate,
            directions: [direction],
            collisionProfiles: [collisionProfile],
        };

        tile.wallObject = wallObject;
        applyWallCollision(tile, adjacentTile, direction, collisionProfile);
        this.m_WallObjects.set(id, wallObject);

        return true;
    }

    public getActors(): ActorSnapshot[] {
        return [...this.m_Actors.values()]
            .filter(
                (actor): actor is ActorRecord & {
                    sceneId: number;
                    coordinate: SceneCoordinate;
                } => actor.sceneId !== undefined && actor.coordinate !== undefined,
            )
            .map((actor) => ({
                id: actor.id,
                kind: actor.kind,
                size: actor.size,
                speed: actor.speed,
                sceneId: actor.sceneId,
                coordinate: actor.coordinate,
                footprint: getActorFootprint(actor.coordinate, actor.size),
            }))
            .sort((a, b) => a.id - b.id);
    }

    public getScene(): SceneSnapshot {
        const tiles: TileSnapshot[] = [];

        for (let plane = 0; plane < 4; plane += 1) {
            for (let y = 0; y < 104; y += 1) {
                for (let x = 0; x < 104; x += 1) {
                    tiles.push(toTileSnapshot(this.getTile({ x, y, plane })));
                }
            }
        }

        return {
            sceneId: this.m_DefaultSceneId,
            width: 104,
            height: 104,
            planeCount: 4,
            tiles,
        };
    }

    public getWallObjects(): WallObjectSnapshot[] {
        return [...this.m_WallObjects.values()].sort((a, b) => a.id - b.id);
    }

    public getGameObjects(): GameObjectSnapshot[] {
        return [...this.m_GameObjects.values()].sort((a, b) => a.id - b.id);
    }

    private createActor(kind: "NPC" | "Player", size: number, speed: number): number {
        const id = this.m_NextActorId;
        this.m_NextActorId += 1;
        this.m_Actors.set(id, {
            id,
            kind,
            size: Math.max(1, size),
            speed: Math.max(0, speed),
        });
        return id;
    }

    private canStandOnMovementBlockers(
        coordinate: SceneCoordinate,
        actorSize: number,
    ): boolean {
        return getActorFootprint(coordinate, actorSize).every((tileCoordinate) => {
            const tile = this.getTile(tileCoordinate);
            return (
                !tile.flags.has("BlockMovementObject") &&
                !tile.flags.has("BlockMovementFull")
            );
        });
    }

    private canMove(
        from: SceneCoordinate,
        destination: SceneCoordinate,
        actorSize: number,
    ): boolean {
        const dx = Math.sign(destination.x - from.x);
        const dy = Math.sign(destination.y - from.y);
        let current = from;

        while (current.x !== destination.x || current.y !== destination.y) {
            const next = {
                x: current.x + (current.x === destination.x ? 0 : dx),
                y: current.y + (current.y === destination.y ? 0 : dy),
                plane: current.plane,
            };

            if (!this.canMoveStep(current, next, actorSize)) {
                return false;
            }

            current = next;
        }

        return true;
    }

    private canMoveStep(
        from: SceneCoordinate,
        to: SceneCoordinate,
        actorSize: number,
    ): boolean {
        if (!this.canStandOnMovementBlockers(to, actorSize)) {
            return false;
        }

        const dx = to.x - from.x;

        if (dx > 0) {
            return getEastEdge(from, actorSize).every((coordinate) => {
                const eastEdge = {
                    x: coordinate.x,
                    y: coordinate.y,
                    plane: coordinate.plane,
                };
                const nextTile = {
                    x: eastEdge.x + 1,
                    y: eastEdge.y,
                    plane: eastEdge.plane,
                };

                return (
                    !this.getTile(eastEdge).flags.has("BlockMovementEast") &&
                    !this.getTile(nextTile).flags.has("BlockMovementWest")
                );
            });
        }

        if (dx < 0) {
            return getWestEdge(from, actorSize).every((coordinate) => {
                const westEdge = {
                    x: coordinate.x,
                    y: coordinate.y,
                    plane: coordinate.plane,
                };
                const nextTile = {
                    x: westEdge.x - 1,
                    y: westEdge.y,
                    plane: westEdge.plane,
                };

                return (
                    !this.getTile(westEdge).flags.has("BlockMovementWest") &&
                    !this.getTile(nextTile).flags.has("BlockMovementEast")
                );
            });
        }

        return true;
    }

    private hasFinalNpcOccupancyConflict(
        current: SceneCoordinate,
        destination: SceneCoordinate,
        actorSize: number,
    ): boolean {
        return getActorFootprint(destination, actorSize).some((coordinate) => {
            if (isInsideFootprint(coordinate, current, actorSize)) {
                return false;
            }

            return this.getTile(coordinate).flags.has("Occupied");
        });
    }

    private setActorOccupancy(
        coordinate: SceneCoordinate,
        actorSize: number,
        occupied: boolean,
    ): void {
        for (const tileCoordinate of getActorFootprint(coordinate, actorSize)) {
            const tile = this.getTile(tileCoordinate);

            if (occupied) {
                tile.flags.add("Occupied");
            } else {
                tile.flags.delete("Occupied");
            }
        }
    }

    private getTile(coordinate: SceneCoordinate): TileRecord {
        const key = getKey(coordinate);
        const existing = this.m_Tiles.get(key);

        if (existing !== undefined) {
            return existing;
        }

        const tile = {
            coordinate,
            flags: new Set<TileFlag>(),
        };

        this.m_Tiles.set(key, tile);

        return tile;
    }
}

function toTileSnapshot(tile: TileRecord): TileSnapshot {
    return {
        coordinate: tile.coordinate,
        flags: [...tile.flags].sort(),
        wallObject: tile.wallObject,
        gameObject: tile.gameObject,
    };
}

function applyGameObjectCollision(
    tile: TileRecord,
    collisionProfile: CollisionProfile,
): void {
    if (collisionProfile.blocksMovement) {
        tile.flags.add("BlockMovementObject");
    }

    if (collisionProfile.blocksLineOfSight) {
        tile.flags.add("BlockLineOfSightFull");
    }
}

function applyWallCollision(
    tile: TileRecord,
    adjacentTile: TileRecord,
    direction: CardinalDirection,
    collisionProfile: CollisionProfile,
): void {
    if (!collisionProfile.blocksMovement) {
        return;
    }

    tile.flags.add(getMovementFlag(direction));
    adjacentTile.flags.add(getMovementFlag(getOppositeDirection(direction)));
}

function getMovementEdges(flags: TileFlag[]): CardinalDirection[] {
    return cardinalDirections.filter((direction) =>
        flags.includes(getMovementFlag(direction)),
    );
}

function getMovementFlag(direction: CardinalDirection): TileFlag {
    return `BlockMovement${direction}` as TileFlag;
}

function getOppositeDirection(direction: CardinalDirection): CardinalDirection {
    switch (direction) {
        case "North":
            return "South";
        case "East":
            return "West";
        case "South":
            return "North";
        case "West":
            return "East";
    }
}

function getAdjacentCoordinate(
    coordinate: SceneCoordinate,
    direction: CardinalDirection,
): SceneCoordinate {
    switch (direction) {
        case "North":
            return { x: coordinate.x, y: coordinate.y + 1, plane: coordinate.plane };
        case "East":
            return { x: coordinate.x + 1, y: coordinate.y, plane: coordinate.plane };
        case "South":
            return { x: coordinate.x, y: coordinate.y - 1, plane: coordinate.plane };
        case "West":
            return { x: coordinate.x - 1, y: coordinate.y, plane: coordinate.plane };
    }
}

function getObjectFootprint(
    coordinate: SceneCoordinate,
    direction: CardinalDirection,
    sizeX: number,
    sizeY: number,
): SceneCoordinate[] {
    const footprint: SceneCoordinate[] = [];
    const width = direction === "East" || direction === "West" ? sizeY : sizeX;
    const height = direction === "East" || direction === "West" ? sizeX : sizeY;

    for (let offsetY = 0; offsetY < height; offsetY += 1) {
        for (let offsetX = 0; offsetX < width; offsetX += 1) {
            footprint.push({
                x: coordinate.x + offsetX,
                y: coordinate.y + offsetY,
                plane: coordinate.plane,
            });
        }
    }

    return footprint;
}

function getActorFootprint(
    coordinate: SceneCoordinate,
    actorSize: number,
): SceneCoordinate[] {
    const footprint: SceneCoordinate[] = [];

    for (let offsetY = 0; offsetY < actorSize; offsetY += 1) {
        for (let offsetX = 0; offsetX < actorSize; offsetX += 1) {
            footprint.push({
                x: coordinate.x + offsetX,
                y: coordinate.y + offsetY,
                plane: coordinate.plane,
            });
        }
    }

    return footprint;
}

function getEastEdge(coordinate: SceneCoordinate, actorSize: number): SceneCoordinate[] {
    return getVerticalEdge(
        { x: coordinate.x + actorSize - 1, y: coordinate.y, plane: coordinate.plane },
        actorSize,
    );
}

function getWestEdge(coordinate: SceneCoordinate, actorSize: number): SceneCoordinate[] {
    return getVerticalEdge(coordinate, actorSize);
}

function getVerticalEdge(
    coordinate: SceneCoordinate,
    actorSize: number,
): SceneCoordinate[] {
    const edge: SceneCoordinate[] = [];

    for (let offsetY = 0; offsetY < actorSize; offsetY += 1) {
        edge.push({
            x: coordinate.x,
            y: coordinate.y + offsetY,
            plane: coordinate.plane,
        });
    }

    return edge;
}

function isInsideFootprint(
    coordinate: SceneCoordinate,
    origin: SceneCoordinate,
    actorSize: number,
): boolean {
    return (
        coordinate.plane === origin.plane &&
        coordinate.x >= origin.x &&
        coordinate.x < origin.x + actorSize &&
        coordinate.y >= origin.y &&
        coordinate.y < origin.y + actorSize
    );
}

function getKey(coordinate: SceneCoordinate): string {
    return `${coordinate.plane}:${coordinate.x}:${coordinate.y}`;
}
