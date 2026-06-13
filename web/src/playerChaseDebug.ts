import type { DevelopmentPlayerChaseScenario } from "./wasm/EngineModule";

export type CameraMode = "Follow Player" | "Follow NPC" | "Free Camera";
export type CameraPanDirection = "north" | "east" | "south" | "west";

export interface SceneCoordinate {
    x: number;
    y: number;
    plane: number;
}

export interface MovementTargetSnapshot {
    kind: "SceneCoordinate" | "Actor";
    coordinate?: SceneCoordinate;
    actorId?: number;
    label?: string;
}

export interface ActorSnapshot {
    id: number;
    kind: "Player" | "NPC";
    coordinate: SceneCoordinate;
    size: number;
    speed: number;
    movementTarget: MovementTargetSnapshot | null;
}

export interface SnapshotTile {
    coordinate: SceneCoordinate;
    flags: string[];
    gameObject?: {
        id: number;
        origin: SceneCoordinate;
        sizeX: number;
        sizeY: number;
    };
}

export interface EnginePlayerChaseSnapshot {
    name: "Player Chase";
    tick: number;
    running: boolean;
    blockedClick: boolean;
    scene: {
        id: number;
        width: number;
        height: number;
        planeCount: number;
    };
    player: ActorSnapshot;
    npc: ActorSnapshot;
    tiles: SnapshotTile[];
}

export interface PlayerChaseDebugSnapshot extends EnginePlayerChaseSnapshot {
    cameraMode: CameraMode;
    fieldOfView: number;
    noPathfindingNote: string;
}

export interface DebugTile {
    key: string;
    coordinate: SceneCoordinate;
    kind: "empty" | "game-object" | "player" | "npc";
    flags: string[];
}

export interface ScreenCoordinate {
    x: number;
    y: number;
}

export interface CameraState {
    mode: CameraMode;
    center: SceneCoordinate;
    fieldOfView: number;
}

export const tileSize = 28;
export const defaultFieldOfView = 20;
export const minFieldOfView = 8;
export const maxFieldOfView = 40;

export function readPlayerChaseDebugSnapshot(
    scenario: DevelopmentPlayerChaseScenario,
    cameraMode: CameraMode,
    fieldOfView: number,
): PlayerChaseDebugSnapshot {
    const engineSnapshot = JSON.parse(
        scenario.GetSnapshotJson(),
    ) as EnginePlayerChaseSnapshot;

    return {
        ...engineSnapshot,
        cameraMode,
        fieldOfView,
        noPathfindingNote:
            "Direct movement only: the NPC keeps its Player movement target when it stops at Edge Adjacency; the Game Object remains available for blocked-click checks.",
    };
}

export function buildDebugTiles(
    snapshot: PlayerChaseDebugSnapshot,
    center: SceneCoordinate,
    fieldOfView: number,
): DebugTile[] {
    const bounds = getVisibleBounds(snapshot, center, fieldOfView);
    const tilesByCoordinate = new Map(
        snapshot.tiles.map((tile) => [getCoordinateKey(tile.coordinate), tile]),
    );
    const tiles: DebugTile[] = [];

    for (let y = bounds.minY; y <= bounds.maxY; y += 1) {
        for (let x = bounds.minX; x <= bounds.maxX; x += 1) {
            const coordinate = { x, y, plane: center.plane };
            const snapshotTile = tilesByCoordinate.get(getCoordinateKey(coordinate));
            tiles.push({
                key: getCoordinateKey(coordinate),
                coordinate,
                kind: getTileKind(snapshot, coordinate, snapshotTile),
                flags: snapshotTile?.flags ?? [],
            });
        }
    }

    return tiles;
}

export function createDefaultCamera(
    snapshot: PlayerChaseDebugSnapshot,
): CameraState {
    return {
        mode: "Follow Player",
        center: snapshot.player.coordinate,
        fieldOfView: defaultFieldOfView,
    };
}

export function getCameraCenter(
    camera: CameraState,
    snapshot: PlayerChaseDebugSnapshot,
): SceneCoordinate {
    if (camera.mode === "Follow Player") {
        return snapshot.player.coordinate;
    }

    if (camera.mode === "Follow NPC") {
        return snapshot.npc.coordinate;
    }

    return camera.center;
}

export function setCameraMode(
    camera: CameraState,
    mode: CameraMode,
    snapshot: PlayerChaseDebugSnapshot,
): CameraState {
    const center =
        mode === "Free Camera"
            ? getCameraCenter(camera, snapshot)
            : getCameraCenter({ ...camera, mode }, snapshot);

    return clampCameraToScene(
        {
            ...camera,
            mode,
            center,
        },
        snapshot,
    );
}

