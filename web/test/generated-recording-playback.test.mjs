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
            actors: [],
            sceneEntities: [],
            projectiles: [],
        },
        ticks: [
            {
                tick: 1,
                actors: [],
                attacks: [],
                damageApplications: [],
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
assert.equal(playback.GetLastTick(), 1);
assert.equal(playback.NextTick(), true);
assert.equal(playback.GetCurrentTick(), 1);
assert.equal(playback.NextTick(), false);

playback.delete?.();
