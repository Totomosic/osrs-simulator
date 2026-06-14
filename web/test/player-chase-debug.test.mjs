import assert from "node:assert/strict";
import { mkdir, writeFile } from "node:fs/promises";
import { dirname, resolve } from "node:path";
import { fileURLToPath, pathToFileURL } from "node:url";
import { build } from "esbuild";

const root = dirname(dirname(fileURLToPath(import.meta.url)));
const outfile = resolve(
    root,
    "../node_modules/.cache/osrs-simulator/player-chase-debug-test.mjs",
);

await mkdir(dirname(outfile), { recursive: true });
await build({
    entryPoints: [resolve(root, "src/playerChaseDebug.ts")],
    outfile,
    bundle: true,
    format: "esm",
    platform: "node",
    logLevel: "silent",
});
await writeFile(`${outfile}.stamp`, new Date().toISOString());

const {
    buildDebugTiles,
    clickDebugTile,
    createDefaultCamera,
    defaultFieldOfView,
    getCameraCenter,
    getSceneScreenCoordinate,
    maxFieldOfView,
    minFieldOfView,
    panCamera,
    readPlayerChaseDebugSnapshot,
    setCameraFieldOfView,
    setCameraMode,
    tileSize,
} = await import(pathToFileURL(outfile));

class FakePlayerChaseScenario {
    constructor() {
        this.tick = 0;
        this.running = false;
        this.lastClickBlocked = false;
        this.actionFeedback = { state: "none" };
        this.playerTarget = null;
        this.player = { id: 1, x: 8, y: 11, plane: 0, size: 1, speed: 2 };
        this.npc = { id: 2, x: 18, y: 20, plane: 0, size: 4, speed: 1 };
        this.secondNpc = { id: 3, x: 6, y: 7, plane: 0, size: 1, speed: 1 };
        this.width = 104;
        this.height = 104;
    }

    GetSnapshotJson() {
        const tiles = [];

        for (let x = 12; x <= 14; x += 1) {
            for (let y = 4; y <= 6; y += 1) {
                tiles.push({
                    coordinate: { x, y, plane: 0 },
                    flags: ["BlockMovementObject", "BlockLineOfSightFull"],
                    gameObject: {
                        id: 200,
                        origin: { x: 12, y: 4, plane: 0 },
                        sizeX: 3,
                        sizeY: 3,
                    },
                });
            }
        }

        return JSON.stringify({
            name: "Player Chase",
            tick: this.tick,
            running: this.running,
            blockedClick: this.lastClickBlocked,
            actionFeedback: this.actionFeedback,
            scene: {
                id: 1,
                width: this.width,
                height: this.height,
                planeCount: 4,
            },
            player: {
                id: this.player.id,
                kind: "Player",
                coordinate: {
                    x: this.player.x,
                    y: this.player.y,
                    plane: this.player.plane,
                },
                size: this.player.size,
                speed: this.player.speed,
                movementTarget:
                    this.playerTarget === null
                        ? null
                        : {
                              kind: "SceneCoordinate",
                              coordinate: this.playerTarget,
                          },
            },
            npc: {
                id: this.npc.id,
                kind: "NPC",
                coordinate: { x: this.npc.x, y: this.npc.y, plane: this.npc.plane },
                size: this.npc.size,
                speed: this.npc.speed,
                movementTarget: {
                    kind: "Actor",
                    actorId: this.player.id,
                    label: "Player #1",
                },
            },
            npcs: [
                {
                    id: this.npc.id,
                    kind: "NPC",
                    coordinate: {
                        x: this.npc.x,
                        y: this.npc.y,
                        plane: this.npc.plane,
                    },
                    size: this.npc.size,
                    speed: this.npc.speed,
                    movementTarget: {
                        kind: "Actor",
                        actorId: this.player.id,
                        label: "Player #1",
                    },
                },
                {
                    id: this.secondNpc.id,
                    kind: "NPC",
                    coordinate: {
                        x: this.secondNpc.x,
                        y: this.secondNpc.y,
                        plane: this.secondNpc.plane,
                    },
                    size: this.secondNpc.size,
                    speed: this.secondNpc.speed,
                    movementTarget: null,
                },
            ],
            selectedNpcId: this.npc.id,
            tiles,
        });
    }