export function panCamera(
    camera: CameraState,
    direction: CameraPanDirection,
    snapshot: PlayerChaseDebugSnapshot,
): CameraState {
    const center = getCameraCenter(camera, snapshot);
    const delta = getPanDelta(direction);

    return clampCameraToScene(
        {
            ...camera,
            mode: "Free Camera",
            center: {
                x: center.x + delta.x,
                y: center.y + delta.y,
                plane: center.plane,
            },
        },
        snapshot,
    );
}

export function setCameraFieldOfView(
    camera: CameraState,
    fieldOfView: number,
    snapshot: PlayerChaseDebugSnapshot,
): CameraState {
    return clampCameraToScene(
        {
            ...camera,
            center: getCameraCenter(camera, snapshot),
            fieldOfView: clampFieldOfView(fieldOfView),
        },
        snapshot,
    );
}

export function getSceneScreenCoordinate(
    coordinate: SceneCoordinate,
    visibleTiles: DebugTile[],
): ScreenCoordinate {
    const minX = Math.min(...visibleTiles.map((tile) => tile.coordinate.x));
    const maxY = Math.max(...visibleTiles.map((tile) => tile.coordinate.y));

    return {
        x: (coordinate.x - minX) * tileSize,
        y: (maxY - coordinate.y) * tileSize,
    };
}

export function clickDebugTile(
    scenario: DevelopmentPlayerChaseScenario,
    tile: DebugTile,
): boolean {
    return scenario.ClickSceneCoordinate(
        tile.coordinate.x,
        tile.coordinate.y,
        tile.coordinate.plane,
    );
}

function getTileKind(
    snapshot: PlayerChaseDebugSnapshot,
    coordinate: SceneCoordinate,
    tile: SnapshotTile | undefined,
): DebugTile["kind"] {
    if (isActorFootprintTile(snapshot.player, coordinate)) {
        return "player";
    }

    if (isActorFootprintTile(snapshot.npc, coordinate)) {
        return "npc";
    }

    if (tile?.gameObject !== undefined) {
        return "game-object";
    }

    return "empty";
}

function isActorFootprintTile(
    actor: ActorSnapshot,
    coordinate: SceneCoordinate,
): boolean {
    return (
        coordinate.plane === actor.coordinate.plane &&
        coordinate.x >= actor.coordinate.x &&
        coordinate.x < actor.coordinate.x + actor.size &&
        coordinate.y >= actor.coordinate.y &&
        coordinate.y < actor.coordinate.y + actor.size
    );
}

function getVisibleBounds(
    snapshot: PlayerChaseDebugSnapshot,
    center: SceneCoordinate,
    fieldOfView: number,
): { minX: number; maxX: number; minY: number; maxY: number } {
    const columnCount = Math.min(fieldOfView, snapshot.scene.width);
    const rowCount = Math.min(fieldOfView, snapshot.scene.height);

    return {
        ...getAxisBounds(center.x, columnCount, snapshot.scene.width),
        ...renameAxisBounds(
            getAxisBounds(center.y, rowCount, snapshot.scene.height),
        ),
    };
}

function getAxisBounds(
    center: number,
    visibleCount: number,
    sceneSize: number,
): { minX: number; maxX: number } {
    const half = Math.floor(visibleCount / 2);
    const maxStart = Math.max(0, sceneSize - visibleCount);
    const start = Math.min(Math.max(0, center - half), maxStart);

    return {
        minX: start,
        maxX: Math.min(sceneSize - 1, start + visibleCount - 1),
    };
}

function renameAxisBounds(bounds: {
    minX: number;
    maxX: number;
}): { minY: number; maxY: number } {
    return {
        minY: bounds.minX,
        maxY: bounds.maxX,
    };
}

function clampCameraToScene(
    camera: CameraState,
    snapshot: PlayerChaseDebugSnapshot,
): CameraState {
    return {
        ...camera,
        center: clampCoordinateToScene(camera.center, snapshot),
    };
}

function clampCoordinateToScene(
    coordinate: SceneCoordinate,
    snapshot: PlayerChaseDebugSnapshot,
): SceneCoordinate {
    return {
        x: clamp(coordinate.x, 0, snapshot.scene.width - 1),
        y: clamp(coordinate.y, 0, snapshot.scene.height - 1),
        plane: coordinate.plane,
    };
}

function clampFieldOfView(fieldOfView: number): number {
    return clamp(fieldOfView, minFieldOfView, maxFieldOfView);
}

function getPanDelta(direction: CameraPanDirection): { x: number; y: number } {
    switch (direction) {
        case "north":
            return { x: 0, y: 1 };
        case "east":
            return { x: 1, y: 0 };
        case "south":
            return { x: 0, y: -1 };
        case "west":
            return { x: -1, y: 0 };
    }
}

function getCoordinateKey(coordinate: SceneCoordinate): string {
    return `${coordinate.plane}:${coordinate.x}:${coordinate.y}`;
}

function clamp(value: number, min: number, max: number): number {
    return Math.min(Math.max(value, min), max);
}
