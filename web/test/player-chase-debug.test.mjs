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
    createDefaultCamera,
    defaultFieldOfView,
    getSceneScreenCoordinate,
    getCameraCenter,
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
        this.playerTarget = null;
        this.player = { id: 1, x: 15, y: 10, plane: 0 };
        this.npc = { id: 2, x: 10, y: 10, plane: 0, size: 2 };
        this.width = 104;
        this.height = 104;
    }

    GetTick() {
        return this.tick;
    }

    IsRunning() {
        return this.running;
    }

    GetPlayerId() {
        return this.player.id;
    }

    GetPlayerX() {
        return this.player.x;
    }

    GetPlayerY() {
        return this.player.y;
    }

    GetPlayerPlane() {
        return this.player.plane;
    }

    HasPlayerMovementTarget() {
        return this.playerTarget !== null;
    }

    GetPlayerMovementTargetX() {
        return this.playerTarget.x;
    }

    GetPlayerMovementTargetY() {
        return this.playerTarget.y;
    }

    GetPlayerMovementTargetPlane() {
        return this.playerTarget.plane;
    }

    GetNpcId() {
        return this.npc.id;
    }

    GetNpcX() {
        return this.npc.x;
    }

    GetNpcY() {
        return this.npc.y;
    }

    GetNpcPlane() {
        return this.npc.plane;
    }

    GetNpcSize() {
        return this.npc.size;
    }

    HasNpcMovementTarget() {
        return true;
    }

    GetNpcMovementTargetLabel() {
        return "Player #1";
    }

    WasLastClickBlocked() {
        return this.lastClickBlocked;
    }

    GetSceneWidth() {
        return this.width;
    }

    GetSceneHeight() {
        return this.height;
    }

    IsPlayerTile(x, y, plane) {
        return x === this.player.x && y === this.player.y && plane === 0;
    }

    IsNpcTile(x, y, plane) {
        return (
            plane === this.npc.plane &&
            x >= this.npc.x &&
            x < this.npc.x + this.npc.size &&
            y >= this.npc.y &&
            y < this.npc.y + this.npc.size
        );
    }

    IsGameObjectTile(x, y, plane) {
        return plane === 0 && x >= 12 && x <= 13 && y >= 10 && y <= 11;
    }
}

{
    const scenario = new FakePlayerChaseScenario();
    const snapshot = readPlayerChaseDebugSnapshot(
        scenario,
        "Follow Player",
        defaultFieldOfView,
    );

    assert.equal(snapshot.tick, 0);
    assert.equal(snapshot.running, false);
    assert.equal(snapshot.cameraMode, "Follow Player");
    assert.equal(snapshot.fieldOfView, 20);
    assert.deepEqual(snapshot.player.coordinate, { x: 15, y: 10, plane: 0 });
    assert.equal(snapshot.player.movementTarget, null);
    assert.deepEqual(snapshot.npc.coordinate, { x: 10, y: 10, plane: 0 });
    assert.equal(snapshot.npc.size, 2);
    assert.equal(snapshot.npc.movementTarget, "Player #1");
    assert.match(snapshot.noPathfindingNote, /Direct movement only/);
}

{
    const scenario = new FakePlayerChaseScenario();
    const snapshot = readPlayerChaseDebugSnapshot(
        scenario,
        "Follow Player",
        defaultFieldOfView,
    );
    const camera = createDefaultCamera(snapshot);

    assert.equal(camera.mode, "Follow Player");
    assert.equal(camera.fieldOfView, 20);
    assert.deepEqual(getCameraCenter(camera, snapshot), {
        x: 15,
        y: 10,
        plane: 0,
    });
}

