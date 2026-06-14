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
        this.gameObjects = new Map();
        this.placements = [];
        this.failNextPlacement = false;
    }

    PlaceGameObject(coordinate, id, direction, sizeX, sizeY, collisionProfile) {
        if (this.failNextPlacement) {
            this.failNextPlacement = false;
            return false;
        }

        this.placements.push({
            coordinate: { ...coordinate },
            id,
            direction,
            sizeX,
            sizeY,
            collisionProfile: { ...collisionProfile },
        });

        const footprintWidth =
            direction === "East" || direction === "West" ? sizeY : sizeX;
        const footprintHeight =
            direction === "East" || direction === "West" ? sizeX : sizeY;

        for (let x = coordinate.x; x < coordinate.x + footprintWidth; x += 1) {
            for (let y = coordinate.y; y < coordinate.y + footprintHeight; y += 1) {
                this.gameObjects.set(`${coordinate.plane}:${x}:${y}`, {
                    id,
                    origin: { ...coordinate },
                    collisionProfile: { ...collisionProfile },
                });
            }
        }

        return true;
    }

    RemoveGameObject(coordinate) {
        const key = `${coordinate.plane}:${coordinate.x}:${coordinate.y}`;
        const gameObject = this.gameObjects.get(key);

        if (gameObject === undefined) {
            return false;
        }

        for (const [candidateKey, candidate] of this.gameObjects) {
            if (candidate.id === gameObject.id) {
                this.gameObjects.delete(candidateKey);
            }
        }

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

    HasLineOfSight(sourceAnchor, sourceActorSize, target, range) {
        return (
            sourceAnchor.plane === target.plane &&
            sourceActorSize > 0 &&
            Math.abs(target.x - sourceAnchor.x) <= range &&
            Math.abs(target.y - sourceAnchor.y) <= range &&
            !this.IsGameObjectTile(target)
        );
    }

    HasActorLineOfSight(
        sourceAnchor,
        sourceActorSize,
        targetAnchor,
        targetActorSize,
        range,
    ) {
        return (
            targetActorSize > 0 &&
            this.HasLineOfSight(sourceAnchor, sourceActorSize, targetAnchor, range)
        );
    }
}

