import assert from "node:assert/strict";
import { mkdir, writeFile } from "node:fs/promises";
import { dirname, resolve } from "node:path";
import { fileURLToPath, pathToFileURL } from "node:url";
import { build } from "esbuild";

const root = dirname(dirname(fileURLToPath(import.meta.url)));
const outfile = resolve(
    root,
    "../node_modules/.cache/osrs-simulator/player-chase-playback-test.mjs",
);

await mkdir(dirname(outfile), { recursive: true });
await build({
    entryPoints: [resolve(root, "src/playerChasePlayback.ts")],
    outfile,
    bundle: true,
    format: "esm",
    platform: "node",
    logLevel: "silent",
});
await writeFile(`${outfile}.stamp`, new Date().toISOString());

const { createPlayerChasePlayback, playbackTickIntervalMs } = await import(
    pathToFileURL(outfile)
);

class FakePlayerChaseScenario {
    constructor() {
        this.running = false;
        this.tick = 0;
    }

    step() {
        this.tick += 1;
    }

    reset() {
        this.running = false;
        this.tick = 0;
    }

    setRunning(running) {
        this.running = running;
    }

    isRunning() {
        return this.running;
    }
}

class FakeScheduler {
    constructor() {
        this.nextId = 1;
        this.intervals = [];
        this.cleared = [];
    }

    setInterval(callback, delayMs) {
        const id = this.nextId;
        this.nextId += 1;
        this.intervals.push({ id, callback, delayMs });
        return id;
    }

    clearInterval(id) {
        this.cleared.push(id);
    }
}

{
    const scenario = new FakePlayerChaseScenario();
    const scheduler = new FakeScheduler();
    let refreshCount = 0;
    const playback = createPlayerChasePlayback(
        scenario,
        () => {
            refreshCount += 1;
        },
        scheduler,
    );

    playback.resume();

    assert.equal(playbackTickIntervalMs, 600);
    assert.equal(scenario.running, true);
    assert.equal(scenario.tick, 0);
    assert.equal(refreshCount, 1);
    assert.equal(scheduler.intervals.length, 1);
    assert.equal(scheduler.intervals[0].delayMs, 600);

    scheduler.intervals[0].callback();

    assert.equal(scenario.tick, 1);
    assert.equal(refreshCount, 2);
}

{
    const scenario = new FakePlayerChaseScenario();
    const scheduler = new FakeScheduler();
    let refreshCount = 0;
    const playback = createPlayerChasePlayback(
        scenario,
        () => {
            refreshCount += 1;
        },
        scheduler,
    );

    playback.resume();
    const intervalId = scheduler.intervals[0].id;
    playback.pause();

    assert.equal(scenario.running, false);
    assert.deepEqual(scheduler.cleared, [intervalId]);
    assert.equal(refreshCount, 2);
}

{
    const scenario = new FakePlayerChaseScenario();
    const scheduler = new FakeScheduler();
    let refreshCount = 0;
    const playback = createPlayerChasePlayback(
        scenario,
        () => {
            refreshCount += 1;
        },
        scheduler,
    );

    playback.toggle();
    playback.toggle();

    assert.equal(scenario.running, false);
    assert.deepEqual(scheduler.cleared, [scheduler.intervals[0].id]);
    assert.equal(refreshCount, 2);
}

{
    const scenario = new FakePlayerChaseScenario();
    const scheduler = new FakeScheduler();
    let refreshCount = 0;
    const playback = createPlayerChasePlayback(
        scenario,
        () => {
            refreshCount += 1;
        },
        scheduler,
    );

    const stepped = playback.stepWhilePaused();

    assert.equal(stepped, true);
    assert.equal(scenario.tick, 1);
    assert.equal(scenario.running, false);
    assert.equal(refreshCount, 1);
    assert.equal(scheduler.intervals.length, 0);
}

{
    const scenario = new FakePlayerChaseScenario();
    const scheduler = new FakeScheduler();
    const playback = createPlayerChasePlayback(scenario, () => {}, scheduler);

    playback.resume();
    const stepped = playback.stepWhilePaused();

    assert.equal(stepped, false);
    assert.equal(scenario.tick, 0);
}

{
    const scenario = new FakePlayerChaseScenario();
    const scheduler = new FakeScheduler();
    let refreshCount = 0;
    const playback = createPlayerChasePlayback(
        scenario,
        () => {
            refreshCount += 1;
        },
        scheduler,
    );

    playback.resume();
    scheduler.intervals[0].callback();
    playback.reset();

    assert.equal(scenario.running, false);
    assert.equal(scenario.tick, 0);
    assert.deepEqual(scheduler.cleared, [scheduler.intervals[0].id]);
    assert.equal(refreshCount, 3);
}

{
    const scenario = new FakePlayerChaseScenario();
    const scheduler = new FakeScheduler();
    const playback = createPlayerChasePlayback(scenario, () => {}, scheduler);

    playback.resume();
    playback.stop();

    assert.deepEqual(scheduler.cleared, [scheduler.intervals[0].id]);
}