    ClickSceneCoordinate(x, y, plane) {
        if (plane === 0 && x >= 12 && x <= 14 && y >= 4 && y <= 6) {
            this.lastClickBlocked = true;
            this.actionFeedback = { state: "blocked-movement" };
            return false;
        }

        this.lastClickBlocked = false;
        this.actionFeedback = { state: "none" };
        this.playerTarget = { x, y, plane };
        return true;
    }

    PlaceNpc() {
        this.actionFeedback = { state: "placement-failure" };
        return false;
    }

    RemoveNpc() {
        this.actionFeedback = { state: "removal-failure" };
        return false;
    }
}

{
    const scenario = new FakePlayerChaseScenario();
    const snapshot = readPlayerChaseDebugSnapshot(
        scenario,
        "Follow Player",
        defaultFieldOfView,
    );

    assert.equal(snapshot.name, "Player Chase");
    assert.equal(snapshot.tick, 0);
    assert.equal(snapshot.running, false);
    assert.equal(snapshot.cameraMode, "Follow Player");
    assert.deepEqual(snapshot.player.coordinate, { x: 8, y: 11, plane: 0 });
    assert.equal(snapshot.player.size, 1);
    assert.equal(snapshot.player.speed, 2);
    assert.equal(snapshot.player.movementTarget, null);
    assert.deepEqual(snapshot.npc.coordinate, { x: 18, y: 20, plane: 0 });
    assert.equal(snapshot.npc.size, 4);
    assert.equal(snapshot.npc.speed, 1);
    assert.deepEqual(snapshot.npc.movementTarget, {
        kind: "Actor",
        actorId: 1,
        label: "Player #1",
    });
    assert.equal(snapshot.npcs.length, 2);
    assert.equal(snapshot.selectedNpc.id, 2);
    assert.equal(
        snapshot.tiles.filter((tile) =>
            tile.flags.includes("BlockMovementObject"),
        ).length,
        9,
    );
}

{
    const scenario = new FakePlayerChaseScenario();
    const snapshot = readPlayerChaseDebugSnapshot(
        scenario,
        "Follow Player",
        defaultFieldOfView,
    );
    const camera = createDefaultCamera(snapshot);
    const npcCamera = setCameraMode(camera, "Follow NPC", snapshot);
    const freeCamera = panCamera(npcCamera, "east", snapshot);
    const zoomed = setCameraFieldOfView(
        npcCamera,
        maxFieldOfView + 100,
        snapshot,
    );
    const zoomedIn = setCameraFieldOfView(
        zoomed,
        minFieldOfView - 100,
        snapshot,
    );

    assert.deepEqual(getCameraCenter(camera, snapshot), { x: 8, y: 11, plane: 0 });
    assert.deepEqual(getCameraCenter(npcCamera, snapshot), {
        x: 18,
        y: 20,
        plane: 0,
    });
    assert.equal(freeCamera.mode, "Free Camera");
    assert.deepEqual(getCameraCenter(freeCamera, snapshot), {
        x: 19,
        y: 20,
        plane: 0,
    });
    assert.equal(zoomed.fieldOfView, maxFieldOfView);
    assert.equal(zoomedIn.fieldOfView, minFieldOfView);
}

{
    const scenario = new FakePlayerChaseScenario();
    const clicked = clickDebugTile(scenario, {
        key: "0:10:11",
        coordinate: { x: 10, y: 11, plane: 0 },
        kind: "empty",
        flags: [],
    });
    const snapshot = readPlayerChaseDebugSnapshot(
        scenario,
        "Follow Player",
        defaultFieldOfView,
    );

    assert.equal(clicked, true);
    assert.deepEqual(snapshot.player.movementTarget, {
        kind: "SceneCoordinate",
        coordinate: { x: 10, y: 11, plane: 0 },
    });
    assert.equal(snapshot.blockedClick, false);
    assert.deepEqual(snapshot.actionFeedback, { state: "none" });
}

