import assert from "node:assert/strict";
import { mkdir, writeFile } from "node:fs/promises";
import { dirname, resolve } from "node:path";
import { fileURLToPath, pathToFileURL } from "node:url";
import { build } from "esbuild";

const root = dirname(dirname(fileURLToPath(import.meta.url)));
const outfile = resolve(
    root,
    "../node_modules/.cache/osrs-simulator/engine-module-loader-test.mjs",
);

await mkdir(dirname(outfile), { recursive: true });
await build({
    entryPoints: [resolve(root, "src/scenarios.ts")],
    outfile,
    bundle: true,
    format: "esm",
    platform: "node",
    logLevel: "silent",
});

const { createPlayerChaseScenario, loadEngineModule, registeredScenarios } =
    await import(pathToFileURL(outfile));

class FakeScene {
    constructor() {
        this.gameObjects = new Set();
    }

    PlaceGameObject(coordinate, id, direction, sizeX, sizeY, collisionProfile) {
        assert.equal(id, 200);
        assert.equal(direction, "North");
        assert.equal(sizeX, 3);
        assert.equal(sizeY, 3);
        assert.deepEqual(collisionProfile, {
            blocksMovement: true,
            blocksLineOfSight: true,
        });

        for (let x = coordinate.x; x < coordinate.x + sizeX; x += 1) {
            for (let y = coordinate.y; y < coordinate.y + sizeY; y += 1) {
                this.gameObjects.add(`${coordinate.plane}:${x}:${y}`);
            }
        }

        return true;
    }

    RemoveGameObject(coordinate) {
        const key = `${coordinate.plane}:${coordinate.x}:${coordinate.y}`;

        if (!this.gameObjects.has(key)) {
            return false;
        }

        this.gameObjects.clear();
        return true;
    }

    IsGameObjectTile(coordinate) {
        return this.gameObjects.has(`${coordinate.plane}:${coordinate.x}:${coordinate.y}`);
    }

    GetTileFlagLabels(coordinate) {
        return this.IsGameObjectTile(coordinate)
            ? ["BlockMovementObject", "BlockLineOfSightFull"]
            : [];
    }
}

class FakeWorld {
    constructor() {
        this.defaultSceneId = 1;
        this.scene = new FakeScene();
        this.nextActorId = 1;
        this.actors = new Map();
    }

    GetDefaultSceneId() {
        return this.defaultSceneId;
    }

    TryGetScene(sceneId) {
        return sceneId === this.defaultSceneId ? this.scene : null;
    }

    CreatePlayer(size, speed) {
        return this.createActor("Player", size, speed);
    }

    CreateNpc(size, speed) {
        return this.createActor("NPC", size, speed);
    }

    PlaceActor(actorId, sceneId, coordinate) {
        const actor = this.actors.get(actorId);

        if (actor === undefined || sceneId !== this.defaultSceneId) {
            return false;
        }

        actor.coordinate = { ...coordinate };
        actor.sceneId = sceneId;
        return true;
    }

    RemoveActor(actorId) {
        return this.actors.delete(actorId);
    }

    SetActorMovementTarget(actorId, targetActorId) {
        const actor = this.actors.get(actorId);

        if (actor === undefined || !this.actors.has(targetActorId)) {
            return false;
        }

        actor.movementTarget = { kind: "Actor", actorId: targetActorId };
        return true;
    }

    SetPlayerSceneCoordinateMovementTarget(actorId, coordinate) {
        const actor = this.actors.get(actorId);

        if (actor === undefined || actor.kind !== "Player") {
            return false;
        }

        actor.movementTarget = {
            kind: "SceneCoordinate",
            coordinate: { ...coordinate },
        };
        return true;
    }

    CanPlayerUseSceneCoordinateMovementTarget(actorId, coordinate) {
        const actor = this.actors.get(actorId);

        return actor?.kind === "Player" && !this.scene.IsGameObjectTile(coordinate);
    }

    GetActorSnapshot(actorId) {
        const actor = this.actors.get(actorId);

        if (actor === undefined) {
            return null;
        }

        return JSON.stringify(actor);
    }

    createActor(kind, size, speed) {
        const id = this.nextActorId;
        this.nextActorId += 1;
        this.actors.set(id, {
            id,
            kind,
            coordinate: { x: 0, y: 0, plane: 0 },
            size,
            speed,
            movementTarget: null,
        });
        return id;
    }
}

class FakeEngine {
    constructor() {
        this.currentTick = 0;
        this.world = new FakeWorld();
    }

    Step() {
        this.currentTick += 1;
        const player = this.world.actors.get(1);
        const npc = this.world.actors.get(2);

        if (player?.movementTarget?.kind === "SceneCoordinate") {
            player.coordinate = { ...player.movementTarget.coordinate };
            player.movementTarget = null;
        }

        if (npc !== undefined && player !== undefined) {
            npc.coordinate = {
                x: Math.max(player.coordinate.x + 1, npc.coordinate.x - 1),
                y: player.coordinate.y,
                plane: player.coordinate.plane,
            };
        }
    }

    GetCurrentTick() {
        return this.currentTick;
    }

    GetWorld() {
        return this.world;
    }
}

let locateFileResult;
const module = await loadEngineModule(async (options) => {
    locateFileResult = options.locateFile("EngineModule.wasm");

    return {
        CardinalDirection: { North: "North" },
        Engine: FakeEngine,
    };
});

const engine = new module.Engine();

assert.equal(engine.GetCurrentTick(), 0);
engine.Step();
assert.equal(engine.GetCurrentTick(), 1);
assert.equal(engine.GetWorld().GetDefaultSceneId(), 1);
assert.equal(registeredScenarios.length, 1);
assert.equal(registeredScenarios[0].name, "Player Chase");
assert.match(locateFileResult, /\/generated\/EngineModule\.wasm$/);

const scenario = createPlayerChaseScenario(module);
const initialSnapshot = JSON.parse(scenario.GetSnapshotJson());

assert.equal(initialSnapshot.name, "Player Chase");
assert.equal(initialSnapshot.tick, 0);
assert.equal(initialSnapshot.player.coordinate.x, 8);
assert.equal(initialSnapshot.npcs.length, 1);
assert.equal(initialSnapshot.npcs[0].coordinate.x, 18);
assert.equal(initialSnapshot.npcs[0].movementTarget.actorId, initialSnapshot.player.id);
assert.equal(
    initialSnapshot.tiles.filter((tile) =>
        tile.flags.includes("BlockMovementObject"),
    ).length,
    9,
);

assert.equal(scenario.ClickSceneCoordinate(12, 4, 0), false);
assert.equal(scenario.WasLastClickBlocked(), true);
assert.equal(scenario.ClickSceneCoordinate(10, 11, 0), true);

scenario.Step();

const movedSnapshot = JSON.parse(scenario.GetSnapshotJson());

assert.equal(movedSnapshot.tick, 1);
assert.deepEqual(movedSnapshot.player.coordinate, { x: 10, y: 11, plane: 0 });
assert.equal(movedSnapshot.npcs[0].coordinate.y, 11);

scenario.Reset();

const resetSnapshot = JSON.parse(scenario.GetSnapshotJson());

assert.equal(resetSnapshot.tick, 0);
assert.deepEqual(resetSnapshot.player.coordinate, { x: 8, y: 11, plane: 0 });

await writeFile(`${outfile}.stamp`, new Date().toISOString());