class FakeWorld {
    constructor() {
        this.defaultSceneId = 1;
        this.scene = new FakeScene();
        this.nextActorId = 1;
        this.actors = new Map();
        this.failNextActorPlacement = false;
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

        if (
            actor === undefined ||
            sceneId !== this.defaultSceneId ||
            this.failNextActorPlacement
        ) {
            this.failNextActorPlacement = false;
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
        FakeEngine.lastCreated = this;
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
assert.equal(module.DevelopmentPlayerChaseScenario, undefined);
assert.match(locateFileResult, /\/generated\/EngineModule\.wasm$/);

const scenario = createPlayerChaseScenario(module);
const initialSnapshot = scenario.snapshot();

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

assert.equal(scenario.clickSceneCoordinate(12, 4, 0), false);
assert.equal(scenario.wasLastClickBlocked(), true);
assert.deepEqual(scenario.snapshot().actionFeedback, {
    state: "blocked-movement",
});
assert.equal(scenario.clickSceneCoordinate(10, 11, 0), true);
assert.deepEqual(scenario.snapshot().actionFeedback, {
    state: "none",
});

scenario.step();

const movedSnapshot = scenario.snapshot();

assert.equal(movedSnapshot.tick, 1);
assert.deepEqual(movedSnapshot.player.coordinate, { x: 10, y: 11, plane: 0 });
assert.equal(movedSnapshot.npcs[0].coordinate.y, 11);

scenario.reset();

const resetSnapshot = scenario.snapshot();

assert.equal(resetSnapshot.tick, 0);
assert.deepEqual(resetSnapshot.player.coordinate, { x: 8, y: 11, plane: 0 });
assert.equal(resetSnapshot.running, false);
assert.deepEqual(resetSnapshot.actionFeedback, { state: "none" });
assert.equal(resetSnapshot.npcs.length, 1);
assert.equal(resetSnapshot.selectedNpcId, resetSnapshot.npc.id);
assert.equal(
    resetSnapshot.tiles.filter((tile) => tile.gameObject?.id === 200).length,
    9,
);

module.Engine.lastCreated.world.failNextActorPlacement = true;
assert.equal(scenario.placeNpc(1, 1, 9, 9, 0), false);
assert.equal(scenario.snapshot().npcs.length, 1);
assert.deepEqual(scenario.snapshot().actionFeedback, {
    state: "placement-failure",
});

assert.equal(scenario.placeNpc(2, 0, 9, 9, 0), true);

const placedNpcSnapshot = scenario.snapshot();

assert.equal(placedNpcSnapshot.npcs.length, 2);
assert.equal(placedNpcSnapshot.npc.size, 2);
assert.equal(placedNpcSnapshot.npc.speed, 0);
assert.deepEqual(placedNpcSnapshot.npc.coordinate, { x: 9, y: 9, plane: 0 });
assert.equal(
    placedNpcSnapshot.npc.movementTarget.actorId,
    placedNpcSnapshot.player.id,
);
assert.equal(placedNpcSnapshot.selectedNpcId, placedNpcSnapshot.npc.id);
assert.equal(scenario.removeNpc(10, 10, 0), true);

const removedByFootprintSnapshot = scenario.snapshot();

assert.equal(removedByFootprintSnapshot.npcs.length, 1);
assert.equal(
    removedByFootprintSnapshot.selectedNpcId,
    removedByFootprintSnapshot.npc.id,
);

assert.equal(scenario.removeNpc(0, 0, 0), false);
assert.deepEqual(scenario.snapshot().actionFeedback, {
    state: "removal-failure",
});

assert.equal(scenario.placeNpc(3, 1, 30, 30, 0), true);
const firstOverlapNpcId = scenario.snapshot().selectedNpcId;
assert.equal(scenario.placeNpc(3, 1, 31, 31, 0), true);
const secondOverlapNpcId = scenario.snapshot().selectedNpcId;

assert.equal(scenario.removeNpc(31, 31, 0), true);
assert.equal(
    scenario.snapshot().npcs.some(
        (npc) => npc.id === secondOverlapNpcId,
    ),
    false,
);
assert.equal(
    scenario.snapshot().npcs.some(
        (npc) => npc.id === firstOverlapNpcId,
    ),
    true,
);
assert.equal(scenario.removeNpc(32, 32, 0), true);
assert.equal(
    scenario.snapshot().npcs.some(
        (npc) => npc.id === firstOverlapNpcId,
    ),
    false,
);

assert.equal(scenario.placeNpc(2, 1, 40, 40, 0), true);
assert.equal(scenario.placeNpc(2, 1, 41, 41, 0), true);
const newestOverlapNpcId = scenario.snapshot().selectedNpcId;
assert.equal(scenario.removeNpc(50, 50, 0), false);
assert.equal(
    scenario.snapshot().npcs.some(
        (npc) => npc.id === newestOverlapNpcId,
    ),
    true,
);

assert.equal(scenario.placeGameObject(20, 20, 0, 1, 1, "North", true, true), true);
assert.deepEqual(scenario.snapshot().actionFeedback, {
    state: "none",
});

assert.equal(scenario.placeGameObject(50, 50, 0, 2, 3, "East", false, true), true);

const eastObjectSnapshot = scenario.snapshot();
const eastObjectTiles = eastObjectSnapshot.tiles.filter(
    (tile) => tile.gameObject?.id === 202,
);

assert.deepEqual(
    module.Engine.lastCreated.world.scene.placements.at(-1),
    {
        coordinate: { x: 50, y: 50, plane: 0 },
        id: 202,
        direction: "East",
        sizeX: 2,
        sizeY: 3,
        collisionProfile: {
            blocksMovement: false,
            blocksLineOfSight: true,
        },
    },
);
assert.equal(eastObjectTiles.length, 6);
assert.ok(
    eastObjectTiles.some(
        (tile) => tile.coordinate.x === 52 && tile.coordinate.y === 51,
    ),
);

assert.equal(scenario.removeGameObject(52, 51, 0), true);
assert.equal(
    scenario.snapshot().tiles.some(
        (tile) => tile.gameObject?.id === 202,
    ),
    false,
);
assert.equal(
    scenario.snapshot().tiles.some(
        (tile) => tile.gameObject?.id === 201,
    ),
    true,
);

module.Engine.lastCreated.world.scene.failNextPlacement = true;
assert.equal(scenario.placeGameObject(21, 20, 0, 1, 1, "North", true, true), false);
assert.deepEqual(scenario.snapshot().actionFeedback, {
    state: "placement-failure",
});

assert.equal(scenario.removeGameObject(0, 0, 0), false);
assert.deepEqual(scenario.snapshot().actionFeedback, {
    state: "removal-failure",
});

scenario.setRunning(true);
assert.equal(scenario.placeNpc(1, 1, 30, 30, 0), true);
assert.equal(scenario.placeGameObject(40, 40, 0, 1, 1, "North", true, true), true);
assert.equal(scenario.removeNpc(18, 20, 0), true);
scenario.reset();

const resetFromEditedSnapshot = scenario.snapshot();

assert.equal(resetFromEditedSnapshot.running, false);
assert.deepEqual(resetFromEditedSnapshot.actionFeedback, { state: "none" });
assert.deepEqual(resetFromEditedSnapshot.player.coordinate, { x: 8, y: 11, plane: 0 });
assert.equal(resetFromEditedSnapshot.npcs.length, 1);
assert.equal(resetFromEditedSnapshot.npc.size, 4);
assert.equal(
    resetFromEditedSnapshot.tiles.filter((tile) => tile.gameObject?.id === 200).length,
    9,
);

await writeFile(`${outfile}.stamp`, new Date().toISOString());
