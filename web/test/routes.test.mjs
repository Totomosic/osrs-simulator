import assert from "node:assert/strict";
import { mkdir } from "node:fs/promises";
import { dirname, resolve } from "node:path";
import { fileURLToPath, pathToFileURL } from "node:url";
import { build } from "esbuild";
import { createMemoryHistory, createRouter } from "vue-router";

const root = dirname(dirname(fileURLToPath(import.meta.url)));
const outfile = resolve(
    root,
    "../node_modules/.cache/osrs-simulator/routes-test.mjs",
);

await mkdir(dirname(outfile), { recursive: true });
await build({
    entryPoints: [resolve(root, "src/routes.ts")],
    outfile,
    bundle: true,
    format: "esm",
    platform: "node",
    external: ["vue", "vue-router"],
    logLevel: "silent",
});

const { createAppRoutes } = await import(pathToFileURL(outfile));

const DebugView = { template: "<main>Player Chase</main>" };
const DpsView = { template: "<main>DPS Calculator</main>" };
const router = createRouter({
    history: createMemoryHistory(),
    routes: createAppRoutes({
        debug: DebugView,
        dps: DpsView,
    }),
});

await router.push("/");
assert.equal(router.currentRoute.value.name, "debug");
assert.equal(router.currentRoute.value.matched[0].components.default, DebugView);

await router.push("/dps");
assert.equal(router.currentRoute.value.name, "dps");
assert.equal(router.currentRoute.value.matched[0].components.default, DpsView);

await router.push("/debug");
assert.equal(router.currentRoute.value.fullPath, "/");
assert.equal(router.currentRoute.value.name, "debug");
