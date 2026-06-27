import assert from "node:assert/strict";
import { readFile } from "node:fs/promises";
import { dirname, resolve } from "node:path";
import { fileURLToPath } from "node:url";

const root = dirname(dirname(fileURLToPath(import.meta.url)));
const viewSource = await readFile(resolve(root, "src/RecordingView.vue"), "utf8");
const sampleJson = JSON.parse(
    await readFile(
        resolve(root, "public/recordings/minimal-encounter-recording.json"),
        "utf8",
    ),
);

assert.equal(sampleJson.version, 2);
assert.equal(sampleJson.metadata.encounterName, "Minimal Encounter Recording");
assert.equal(sampleJson.metadata.secondsPerTick, 0.6);
assert.equal(sampleJson.initialTick, 0);
assert.equal(sampleJson.initialFacts.actorFacts.length, 1);
assert.equal(sampleJson.initialFacts.actorFacts[0].kind, "player");
assert.equal(sampleJson.initialFacts.actorFacts[0].currentHitpoints, 82);
assert.equal(sampleJson.initialFacts.actorFacts[0].combatComposition.stats.hitpoints, 82);
assert.deepEqual(
    sampleJson.initialFacts.actorFacts[0].combatComposition.equipmentProvenance,
    [
        { slot: "Amulet", pieceId: 2001 },
        { slot: "Weapon", pieceId: 2002 },
    ],
);
assert.equal(sampleJson.initialFacts.sceneEntityFacts.length, 1);
assert.equal(sampleJson.initialFacts.sceneEntityFacts[0].kind, "game_object");
assert.equal(sampleJson.initialFacts.sceneEntityFacts[0].id, 5001);
assert.equal(sampleJson.initialFacts.sceneEntityFacts[0].coordinate.x, 11);
assert.equal(sampleJson.completedTicks[0].actorFacts.length, 2);
assert.equal(sampleJson.completedTicks[0].actorFacts[0].currentHitpoints, 77);
assert.equal(sampleJson.completedTicks[0].attacks[0].callback, "standard_attack");
assert.equal(sampleJson.completedTicks[0].attacks[0].queuedDamageEvents[0].attackId, 1);
assert.equal(sampleJson.completedTicks[0].visibleProjectiles[0].projectileId, 88);
assert.equal(sampleJson.completedTicks[0].sceneEntityFacts.length, 1);
assert.equal(sampleJson.completedTicks[0].sceneEntityFacts[0].kind, "wall_object");
assert.equal(sampleJson.completedTicks[0].sceneEntityFacts[0].present, true);
assert.equal(sampleJson.completedTicks[1].damageApplications[0].attackId, 1);
assert.deepEqual(Object.keys(sampleJson.completedTicks[0]), [
    "tick",
    "actorFacts",
    "sceneEntityFacts",
    "attacks",
    "damageApplications",
    "visibleProjectiles",
]);

assert.match(viewSource, /loadEngineModule/);
assert.match(viewSource, /RecordingPlayback\.LoadFromJson/);
assert.match(viewSource, /recordings\/minimal-encounter-recording\.json/);
assert.match(viewSource, /aria-label="Recording loader"/);
assert.match(viewSource, /aria-label="Recording metadata"/);
assert.match(viewSource, /aria-label="Tick navigation"/);
assert.match(viewSource, /aria-label="Recording JSON file"/);
assert.match(viewSource, /handleRecordingFileChange/);
assert.match(viewSource, /loadRecordingJson/);
assert.match(viewSource, /Play/);
assert.match(viewSource, /Pause/);
assert.match(viewSource, /playbackTimer/);
assert.match(viewSource, /goToSelectedTick/);
assert.match(viewSource, /type="number"/);
assert.match(viewSource, /Follow selected actor/);
assert.match(viewSource, /Free camera/);
assert.match(viewSource, /cameraMode/);
assert.match(viewSource, /cameraActor/);
assert.match(viewSource, /firstPlayer/);
assert.match(viewSource, /aria-label="Actor inspector"/);
assert.match(viewSource, /aria-label="Recording scene"/);
assert.match(viewSource, /GetCurrentSnapshotJson/);
assert.doesNotMatch(viewSource, /GetActorsJson/);
assert.doesNotMatch(viewSource, /GetSceneEntitiesJson/);
assert.doesNotMatch(viewSource, /GetAttacksJson/);
assert.doesNotMatch(viewSource, /GetDamageApplicationsJson/);
assert.doesNotMatch(viewSource, /GetProjectilesJson/);
assert.match(viewSource, /Advance/);
assert.match(viewSource, /Reset/);
assert.match(viewSource, /IsComplete/);
assert.doesNotMatch(viewSource, /PreviousTick/);
assert.doesNotMatch(viewSource, /NextTick/);
assert.doesNotMatch(viewSource, /GoToTick/);
assert.match(viewSource, /sceneEntityAtCell/);
assert.match(viewSource, /projectileAtCell/);
assert.match(viewSource, /Hitpoints/);
assert.match(viewSource, /Combat Composition/);
assert.match(viewSource, /Equipment Provenance/);
assert.match(viewSource, /equipmentProvenanceText/);
assert.match(viewSource, /Movement Target/);
assert.match(viewSource, /Attack Timer/);
assert.match(viewSource, /Current Tick Events/);
assert.match(viewSource, /Attack #/);
assert.match(viewSource, /Damage #/);
assert.match(viewSource, /:id="`attack-/);
assert.match(viewSource, /:href="`#attack-/);
assert.match(viewSource, /:id="`damage-/);
assert.match(viewSource, /:href="`#damage-/);
assert.match(viewSource, /Previous/);
assert.match(viewSource, /Next/);
assert.match(viewSource, /Current Tick/);
assert.match(viewSource, /role="alert"/);
assert.match(viewSource, /formatRecordingLoadError/);
assert.match(viewSource, /Recording load failed: invalid or unsupported recording JSON\./);
