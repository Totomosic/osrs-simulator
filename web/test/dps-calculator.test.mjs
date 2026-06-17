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
    createFixedDpsScenarios,
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

    SampleAttacksWithSeed(request, attackCount, seed) {
        FakeDpsService.lastAggregateRequest = request;
        FakeDpsService.lastAggregateAttackCount = attackCount;
        FakeDpsService.lastAggregateSeed = seed;

        return {
            attackCount,
            totalSampledDamage: 74,
            averageSampledDamagePerAttack: 14.8,
            sampledDps: 6.166666666666667,
        };
    }
}

const module = {
    AttackType: {
        Stab: 0,
        Slash: 1,
        Crush: 2,
        Magic: 3,
        RangedLight: 4,
        RangedStandard: 5,
        RangedHeavy: 6,
    },
    DpsService: FakeDpsService,
};

const scenarios = createFixedMeleeDpsScenarios(module);
const defaultScenarios = createFixedDpsScenarios(module);
assert.equal(defaultScenarios.length, 5);
assert.equal(scenarios.length, 5);
assert.equal(scenarios[0].name, "Melee slash tracer");
assert.equal(scenarios[0].attackTypeLabel, "Slash");
assert.equal(scenarios[0].sampleSeed, 12345);
assert.equal(scenarios[0].sampleAttackCount, 5);
assert.equal(scenarios[0].request.attackType, module.AttackType.Slash);
assert.equal(scenarios[0].request.attackerStats.attack, 99);
assert.equal(scenarios[0].request.attackerStats.strength, 99);
assert.equal(scenarios[0].request.attackerBonuses.slashAttack, 132);
assert.equal(scenarios[0].request.attackerBonuses.meleeStrength, 118);
assert.equal(scenarios[0].request.defenderStats.defence, 80);
assert.equal(scenarios[0].request.defenderBonuses.slashDefence, 80);
assert.equal(scenarios[0].request.weaponSpeedTicks, 4);
assert.equal(scenarios[1].name, "Ranged light tracer");
assert.equal(scenarios[1].attackTypeLabel, "Ranged Light");
assert.equal(scenarios[1].request.attackType, module.AttackType.RangedLight);
assert.equal(scenarios[1].request.attackerStats.ranged, 99);
assert.equal(scenarios[1].request.attackerBonuses.rangedAttack, 100);
assert.equal(scenarios[1].request.attackerBonuses.rangedStrength, 80);
assert.equal(scenarios[1].request.defenderBonuses.rangedDefenceLight, 20);
assert.equal(scenarios[1].request.defenderBonuses.rangedDefenceHeavy, 60);
assert.equal(scenarios[1].request.attackPrayerMultiplier, 1.10);
assert.equal(scenarios[1].request.strengthPrayerMultiplier, 1.05);
assert.equal(scenarios[2].name, "Ranged standard tracer");
assert.equal(scenarios[2].attackTypeLabel, "Ranged Standard");
assert.equal(scenarios[2].request.attackType, module.AttackType.RangedStandard);
assert.equal(scenarios[2].request.defenderBonuses.rangedDefenceStandard, 40);
assert.equal(scenarios[3].name, "Ranged heavy tracer");
assert.equal(scenarios[3].attackTypeLabel, "Ranged Heavy");
assert.equal(scenarios[3].request.attackType, module.AttackType.RangedHeavy);
assert.equal(scenarios[3].request.defenderBonuses.rangedDefenceHeavy, 60);
assert.equal(scenarios[4].name, "Magic fixed-spell tracer");
assert.equal(scenarios[4].attackTypeLabel, "Magic");
assert.equal(scenarios[4].request.attackType, module.AttackType.Magic);
assert.equal(scenarios[4].request.attackerStats.magic, 90);
assert.equal(scenarios[4].request.attackerBonuses.magicAttack, 70);
assert.equal(scenarios[4].request.attackerBonuses.magicDamagePercent, 10);
assert.equal(scenarios[4].request.magicBaseMaximumHit, 24);

const results = calculateDpsScenarioResults(module, defaultScenarios);
assert.equal(results.length, 5);
assert.equal(FakeDpsService.lastRequest, defaultScenarios[4].request);
assert.equal(FakeDpsService.lastSampleRequest, defaultScenarios[4].request);
assert.equal(FakeDpsService.lastSampleSeed, 34567);
assert.equal(FakeDpsService.lastAggregateRequest, defaultScenarios[4].request);
assert.equal(FakeDpsService.lastAggregateAttackCount, 5);
assert.equal(FakeDpsService.lastAggregateSeed, 34567);
assert.equal(results[0].result.attackRoll, 21560);
assert.equal(results[0].result.defenceRoll, 12672);
assert.equal(results[0].result.maximumHit, 31);
assert.equal(results[0].sampledResult.accuracyPassed, true);
assert.equal(results[0].sampledResult.sampledDamage, 29);
assert.equal(results[0].aggregateResult.attackCount, 5);
assert.equal(results[0].aggregateResult.totalSampledDamage, 74);
assert.equal(results[0].aggregateResult.sampledDps.toFixed(6), "6.166667");
assert.equal(formatDpsNumber(results[0].result.dps), "4.560");
