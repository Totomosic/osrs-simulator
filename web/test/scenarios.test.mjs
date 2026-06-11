import assert from "node:assert/strict";
import { mkdir, writeFile } from "node:fs/promises";
import { dirname, resolve } from "node:path";
import { fileURLToPath, pathToFileURL } from "node:url";
import { build } from "esbuild";

const root = dirname(dirname(fileURLToPath(import.meta.url)));
const outfile = resolve(root, "../node_modules/.cache/osrs-simulator/scenarios-test.mjs");

await mkdir(dirname(outfile), { recursive: true });
await build({
    entryPoints: [resolve(root, "src/scenarios.ts")],
    outfile,
    bundle: true,
    format: "esm",
    platform: "node",
    logLevel: "silent",
});
await writeFile(`${outfile}.stamp`, new Date().toISOString());

const { buildRenderTiles, getSceneScreenCoordinate, runScenario } = await import(
    pathToFileURL(outfile)
);

{
    const scenario = runScenario("empty-world");
    const renderTiles = buildRenderTiles(scenario, 0);

    assert.equal(scenario.scene.width, 104);
    assert.equal(scenario.scene.height, 104);
    assert.equal(scenario.scene.planeCount, 4);
    assert.equal(renderTiles.length, 104 * 104);
    assert.deepEqual(renderTiles[0].coordinate, { x: 0, y: 0, plane: 0 });
    assert.deepEqual(renderTiles.at(-1).coordinate, { x: 103, y: 103, plane: 0 });
    assert.ok(renderTiles.every((tile) => tile.flags.length === 0));
    assert.ok(renderTiles.every((tile) => tile.wallObject === undefined));
    assert.ok(renderTiles.every((tile) => tile.gameObject === undefined));
    assert.deepEqual(
        getSceneScreenCoordinate({ x: 0, y: 0, plane: 0 }, scenario.scene, 10),
        { x: 0, y: 1030 },
    );
    assert.deepEqual(
        getSceneScreenCoordinate({ x: 103, y: 103, plane: 0 }, scenario.scene, 10),
        { x: 1030, y: 0 },
    );
}

{
    const scenario = runScenario("wall-blocker");
    const renderTiles = buildRenderTiles(scenario, 0);
    const wallTile = renderTiles.find(
        (tile) => tile.coordinate.x === 10 && tile.coordinate.y === 10,
    );
    const eastTile = renderTiles.find(
        (tile) => tile.coordinate.x === 11 && tile.coordinate.y === 10,
    );

    assert.deepEqual(scenario.wallObjects, [
        {
            id: 100,
            coordinate: { x: 10, y: 10, plane: 0 },
            directions: ["East"],
            collisionProfiles: [{ blocksMovement: true, blocksLineOfSight: false }],
        },
    ]);
    assert.ok(wallTile.flags.includes("BlockMovementEast"));
    assert.ok(wallTile.wallEdges.includes("East"));
    assert.ok(eastTile.flags.includes("BlockMovementWest"));
    assert.ok(eastTile.movementEdges.includes("West"));
}

{
    const scenario = runScenario("game-object-blocker");
    const renderTiles = buildRenderTiles(scenario, 0);
    const footprint = renderTiles.filter((tile) => tile.gameObject !== undefined);

    assert.deepEqual(scenario.gameObjects, [
        {
            id: 200,
            origin: { x: 12, y: 10, plane: 0 },
            direction: "North",
            sizeX: 2,
            sizeY: 2,
            footprint: [
                { x: 12, y: 10, plane: 0 },
                { x: 13, y: 10, plane: 0 },
                { x: 12, y: 11, plane: 0 },
                { x: 13, y: 11, plane: 0 },
            ],
            collisionProfile: { blocksMovement: true, blocksLineOfSight: true },
        },
    ]);
    assert.deepEqual(
        footprint.map((tile) => tile.key).sort(),
        ["0:12:10", "0:12:11", "0:13:10", "0:13:11"],
    );
    assert.ok(footprint.every((tile) => tile.flags.includes("BlockMovementObject")));
    assert.ok(footprint.every((tile) => tile.flags.includes("BlockLineOfSightFull")));
}
