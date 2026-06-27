import assert from "node:assert/strict";
import { loadEngineModule } from "../src/wasm/EngineModule.ts";

const engineModule = await loadEngineModule();
const playback = engineModule.RecordingPlayback.LoadFromJson(
    JSON.stringify({
        version: 1,
        metadata: {
            encounterName: "Generated Recording Playback",
            secondsPerTick: 0.6,
        },
        initialState: {
            tick: 0,
            actors: [
                {
                    id: 1,
                    kind: "Player",
                    present: true,
                    playerIndex: 0,
                    sceneMembership: {
                        sceneId: 1,
                        coordinate: { x: 10, y: 11, plane: 0 },
                    },
                    size: 1,
                    speed: 2,
                    combatComposition: {
                        stats: {
                            attack: 70,
                            strength: 71,
                            defence: 72,
                            ranged: 73,
                            magic: 74,
                            hitpoints: 82,
                        },
                        baseStats: {
                            attack: 1,
                            strength: 1,
                            defence: 1,
                            ranged: 1,
                            magic: 1,
                            hitpoints: 99,
                        },
                        bonuses: {},
                        attackType: "Slash",
                        magicBaseMaximumHit: 9,
                        weapon: { id: 12, range: 4, speed: 6, projectileId: 88 },
                    },
                    debug: { movementTarget: null, attackTimer: 0 },
                },
            ],
            sceneEntities: [
                {
                    kind: "GameObject",
                    sceneId: 1,
                    id: 5001,
                    coordinate: { x: 11, y: 12, plane: 0 },
                    direction: "North",
                    sizeX: 1,
                    sizeY: 1,
                    collision: {
                        blocksMovement: true,
                        blocksLineOfSight: false,
                    },
                },
            ],
            projectiles: [],
        },
        ticks: [
            {
                tick: 1,
                actors: [
                    {
                        id: 1,
                        currentHitpoints: 77,
                        debug: { attackTimer: 6 },
                    },
                ],
                attacks: [
                    {
                        id: 1,
                        tick: 1,
                        attackerId: 1,
                        targetId: 2,
                        callback: "standard_attack",
                        queuedDamageEvents: [
                            {
                                id: 1,
                                attackId: 1,
                                targetId: 2,
                                damage: 7,
                                delayTicks: 1,
                            },
                        ],
                        projectile: {
                            projectileId: 88,
                            source: { x: 10.5, y: 11.5, plane: 0 },
                            targetActorId: 2,
                            lastKnownTargetCenter: { x: 13.5, y: 13.5, plane: 0 },
                            totalTicks: 1,
                        },
                    },
                ],
                damageApplications: [],
                sceneChanges: [
                    {
                        kind: "WallObject",
                        sceneId: 1,
                        id: 6001,
                        coordinate: { x: 13, y: 13, plane: 0 },
                        direction: "East",
                        collision: {
                            blocksMovement: true,
                            blocksLineOfSight: false,
                        },
                        present: true,
                    },
                ],
                projectiles: [
                    {
                        projectileId: 88,
                        source: { x: 10.5, y: 11.5, plane: 0 },
                        targetActorId: 2,
                        lastKnownTargetCenter: { x: 13.5, y: 13.5, plane: 0 },
                        elapsedTicks: 0,
                        totalTicks: 1,
                    },
                ],
            },
            {
                tick: 2,
                actors: [
                    {
                        id: 1,
                        currentHitpoints: 75,
                        debug: { attackTimer: 5 },
                    },
                ],
                attacks: [],
                damageApplications: [
                    {
                        damageEventId: 1,
                        attackId: 1,
                        tick: 2,
                        targetId: 2,
                        queuedDamage: 7,
                        appliedDamage: 7,
                    },
                ],
                sceneChanges: [],
                projectiles: [],
            },
        ],
    }),
);