{
    const scenario = new FakePlayerChaseScenario();
    scenario.playerTarget = { x: 10, y: 11, plane: 0 };

    const clicked = clickDebugTile(scenario, {
        key: "0:12:4",
        coordinate: { x: 12, y: 4, plane: 0 },
        kind: "game-object",
        flags: ["BlockMovementObject"],
    });
    const snapshot = readPlayerChaseDebugSnapshot(
        scenario,
        "Follow NPC",
        defaultFieldOfView,
    );

    assert.equal(clicked, false);
    assert.equal(snapshot.blockedClick, true);
    assert.deepEqual(snapshot.actionFeedback, { state: "blocked-movement" });
    assert.deepEqual(snapshot.player.movementTarget, {
        kind: "SceneCoordinate",
        coordinate: { x: 10, y: 11, plane: 0 },
    });
}

{
    const scenario = new FakePlayerChaseScenario();

    assert.equal(scenario.PlaceNpc(), false);

    let snapshot = readPlayerChaseDebugSnapshot(
        scenario,
        "Follow Player",
        defaultFieldOfView,
    );

    assert.deepEqual(snapshot.actionFeedback, { state: "placement-failure" });

    assert.equal(scenario.RemoveNpc(), false);

    snapshot = readPlayerChaseDebugSnapshot(
        scenario,
        "Follow Player",
        defaultFieldOfView,
    );

    assert.deepEqual(snapshot.actionFeedback, { state: "removal-failure" });
}

{
    const scenario = new FakePlayerChaseScenario();
    scenario.width = 24;
    scenario.height = 24;
    const snapshot = readPlayerChaseDebugSnapshot(
        scenario,
        "Free Camera",
        defaultFieldOfView,
    );

    const tiles = buildDebugTiles(
        snapshot,
        { x: 23, y: 23, plane: 0 },
        defaultFieldOfView,
    );
    const xs = tiles.map((tile) => tile.coordinate.x);
    const ys = tiles.map((tile) => tile.coordinate.y);

    assert.equal(tiles.length, defaultFieldOfView * defaultFieldOfView);
    assert.equal(Math.min(...xs), 4);
    assert.equal(Math.max(...xs), 23);
    assert.equal(Math.min(...ys), 4);
    assert.equal(Math.max(...ys), 23);
}

{
    const scenario = new FakePlayerChaseScenario();
    const snapshot = readPlayerChaseDebugSnapshot(
        scenario,
        "Follow NPC",
        defaultFieldOfView,
    );
    const tiles = buildDebugTiles(
        snapshot,
        { x: 14, y: 14, plane: 0 },
        defaultFieldOfView,
    );
    const kinds = new Set(tiles.map((tile) => tile.kind));
    const objectTile = tiles.find(
        (tile) => tile.coordinate.x === 12 && tile.coordinate.y === 4,
    );
    const npcTile = tiles.find(
        (tile) => tile.coordinate.x === 21 && tile.coordinate.y === 23,
    );
    const playerScreen = getSceneScreenCoordinate(
        { x: 8, y: 11, plane: 0 },
        tiles,
    );
    const secondNpcTile = tiles.find(
        (tile) => tile.coordinate.x === 6 && tile.coordinate.y === 7,
    );

    assert.ok(kinds.has("player"));
    assert.ok(kinds.has("npc"));
    assert.equal(objectTile.kind, "game-object");
    assert.equal(npcTile.kind, "npc");
    assert.equal(secondNpcTile.kind, "npc");
    assert.deepEqual(objectTile.flags, [
        "BlockMovementObject",
        "BlockLineOfSightFull",
    ]);
    assert.equal(playerScreen.x % tileSize, 0);
    assert.equal(playerScreen.y % tileSize, 0);
}
