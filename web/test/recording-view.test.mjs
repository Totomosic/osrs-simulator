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

assert.equal(sampleJson.version, 1);
assert.equal(sampleJson.metadata.encounterName, "Minimal Encounter Recording");
assert.equal(sampleJson.metadata.secondsPerTick, 0.6);
assert.equal(sampleJson.initialState.tick, 0);
assert.equal(sampleJson.initialState.actors.length, 1);
assert.equal(sampleJson.initialState.actors[0].kind, "Player");
assert.equal(sampleJson.initialState.actors[0].combatComposition.stats.hitpoints, 82);
assert.equal(sampleJson.ticks[0].actors.length, 2);
assert.equal(sampleJson.ticks[0].actors[0].currentHitpoints, 77);
assert.deepEqual(Object.keys(sampleJson.ticks[0]), [
    "tick",
    "actors",
    "attacks",
    "damageApplications",
    "sceneChanges",
    "projectiles",
]);

assert.match(viewSource, /loadEngineModule/);
assert.match(viewSource, /RecordingPlayback\.LoadFromJson/);
assert.match(viewSource, /recordings\/minimal-encounter-recording\.json/);
assert.match(viewSource, /aria-label="Recording loader"/);
assert.match(viewSource, /aria-label="Recording metadata"/);
assert.match(viewSource, /aria-label="Tick navigation"/);
assert.match(viewSource, /aria-label="Actor inspector"/);
assert.match(viewSource, /aria-label="Recording scene"/);
assert.match(viewSource, /GetActorsJson/);
assert.match(viewSource, /Hitpoints/);
assert.match(viewSource, /Combat Composition/);
assert.match(viewSource, /Movement Target/);
assert.match(viewSource, /Attack Timer/);
assert.match(viewSource, /Previous/);
assert.match(viewSource, /Next/);
assert.match(viewSource, /Current Tick/);
assert.match(viewSource, /role="alert"/);