assert.equal(playback.GetEncounterName(), "Generated Recording Playback");
assert.equal(playback.GetSecondsPerTick(), 0.6);
assert.equal(playback.GetInitialTick(), 0);
assert.equal(playback.GetCurrentTick(), 0);
assert.equal(playback.GetLastTick(), 2);
assert.equal(typeof playback.GetActorsJson, "undefined");
assert.equal(typeof playback.GetSceneEntitiesJson, "undefined");
assert.equal(typeof playback.GetAttacksJson, "undefined");
assert.equal(typeof playback.GetDamageApplicationsJson, "undefined");
assert.equal(typeof playback.GetProjectilesJson, "undefined");
assert.equal(typeof playback.PreviousTick, "undefined");
assert.equal(typeof playback.NextTick, "undefined");
assert.equal(typeof playback.GoToTick, "undefined");

let currentSnapshot = JSON.parse(playback.GetCurrentSnapshotJson());
assert.equal(currentSnapshot.tick, 0);
assert.equal(currentSnapshot.actors[0].kind, "Player");
assert.equal(currentSnapshot.sceneEntities[0].id, 5001);
assert.deepEqual(currentSnapshot.attacks, []);
assert.deepEqual(currentSnapshot.damageApplications, []);
assert.deepEqual(currentSnapshot.visibleProjectiles, []);
assert.equal(playback.IsComplete(), false);
assert.equal(playback.Advance(), true);
assert.equal(playback.GetCurrentTick(), 1);
currentSnapshot = JSON.parse(playback.GetCurrentSnapshotJson());
assert.equal(
    currentSnapshot.actors[0].combatComposition.stats.hitpoints,
    77,
);
assert.equal(currentSnapshot.actors[0].debug.attackTimer, 6);
assert.equal(currentSnapshot.sceneEntities.length, 2);
assert.equal(currentSnapshot.sceneEntities[1].id, 6001);
assert.equal(currentSnapshot.attacks[0].callback, "standard_attack");
assert.equal(
    currentSnapshot.attacks[0].queuedDamageEvents[0].attackId,
    1,
);
assert.equal(currentSnapshot.visibleProjectiles[0].projectileId, 88);
assert.equal(playback.Advance(), true);
assert.equal(playback.GetCurrentTick(), 2);
assert.equal(playback.IsComplete(), true);
currentSnapshot = JSON.parse(playback.GetCurrentSnapshotJson());
assert.deepEqual(currentSnapshot.attacks, []);
assert.equal(currentSnapshot.damageApplications[0].attackId, 1);
assert.deepEqual(currentSnapshot.visibleProjectiles, []);
assert.equal(playback.Advance(), false);
playback.Reset();
assert.equal(playback.GetCurrentTick(), 0);
currentSnapshot = JSON.parse(playback.GetCurrentSnapshotJson());
assert.equal(
    currentSnapshot.actors[0].combatComposition.stats.hitpoints,
    82,
);
assert.deepEqual(currentSnapshot.attacks, []);
assert.deepEqual(currentSnapshot.damageApplications, []);

playback.delete?.();

const versionTwoPlayback = engineModule.RecordingPlayback.LoadFromJson(
    JSON.stringify({
        version: 2,
        metadata: {
            encounterName: "Generated Version 2 Recording Playback",
            secondsPerTick: 0.6,
        },
        initialTick: 4,
        initialFacts: {
            actorFacts: [
                {
                    id: 1,
                    present: true,
                    kind: "player",
                    playerIndex: 0,
                    sceneMembership: {
                        sceneId: 1,
                        coordinate: { x: 10, y: 10, plane: 0 },
                    },
                    size: 1,
                    speed: 2,
                    currentHitpoints: 82,
                    combatComposition: {
                        stats: {
                            attack: 70,
                            strength: 71,
                            defence: 72,
                            ranged: 73,
                            magic: 74,
                            hitpoints: 1,
                        },
                        baseStats: {
                            attack: 70,
                            strength: 71,
                            defence: 72,
                            ranged: 73,
                            magic: 74,
                            hitpoints: 99,
                        },
                        bonuses: {},
                        attackType: "slash",
                        magicBaseMaximumHit: 9,
                        weapon: {
                            id: 12,
                            range: 4,
                            speed: 6,
                            projectileId: 88,
                        },
                    },
                    movementTarget: null,
                    attackTimer: 0,
                },
            ],
            sceneEntityFacts: [],
            visibleProjectiles: [],
        },
        completedTicks: [
            {
                tick: 5,
                actorFacts: [
                    {
                        id: 1,
                        present: true,
                        currentHitpoints: 77,
                        attackTimer: 6,
                    },
                ],
                sceneEntityFacts: [],
                attacks: [],
                damageApplications: [],
                visibleProjectiles: [],
            },
        ],
    }),
);

