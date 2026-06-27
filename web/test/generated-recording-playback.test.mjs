import assert from "node:assert/strict";
import { loadEngineModule } from "../src/wasm/EngineModule.ts";

const engineModule = await loadEngineModule();
assert.throws(() =>
    engineModule.RecordingPlayback.LoadFromJson(
        JSON.stringify({
            version: 1,
            metadata: {
                encounterName: "Unsupported Version 1 Recording",
                secondsPerTick: 0.6,
            },
        }),
    ),
);

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
assert.equal(typeof versionTwoPlayback.GetActorsJson, "undefined");
assert.equal(typeof versionTwoPlayback.GetSceneEntitiesJson, "undefined");
assert.equal(typeof versionTwoPlayback.GetAttacksJson, "undefined");
assert.equal(typeof versionTwoPlayback.GetDamageApplicationsJson, "undefined");
assert.equal(typeof versionTwoPlayback.GetProjectilesJson, "undefined");
assert.equal(typeof versionTwoPlayback.PreviousTick, "undefined");
assert.equal(typeof versionTwoPlayback.NextTick, "undefined");
assert.equal(typeof versionTwoPlayback.GoToTick, "undefined");
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
            version: 2,
            metadata: {
                encounterName: "Invalid Recording Playback",
                secondsPerTick: 0.6,
            },
            initialTick: 0,
            initialFacts: {
                actorFacts: [
                    {
                        id: 1,
                        kind: "goblin",
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
                            attackType: "slash",
                            magicBaseMaximumHit: 9,
                            weapon: {
                                id: 12,
                                range: 4,
                                speed: 6,
                                projectileId: 88,
                            },
                        },
                        currentHitpoints: 82,
                        movementTarget: null,
                        attackTimer: 0,
                    },
                ],
                sceneEntityFacts: [],
                visibleProjectiles: [],
            },
            completedTicks: [],
        }),
    ),
);
