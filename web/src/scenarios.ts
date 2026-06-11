export type ScenarioId =
    | "npc-movement"
    | "game-object-blocker"
    | "wall-blocker";

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
    coordinate: SceneCoordinate;
    footprint: SceneCoordinate[];
}

export interface BlockerSnapshot {
    coordinate: SceneCoordinate;
    label: string;
}

export interface ScenarioResult {
    id: ScenarioId;
    title: string;
    description: string;
    movementOutcome: string;
    actors: ActorSnapshot[];
    occupiedTiles: SceneCoordinate[];
    blockers: BlockerSnapshot[];
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
    coordinate?: SceneCoordinate;
}

interface TileRecord {
    occupied: boolean;
    blockMovementObject: boolean;
    blockMovementEast: boolean;
    blockMovementWest: boolean;
    blockerLabel?: string;
}

interface WorldApi {
    getDefaultSceneId(): number;
    createNpc(size: number, speed: number): number;
    createPlayer(size: number, speed: number): number;
    placeActor(actorId: number, sceneId: number, coordinate: SceneCoordinate): boolean;
    moveActorByDelta(actorId: number, dx: number, dy: number): boolean;
    placeGameObject(
        coordinate: SceneCoordinate,
        label: string,
        sizeX: number,
        sizeY: number,
    ): boolean;
    placeWallObject(
        coordinate: SceneCoordinate,
        direction: "East" | "West",
        label: string,
    ): boolean;
    getActor(actorId: number): ActorSnapshot | undefined;
    getOccupiedTiles(): SceneCoordinate[];
    getBlockers(): BlockerSnapshot[];
}

export const scenarioOptions: Array<{ id: ScenarioId; title: string }> = [
    { id: "npc-movement", title: "NPC Movement" },
    { id: "game-object-blocker", title: "Game Object Blocker" },
    { id: "wall-blocker", title: "Wall Object Blocker" },
];

export function runScenario(id: ScenarioId): ScenarioResult {
    switch (id) {
        case "game-object-blocker":
            return runGameObjectBlockerScenario();
        case "wall-blocker":
            return runWallBlockerScenario();
        case "npc-movement":
        default:
            return runNpcMovementScenario();
    }
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
        actorIds: [npcId],
        focus: { minX: 9, maxX: 15, minY: 9, maxY: 13 },
    });
}

function runGameObjectBlockerScenario(): ScenarioResult {
    const world = createWorld();
    const sceneId = world.getDefaultSceneId();
    const npcId = world.createNpc(1, 2);

    world.placeGameObject({ x: 12, y: 10, plane: 0 }, "Crate", 1, 1);
    world.placeActor(npcId, sceneId, { x: 10, y: 10, plane: 0 });
    const moved = world.moveActorByDelta(npcId, 2, 0);

    return buildResult(world, {
        id: "game-object-blocker",
        title: "Game Object Blocker",
        description: "A blocking game object prevents the NPC from crossing the tile.",
        movementOutcome: moved
            ? "MoveActorByDelta returned true."
            : "MoveActorByDelta returned false; the NPC stayed west of the blocker.",
        actorIds: [npcId],
        focus: { minX: 9, maxX: 13, minY: 9, maxY: 11 },
    });
}

