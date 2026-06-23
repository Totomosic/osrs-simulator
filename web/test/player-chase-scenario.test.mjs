import assert from "node:assert/strict";
import { mkdir, readFile, writeFile } from "node:fs/promises";
import { dirname, resolve } from "node:path";
import { fileURLToPath, pathToFileURL } from "node:url";
import { build } from "esbuild";

const root = dirname(dirname(fileURLToPath(import.meta.url)));
const outfile = resolve(
    root,
    "../node_modules/.cache/osrs-simulator/player-chase-scenario-test.mjs",
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
await writeFile(`${outfile}.stamp`, new Date().toISOString());

const { createPlayerChaseScenario } = await import(pathToFileURL(outfile));

function createFakeEngineModule() {
    class FakeEngine {
        constructor() {
            this.tick = 0;
            this.world = new FakeWorld();
        }

        Step() {
            this.tick += 1;
        }

        GetCurrentTick() {
            return this.tick;
        }

        GetWorld() {
            return this.world;
        }
    }

    return {
        Engine: FakeEngine,
        CardinalDirection: {
            North: "North",
            East: "East",
            South: "South",
            West: "West",
        },
    };
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

    CreatePlayer(size, speed, combatComposition) {
        return this.createActor("Player", size, speed, combatComposition);
    }

    CreateNpc(size, speed, combatComposition) {
        return this.createActor("NPC", size, speed, combatComposition);
    }

    PlaceActor(actorId, sceneId, coordinate) {
        const actor = this.actors.get(actorId);

        if (actor === undefined || sceneId !== this.defaultSceneId) {
            return false;
        }

        actor.coordinate = coordinate;
        return true;
    }

    RemoveActor(actorId) {
        return this.actors.delete(actorId);
    }

    SetActorCombatComposition(actorId, combatComposition) {
        const actor = this.actors.get(actorId);

        if (actor === undefined) {
            return false;
        }

        actor.weapon = combatComposition.weapon;
        return true;
    }

    SetActorMovementTarget(actorId, targetActorId) {
        const actor = this.actors.get(actorId);

        if (actor === undefined || !this.actors.has(targetActorId)) {
            return false;
        }

        actor.movementTarget = {
            kind: "Actor",
            actorId: targetActorId,
        };
        return true;
    }

    SetPlayerSceneCoordinateMovementTarget(actorId, coordinate) {
        const actor = this.actors.get(actorId);

        if (actor === undefined || actor.kind !== "Player") {
            return false;
        }

        actor.movementTarget = {
            kind: "SceneCoordinate",
            coordinate,
        };
        return true;
    }

    CanPlayerUseSceneCoordinateMovementTarget(actorId, coordinate) {
        return (
            this.actors.get(actorId)?.kind === "Player" &&
            !this.scene.IsGameObjectTile(coordinate)
        );
    }

    GetActorSnapshot(actorId) {
        const actor = this.actors.get(actorId);

        if (actor === undefined || actor.coordinate === null) {
            return null;
        }

        return JSON.stringify(actor);
    }

    GetProjectileSnapshotsJson() {
        return JSON.stringify([
            {
                projectileId: 61,
                source: { x: 8.5, y: 11.5, plane: 0 },
                targetActorId: 2,
                lastKnownTargetCenter: { x: 20, y: 22, plane: 0 },
                elapsedTicks: 0,
                totalTicks: 2,
            },
        ]);
    }

    createActor(kind, size, speed, combatComposition) {
        const id = this.nextActorId;
        this.nextActorId += 1;
        this.actors.set(id, {
            id,
            kind,
            playerIndex: kind === "Player" ? id - 1 : undefined,
            npcIndex: kind === "NPC" ? id - 2 : undefined,
            coordinate: null,
            size,
            speed,
            weapon: combatComposition.weapon,
            hitpoints: combatComposition.stats.hitpoints,
            baseHitpoints: combatComposition.baseStats.hitpoints,
            movementTarget: null,
        });
        return id;
    }
}

class FakeScene {
    constructor() {
        this.gameObjects = [];
    }

    PlaceGameObject(
        coordinate,
        id,
        direction,
        sizeX,
        sizeY,
        collisionProfile,
    ) {
        this.gameObjects.push({
            coordinate,
            id,
            direction,
            sizeX,
            sizeY,
            collisionProfile,
        });
        return true;
    }

    RemoveGameObject(coordinate) {
        const initialLength = this.gameObjects.length;
        this.gameObjects = this.gameObjects.filter(
            (gameObject) => !this.isGameObjectTile(gameObject, coordinate),
        );
        return this.gameObjects.length !== initialLength;
    }

    IsGameObjectTile(coordinate) {
        return this.gameObjects.some((gameObject) =>
            this.isGameObjectTile(gameObject, coordinate),
        );
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

    isGameObjectTile(gameObject, coordinate) {
        return (
            coordinate.plane === gameObject.coordinate.plane &&
            coordinate.x >= gameObject.coordinate.x &&
            coordinate.x < gameObject.coordinate.x + gameObject.sizeX &&
            coordinate.y >= gameObject.coordinate.y &&
            coordinate.y < gameObject.coordinate.y + gameObject.sizeY
        );
    }
}

const module = createFakeEngineModule();
const playerChaseViewSource = await readFile(
    resolve(root, "src/PlayerChaseView.vue"),
    "utf8",
);

{
    const scenario = createPlayerChaseScenario(module);
    const snapshot = scenario.snapshot();

    assert.equal(snapshot.name, "Player Chase");
    assert.equal(snapshot.player.weapon.range, 5);
    assert.equal(snapshot.player.playerIndex, 0);
    assert.equal(snapshot.player.hitpoints, 10);
    assert.equal(snapshot.player.baseHitpoints, 10);
    assert.equal(snapshot.npcs.length, 1);
    assert.equal(snapshot.npcs[0].npcIndex, 0);
    assert.equal(snapshot.npcs[0].weapon.range, 8);
    assert.equal(snapshot.npcs[0].hitpoints, 10);
    assert.equal(snapshot.npcs[0].baseHitpoints, 10);
    assert.equal(snapshot.selectedNpcId, snapshot.npcs[0].id);
    assert.deepEqual(snapshot.selectedNpc, snapshot.npcs[0]);
    assert.deepEqual(snapshot.npc, snapshot.selectedNpc);
    assert.deepEqual(snapshot.projectiles, [
        {
            projectileId: 61,
            source: { x: 8.5, y: 11.5, plane: 0 },
            targetActorId: 2,
            lastKnownTargetCenter: { x: 20, y: 22, plane: 0 },
            elapsedTicks: 0,
            totalTicks: 2,
        },
    ]);
}

{
    const scenario = createPlayerChaseScenario(module);

    assert.equal(scenario.placeNpc(1, 1, 6, 7, 0), true);

    const snapshot = scenario.snapshot();

    assert.equal(snapshot.npcs.length, 2);
    assert.equal(snapshot.selectedNpcId, snapshot.npcs[1].id);
    assert.equal(snapshot.npcs[1].npcIndex, 1);
    assert.equal(snapshot.npcs[1].weapon.range, 8);
    assert.deepEqual(snapshot.selectedNpc, snapshot.npcs[1]);
    assert.deepEqual(snapshot.npc, snapshot.selectedNpc);
}

{
    const scenario = createPlayerChaseScenario(module);
    let snapshot = scenario.snapshot();

    assert.equal(
        scenario.removeNpc(
            snapshot.selectedNpc.coordinate.x,
            snapshot.selectedNpc.coordinate.y,
            snapshot.selectedNpc.coordinate.plane,
        ),
        true,
    );

    snapshot = scenario.snapshot();

    assert.deepEqual(snapshot.npcs, []);
    assert.equal(snapshot.selectedNpcId, null);
    assert.equal(snapshot.selectedNpc, null);
    assert.equal(snapshot.npc, null);
}

{
    const scenario = createPlayerChaseScenario(module);
    const snapshot = scenario.snapshot();

    assert.equal(
        scenario.hasLineOfSight(
            snapshot.player.id,
            10,
            11,
            0,
            10,
        ),
        true,
    );
    assert.equal(
        scenario.hasLineOfSight(
            snapshot.player.id,
            12,
            4,
            0,
            10,
        ),
        false,
    );
    assert.equal(
        scenario.hasActorLineOfSight(
            snapshot.player.id,
            snapshot.selectedNpc.id,
            20,
        ),
        true,
    );
}

{
    assert.match(playerChaseViewSource, /v-for="\(projectile, index\) in snapshot\.projectiles"/);
    assert.match(playerChaseViewSource, /:cx="getProjectileX\(projectile\)"/);
    assert.match(playerChaseViewSource, /:cy="getProjectileY\(projectile\)"/);
    assert.match(playerChaseViewSource, /:fill="getProjectileFill\(projectile\)"/);
    assert.match(playerChaseViewSource, /getInterpolatedProjectileProgress\(/);
    assert.match(playerChaseViewSource, /class="projectile"/);
}

{
    assert.match(playerChaseViewSource, /v-for="actor in healthBarActors"/);
    assert.match(playerChaseViewSource, /:width="getHealthBarFillWidth\(actor\)"/);
    assert.match(playerChaseViewSource, /class="health-bar-fill"/);
}