{
    const scenario = new FakePlayerChaseScenario();
    const snapshot = readPlayerChaseDebugSnapshot(
        scenario,
        "Follow Player",
        defaultFieldOfView,
    );
    const playerCamera = createDefaultCamera(snapshot);
    const npcCamera = setCameraMode(playerCamera, "Follow NPC", snapshot, scenario);
    const freeCamera = setCameraMode(npcCamera, "Free Camera", snapshot, scenario);

    assert.equal(npcCamera.mode, "Follow NPC");
    assert.deepEqual(getCameraCenter(npcCamera, snapshot), {
        x: 10,
        y: 10,
        plane: 0,
    });
    assert.equal(freeCamera.mode, "Free Camera");
    assert.deepEqual(getCameraCenter(freeCamera, snapshot), {
        x: 10,
        y: 10,
        plane: 0,
    });
}

{
    const scenario = new FakePlayerChaseScenario();
    const snapshot = readPlayerChaseDebugSnapshot(
        scenario,
        "Follow Player",
        defaultFieldOfView,
    );
    const camera = createDefaultCamera(snapshot);
    scenario.player = { ...scenario.player, x: 20, y: 12 };
    const movedSnapshot = readPlayerChaseDebugSnapshot(
        scenario,
        "Follow Player",
        defaultFieldOfView,
    );
    const freeCamera = setCameraMode(
        camera,
        "Free Camera",
        movedSnapshot,
        scenario,
    );

    assert.deepEqual(getCameraCenter(freeCamera, movedSnapshot), {
        x: 20,
        y: 12,
        plane: 0,
    });
}

{
    const scenario = new FakePlayerChaseScenario();
    const snapshot = readPlayerChaseDebugSnapshot(
        scenario,
        "Follow Player",
        defaultFieldOfView,
    );
    const camera = createDefaultCamera(snapshot);
    const panned = panCamera(camera, "east", snapshot, scenario);

    assert.equal(panned.mode, "Free Camera");
    assert.deepEqual(getCameraCenter(panned, snapshot), {
        x: 16,
        y: 10,
        plane: 0,
    });
}

{
    const scenario = new FakePlayerChaseScenario();
    const snapshot = readPlayerChaseDebugSnapshot(
        scenario,
        "Follow Player",
        defaultFieldOfView,
    );
    const camera = createDefaultCamera(snapshot);
    const zoomed = setCameraFieldOfView(
        setCameraMode(camera, "Follow NPC", snapshot, scenario),
        maxFieldOfView + 100,
        snapshot,
        scenario,
    );
    const zoomedIn = setCameraFieldOfView(
        zoomed,
        minFieldOfView - 100,
        snapshot,
        scenario,
    );

    assert.equal(zoomed.mode, "Follow NPC");
    assert.equal(zoomed.fieldOfView, maxFieldOfView);
    assert.equal(zoomedIn.mode, "Follow NPC");
    assert.equal(zoomedIn.fieldOfView, minFieldOfView);
}

{
    const scenario = new FakePlayerChaseScenario();
    scenario.lastClickBlocked = true;
    scenario.playerTarget = { x: 16, y: 10, plane: 0 };

    const snapshot = readPlayerChaseDebugSnapshot(
        scenario,
        "Fixed Scene",
        defaultFieldOfView,
    );

    assert.equal(snapshot.blockedClick, true);
    assert.deepEqual(snapshot.player.movementTarget, { x: 16, y: 10, plane: 0 });
}

{
    const scenario = new FakePlayerChaseScenario();
    scenario.width = 24;
    scenario.height = 24;

    const tiles = buildDebugTiles(
        scenario,
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
    const tiles = buildDebugTiles(
        scenario,
        { x: 15, y: 10, plane: 0 },
        defaultFieldOfView,
    );
    const kinds = new Set(tiles.map((tile) => tile.kind));
    const playerScreen = getSceneScreenCoordinate(
        { x: 15, y: 10, plane: 0 },
        tiles,
    );
    const objectTile = tiles.find(
        (tile) => tile.coordinate.x === 12 && tile.coordinate.y === 10,
    );

    assert.equal(tiles.length, defaultFieldOfView * defaultFieldOfView);
    assert.ok(kinds.has("player"));
    assert.ok(kinds.has("npc"));
    assert.equal(objectTile.kind, "game-object");
    assert.equal(playerScreen.x % tileSize, 0);
    assert.equal(playerScreen.y % tileSize, 0);
}
