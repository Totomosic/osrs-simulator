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
                attacks: [],
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
                projectiles: [],
            },
        ],
    }),
);

assert.equal(playback.GetEncounterName(), "Generated Recording Playback");
assert.equal(playback.GetSecondsPerTick(), 0.6);
assert.equal(playback.GetInitialTick(), 0);
assert.equal(playback.GetCurrentTick(), 0);
assert.equal(playback.GetLastTick(), 1);
assert.equal(JSON.parse(playback.GetActorsJson())[0].kind, "Player");
assert.equal(JSON.parse(playback.GetSceneEntitiesJson())[0].id, 5001);
assert.equal(playback.NextTick(), true);
assert.equal(playback.GetCurrentTick(), 1);
assert.equal(
    JSON.parse(playback.GetActorsJson())[0].combatComposition.stats.hitpoints,
    77,
);
assert.equal(JSON.parse(playback.GetActorsJson())[0].debug.attackTimer, 6);
assert.equal(JSON.parse(playback.GetSceneEntitiesJson()).length, 2);
assert.equal(JSON.parse(playback.GetSceneEntitiesJson())[1].id, 6001);
assert.equal(playback.NextTick(), false);

playback.delete?.();
