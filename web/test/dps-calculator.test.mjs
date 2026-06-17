import assert from "node:assert/strict";
import { mkdir } from "node:fs/promises";
import { dirname, resolve } from "node:path";
import { fileURLToPath, pathToFileURL } from "node:url";
import { build } from "esbuild";

const root = dirname(dirname(fileURLToPath(import.meta.url)));
const outfile = resolve(
    root,
    "../node_modules/.cache/osrs-simulator/dps-calculator-test.mjs",
);

await mkdir(dirname(outfile), { recursive: true });
await build({
    entryPoints: [resolve(root, "src/dpsCalculator.ts")],
    outfile,
    bundle: true,
    format: "esm",
    platform: "node",
    logLevel: "silent",
});

const {
    calculateDpsScenarioResults,
    createFixedMeleeDpsScenarios,
    formatDpsNumber,
} = await import(pathToFileURL(outfile));

class FakeDpsService {
    CalculateExpected(request) {
        FakeDpsService.lastRequest = request;

        return {
            attackRoll: 21560,
            defenceRoll: 12672,
            maximumHit: 31,
            hitChance: 0.7060896999211539,
            expectedDamagePerAttack: 10.944390348778885,
            secondsPerAttack: 2.4,
            dps: 4.560162645324535,
        };
    }

    SampleSingleAttackWithSeed(request, seed) {
        FakeDpsService.lastSampleRequest = request;
        FakeDpsService.lastSampleSeed = seed;

        return {
            attackRoll: 21560,
            defenceRoll: 12672,
            maximumHit: 31,
            hitChance: 0.7060896999211539,
            expectedDamagePerAttack: 10.944390348778885,
            secondsPerAttack: 2.4,
            dps: 4.560162645324535,
            accuracyPassed: true,
            sampledDamage: 29,
        };
    }
}

const module = {
    AttackType: {
        Stab: 0,
        Slash: 1,
        Crush: 2,
    },
    DpsService: FakeDpsService,
};

const scenarios = createFixedMeleeDpsScenarios(module);
assert.equal(scenarios.length, 1);
assert.equal(scenarios[0].name, "Melee slash tracer");
assert.equal(scenarios[0].attackTypeLabel, "Slash");
assert.equal(scenarios[0].sampleSeed, 12345);
assert.equal(scenarios[0].request.attackType, module.AttackType.Slash);
assert.equal(scenarios[0].request.attackerStats.attack, 99);
assert.equal(scenarios[0].request.attackerStats.strength, 99);
assert.equal(scenarios[0].request.attackerBonuses.slashAttack, 132);
assert.equal(scenarios[0].request.attackerBonuses.meleeStrength, 118);
assert.equal(scenarios[0].request.defenderStats.defence, 80);
assert.equal(scenarios[0].request.defenderBonuses.slashDefence, 80);
assert.equal(scenarios[0].request.weaponSpeedTicks, 4);

const results = calculateDpsScenarioResults(module, scenarios);
assert.equal(results.length, 1);
assert.equal(FakeDpsService.lastRequest, scenarios[0].request);
assert.equal(FakeDpsService.lastSampleRequest, scenarios[0].request);
assert.equal(FakeDpsService.lastSampleSeed, 12345);
assert.equal(results[0].result.attackRoll, 21560);
assert.equal(results[0].result.defenceRoll, 12672);
assert.equal(results[0].result.maximumHit, 31);
assert.equal(results[0].sampledResult.accuracyPassed, true);
assert.equal(results[0].sampledResult.sampledDamage, 29);
assert.equal(formatDpsNumber(results[0].result.dps), "4.560");