function runWallBlockerScenario(): ScenarioResult {
    const world = createWorld();
    const sceneId = world.getDefaultSceneId();
    const npcId = world.createNpc(1, 1);

    world.placeWallObject({ x: 10, y: 10, plane: 0 }, "East", "East wall");
    world.placeActor(npcId, sceneId, { x: 10, y: 10, plane: 0 });
    const moved = world.moveActorByDelta(npcId, 1, 0);

    return buildResult(world, {
        id: "wall-blocker",
        title: "Wall Object Blocker",
        description: "A wall object blocks eastward movement between adjacent tiles.",
        movementOutcome: moved
            ? "MoveActorByDelta returned true."
            : "MoveActorByDelta returned false; the wall object blocked the east step.",
        actorIds: [npcId],
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
        actorIds: number[];
        focus: ScenarioResult["focus"];
    },
): ScenarioResult {
    return {
        id: options.id,
        title: options.title,
        description: options.description,
        movementOutcome: options.movementOutcome,
        actors: options.actorIds
            .map((actorId) => world.getActor(actorId))
            .filter((actor): actor is ActorSnapshot => actor !== undefined),
        occupiedTiles: world.getOccupiedTiles(),
        blockers: world.getBlockers(),
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
        label: string,
        sizeX: number,
        sizeY: number,
    ): boolean {
        for (let offsetX = 0; offsetX < sizeX; offsetX += 1) {
            for (let offsetY = 0; offsetY < sizeY; offsetY += 1) {
                const tile = this.getTile({
                    x: coordinate.x + offsetX,
                    y: coordinate.y + offsetY,
                    plane: coordinate.plane,
                });

                if (tile.blockMovementObject) {
                    return false;
                }
            }
        }

        for (let offsetX = 0; offsetX < sizeX; offsetX += 1) {
            for (let offsetY = 0; offsetY < sizeY; offsetY += 1) {
                const tile = this.getTile({
                    x: coordinate.x + offsetX,
                    y: coordinate.y + offsetY,
                    plane: coordinate.plane,
                });

                tile.blockMovementObject = true;
                tile.blockerLabel = label;
            }
        }

        return true;
    }

    public placeWallObject(
        coordinate: SceneCoordinate,
        direction: "East" | "West",
        label: string,
    ): boolean {
        const adjacent = {
            x: coordinate.x + (direction === "East" ? 1 : -1),
            y: coordinate.y,
            plane: coordinate.plane,
        };
        const tile = this.getTile(coordinate);
        const adjacentTile = this.getTile(adjacent);

        if (direction === "East") {
            tile.blockMovementEast = true;
            adjacentTile.blockMovementWest = true;
        } else {
            tile.blockMovementWest = true;
            adjacentTile.blockMovementEast = true;
        }

        tile.blockerLabel = label;
        adjacentTile.blockerLabel = label;

        return true;
    }

    public getActor(actorId: number): ActorSnapshot | undefined {
        const actor = this.m_Actors.get(actorId);

        if (actor === undefined || actor.coordinate === undefined) {
            return undefined;
        }

        return {
            id: actor.id,
            kind: actor.kind,
            size: actor.size,
            speed: actor.speed,
            coordinate: actor.coordinate,
            footprint: getFootprint(actor.coordinate, actor.size),
        };
    }

    public getOccupiedTiles(): SceneCoordinate[] {
        return this.getCoordinates((tile) => tile.occupied);
    }

    public getBlockers(): BlockerSnapshot[] {
        return this.getCoordinates(
            (tile) =>
                tile.blockMovementObject ||
                tile.blockMovementEast ||
                tile.blockMovementWest,
        ).map((coordinate) => ({
            coordinate,
            label: this.getTile(coordinate).blockerLabel ?? "Blocker",
        }));
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
        return getFootprint(coordinate, actorSize).every((tileCoordinate) => {
            const tile = this.getTile(tileCoordinate);
            return !tile.blockMovementObject;
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
                    !this.getTile(eastEdge).blockMovementEast &&
                    !this.getTile(nextTile).blockMovementWest
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
                    !this.getTile(westEdge).blockMovementWest &&
                    !this.getTile(nextTile).blockMovementEast
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
        return getFootprint(destination, actorSize).some((coordinate) => {
            if (isInsideFootprint(coordinate, current, actorSize)) {
                return false;
            }

            return this.getTile(coordinate).occupied;
        });
    }

    private setActorOccupancy(
        coordinate: SceneCoordinate,
        actorSize: number,
        occupied: boolean,
    ): void {
        for (const tileCoordinate of getFootprint(coordinate, actorSize)) {
            this.getTile(tileCoordinate).occupied = occupied;
        }
    }

    private getTile(coordinate: SceneCoordinate): TileRecord {
        const key = getKey(coordinate);
        const existing = this.m_Tiles.get(key);

        if (existing !== undefined) {
            return existing;
        }

        const tile = {
            occupied: false,
            blockMovementObject: false,
            blockMovementEast: false,
            blockMovementWest: false,
        };

        this.m_Tiles.set(key, tile);

        return tile;
    }

    private getCoordinates(predicate: (tile: TileRecord) => boolean): SceneCoordinate[] {
        return [...this.m_Tiles.entries()]
            .filter(([, tile]) => predicate(tile))
            .map(([key]) => parseKey(key))
            .sort((a, b) => a.y - b.y || a.x - b.x || a.plane - b.plane);
    }
}

function getFootprint(coordinate: SceneCoordinate, actorSize: number): SceneCoordinate[] {
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

function parseKey(key: string): SceneCoordinate {
    const [plane, x, y] = key.split(":").map(Number);

    return { plane, x, y };
}
