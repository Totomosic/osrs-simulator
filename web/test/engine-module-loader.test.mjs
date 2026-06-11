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
    entryPoints: [resolve(root, "src/wasm/EngineModule.ts")],
    outfile,
    bundle: true,
    format: "esm",
    platform: "node",
    logLevel: "silent",
});

const { loadEngineModule } = await import(pathToFileURL(outfile));

let locateFileResult;
const module = await loadEngineModule(async (options) => {
    locateFileResult = options.locateFile("EngineModule.wasm");

    class Engine {
        constructor() {
            this.currentTick = 0;
            this.world = new World();
        }

        Step() {
            this.currentTick += 1;
        }

        GetCurrentTick() {
            return this.currentTick;
        }

        GetWorld() {
            return this.world;
        }
    }

    class World {
        GetDefaultSceneId() {
            return 1;
        }
    }

    return { Engine, World };
});

const engine = new module.Engine();
const world = new module.World();

assert.equal(engine.GetCurrentTick(), 0);
engine.Step();
assert.equal(engine.GetCurrentTick(), 1);
assert.equal(engine.GetWorld().GetDefaultSceneId(), 1);
assert.equal(world.GetDefaultSceneId(), 1);
assert.equal(locateFileResult, "/wasm/EngineModule.wasm");
await writeFile(`${outfile}.stamp`, new Date().toISOString());