assert.equal(
    versionTwoPlayback.GetEncounterName(),
    "Generated Version 2 Recording Playback",
);
assert.equal(versionTwoPlayback.GetInitialTick(), 4);
assert.equal(versionTwoPlayback.GetCurrentTick(), 4);
assert.equal(versionTwoPlayback.GetLastTick(), 5);
assert.equal(versionTwoPlayback.IsComplete(), false);
const versionTwoInitialSnapshot = JSON.parse(
    versionTwoPlayback.GetCurrentSnapshotJson(),
);
assert.equal(versionTwoInitialSnapshot.tick, 4);
assert.equal(versionTwoInitialSnapshot.actors.length, 1);
assert.equal(versionTwoInitialSnapshot.actors[0].kind, "player");
assert.equal(
    versionTwoInitialSnapshot.actors[0].combatComposition.stats.hitpoints,
    82,
);
assert.equal(versionTwoInitialSnapshot.actors[0].currentHitpoints, 82);
assert.equal(versionTwoPlayback.Advance(), true);
assert.equal(versionTwoPlayback.GetCurrentTick(), 5);
assert.equal(versionTwoPlayback.IsComplete(), true);
const versionTwoCompletedSnapshot = JSON.parse(
    versionTwoPlayback.GetCurrentSnapshotJson(),
);
assert.equal(
    versionTwoCompletedSnapshot.actors[0].combatComposition.stats.hitpoints,
    77,
);
assert.equal(versionTwoCompletedSnapshot.actors[0].attackTimer, 6);
assert.equal(versionTwoPlayback.Advance(), false);
versionTwoPlayback.Reset();
assert.equal(versionTwoPlayback.GetCurrentTick(), 4);
assert.equal(versionTwoPlayback.IsComplete(), false);

versionTwoPlayback.delete?.();

assert.throws(() =>
    engineModule.RecordingPlayback.LoadFromJson(
        JSON.stringify({
            version: 1,
            metadata: {
                encounterName: "Invalid Recording Playback",
                secondsPerTick: 0.6,
            },
            initialState: {
                tick: 0,
                actors: [
                    {
                        id: 1,
                        kind: "Goblin",
                        present: true,
                        sceneMembership: {
                            sceneId: 1,
                            coordinate: { x: 10, y: 11, plane: 0 },
                        },
                        size: 1,
                        speed: 2,
                        combatComposition: {
                            stats: {
                                attack: 70,
                                strength: 71,
                                defence: 72,
                                ranged: 73,
                                magic: 74,
                                hitpoints: 82,
                            },
                            baseStats: {
                                attack: 1,
                                strength: 1,
                                defence: 1,
                                ranged: 1,
                                magic: 1,
                                hitpoints: 99,
                            },
                            bonuses: {},
                            attackType: "Slash",
                            magicBaseMaximumHit: 9,
                            weapon: {
                                id: 12,
                                range: 4,
                                speed: 6,
                                projectileId: 88,
                            },
                        },
                        debug: { movementTarget: null, attackTimer: 0 },
                    },
                ],
                sceneEntities: [],
                projectiles: [],
            },
            ticks: [],
        }),
    ),
);
