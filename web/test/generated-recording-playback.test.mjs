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
assert.equal(JSON.parse(playback.GetActorsJson())[0].kind, "Player");
assert.equal(JSON.parse(playback.GetSceneEntitiesJson())[0].id, 5001);
assert.deepEqual(JSON.parse(playback.GetAttacksJson()), []);
assert.deepEqual(JSON.parse(playback.GetDamageApplicationsJson()), []);
assert.deepEqual(JSON.parse(playback.GetProjectilesJson()), []);
assert.equal(playback.NextTick(), true);
assert.equal(playback.GetCurrentTick(), 1);
assert.equal(
    JSON.parse(playback.GetActorsJson())[0].combatComposition.stats.hitpoints,
    77,
);
assert.equal(JSON.parse(playback.GetActorsJson())[0].debug.attackTimer, 6);
assert.equal(JSON.parse(playback.GetSceneEntitiesJson()).length, 2);
assert.equal(JSON.parse(playback.GetSceneEntitiesJson())[1].id, 6001);
assert.equal(JSON.parse(playback.GetAttacksJson())[0].callback, "standard_attack");
assert.equal(
    JSON.parse(playback.GetAttacksJson())[0].queuedDamageEvents[0].attackId,
    1,
);
assert.equal(JSON.parse(playback.GetProjectilesJson())[0].projectileId, 88);
assert.equal(playback.NextTick(), true);
assert.equal(playback.GetCurrentTick(), 2);
assert.deepEqual(JSON.parse(playback.GetAttacksJson()), []);
assert.equal(JSON.parse(playback.GetDamageApplicationsJson())[0].attackId, 1);
assert.deepEqual(JSON.parse(playback.GetProjectilesJson()), []);
assert.equal(playback.NextTick(), false);

playback.delete?.();
