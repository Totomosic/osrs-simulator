import type { DevelopmentPlayerChaseScenario } from "./wasm/EngineModule";

export type CameraMode = "Follow Player" | "Follow NPC" | "Free Camera";
export type CameraPanDirection = "north" | "east" | "south" | "west";

export interface SceneCoordinate {
    x: number;
    y: number;
    plane: number;
}

export interface PlayerChaseDebugSnapshot {
    tick: number;
    running: boolean;
    cameraMode: CameraMode;
    fieldOfView: number;
    player: {
        id: number;
        coordinate: SceneCoordinate;
        movementTarget: SceneCoordinate | null;
    };
    npc: {
        id: number;
        coordinate: SceneCoordinate;
        size: number;
        movementTarget: string;
    };
    blockedClick: boolean;
    noPathfindingNote: string;
}

export interface DebugTile {
    key: string;
    coordinate: SceneCoordinate;
    kind: "empty" | "game-object" | "player" | "npc";
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
    return {
        tick: Number(scenario.GetTick()),
        running: scenario.IsRunning(),
        cameraMode,
        fieldOfView,
        player: {
            id: Number(scenario.GetPlayerId()),
            coordinate: {
                x: scenario.GetPlayerX(),
                y: scenario.GetPlayerY(),
                plane: scenario.GetPlayerPlane(),
            },
            movementTarget: scenario.HasPlayerMovementTarget()
                ? {
                      x: scenario.GetPlayerMovementTargetX(),
                      y: scenario.GetPlayerMovementTargetY(),
                      plane: scenario.GetPlayerMovementTargetPlane(),
                  }
                : null,
        },
        npc: {
            id: Number(scenario.GetNpcId()),
            coordinate: {
                x: scenario.GetNpcX(),
                y: scenario.GetNpcY(),
                plane: scenario.GetNpcPlane(),
            },
            size: scenario.GetNpcSize(),
            movementTarget: scenario.HasNpcMovementTarget()
                ? scenario.GetNpcMovementTargetLabel()
                : "None",
        },
        blockedClick: scenario.WasLastClickBlocked(),
        noPathfindingNote:
            "Direct movement only: the NPC keeps its Player movement target but stops when the Game Object blocks the straight chase.",
    };
}

export function buildDebugTiles(
    scenario: DevelopmentPlayerChaseScenario,
    center: SceneCoordinate,
    fieldOfView: number,
): DebugTile[] {
    const bounds = getVisibleBounds(scenario, center, fieldOfView);
    const tiles: DebugTile[] = [];

    for (let y = bounds.minY; y <= bounds.maxY; y += 1) {
        for (let x = bounds.minX; x <= bounds.maxX; x += 1) {
            const coordinate = { x, y, plane: center.plane };
            tiles.push({
                key: `${coordinate.plane}:${coordinate.x}:${coordinate.y}`,
                coordinate,
                kind: getTileKind(scenario, coordinate),
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
    scenario: DevelopmentPlayerChaseScenario,
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
        scenario,
    );
}

export function panCamera(
    camera: CameraState,
    direction: CameraPanDirection,
    snapshot: PlayerChaseDebugSnapshot,
    scenario: DevelopmentPlayerChaseScenario,
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
        scenario,
    );
}

export function setCameraFieldOfView(
    camera: CameraState,
    fieldOfView: number,
    snapshot: PlayerChaseDebugSnapshot,
    scenario: DevelopmentPlayerChaseScenario,
): CameraState {
    return clampCameraToScene(
        {
            ...camera,
            center: getCameraCenter(camera, snapshot),
            fieldOfView: clampFieldOfView(fieldOfView),
        },
        scenario,
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
    scenario: DevelopmentPlayerChaseScenario,
    coordinate: SceneCoordinate,
): DebugTile["kind"] {
    if (scenario.IsPlayerTile(coordinate.x, coordinate.y, coordinate.plane)) {
        return "player";
    }

    if (scenario.IsNpcTile(coordinate.x, coordinate.y, coordinate.plane)) {
        return "npc";
    }

    if (scenario.IsGameObjectTile(coordinate.x, coordinate.y, coordinate.plane)) {
        return "game-object";
    }

    return "empty";
}

function getVisibleBounds(
    scenario: DevelopmentPlayerChaseScenario,
    center: SceneCoordinate,
    fieldOfView: number,
): { minX: number; maxX: number; minY: number; maxY: number } {
    const width = scenario.GetSceneWidth();
    const height = scenario.GetSceneHeight();
    const columnCount = Math.min(fieldOfView, width);
    const rowCount = Math.min(fieldOfView, height);

    return {
        ...getAxisBounds(center.x, columnCount, width),
        ...renameAxisBounds(getAxisBounds(center.y, rowCount, height)),
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
    scenario: DevelopmentPlayerChaseScenario,
): CameraState {
    return {
        ...camera,
        center: clampCoordinateToScene(camera.center, scenario),
    };
}

function clampCoordinateToScene(
    coordinate: SceneCoordinate,
    scenario: DevelopmentPlayerChaseScenario,
): SceneCoordinate {
    return {
        x: clamp(coordinate.x, 0, scenario.GetSceneWidth() - 1),
        y: clamp(coordinate.y, 0, scenario.GetSceneHeight() - 1),
        plane: coordinate.plane,
    };
}

function clampFieldOfView(fieldOfView: number): number {
    return clamp(fieldOfView, minFieldOfView, maxFieldOfView);
}

function getPanDelta(
    direction: CameraPanDirection,
): { x: number; y: number } {
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

function clamp(value: number, min: number, max: number): number {
    return Math.min(Math.max(value, min), max);
}
