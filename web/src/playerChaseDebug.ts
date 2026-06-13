import type { DevelopmentPlayerChaseScenario } from "./wasm/EngineModule";

export type CameraMode = "Follow Player" | "Fixed Scene";

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

export const tileSize = 28;
export const defaultFieldOfView = 12;

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
    const half = Math.floor(fieldOfView / 2);
    const minX = Math.max(0, center.x - half);
    const minY = Math.max(0, center.y - half);
    const maxX = Math.min(scenario.GetSceneWidth() - 1, minX + fieldOfView - 1);
    const maxY = Math.min(scenario.GetSceneHeight() - 1, minY + fieldOfView - 1);
    const tiles: DebugTile[] = [];

    for (let y = minY; y <= maxY; y += 1) {
        for (let x = minX; x <= maxX; x += 1) {
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
