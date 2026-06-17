import assert from "node:assert/strict";
import { mkdir, readFile } from "node:fs/promises";
import { dirname, resolve } from "node:path";
import { fileURLToPath, pathToFileURL } from "node:url";
import { build } from "esbuild";
import createGeneratedEngineModule from "../src/wasm/generated/EngineModule.js";

const root = dirname(dirname(fileURLToPath(import.meta.url)));
const outfile = resolve(
    root,
    "../node_modules/.cache/osrs-simulator/generated-dps-scenarios-test.mjs",
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

const { calculateDpsScenarioResults, createFixedDpsScenarios } = await import(
    pathToFileURL(outfile)
);

const wasmBinary = await readFile(
    resolve(root, "src/wasm/generated/EngineModule.wasm"),
);
const module = await createGeneratedEngineModule({ wasmBinary });
const service = new module.DpsService();

assert.equal(
    module.DpsService.CalculateEffectiveLevel(99, 1.15, 1.10, 3),
    136,
);
assert.equal(module.DpsService.CalculateAttackRoll(136, 132, 1.15), 30654);
assert.equal(module.DpsService.CalculateDefenceRoll(97, 80, 0.90), 12571);
assert.equal(
    module.DpsService.CalculateStandardMaximumHit(136, 118, 1.10),
    42,
);
assert.equal(
    module.DpsService.CalculateHitChance(30654, 12571).toFixed(6),
    "0.794927",
);
assert.equal(
    module.DpsService.CalculateHitChance(10000, 12571).toFixed(6),
    "0.397709",
);

const defaultStats = {
    attack: 1,
    strength: 1,
    defence: 1,
    ranged: 1,
    magic: 1,
    hitpoints: 10,
};
const defaultBonuses = {
    stabAttack: 0,
    slashAttack: 0,
    crushAttack: 0,
    magicAttack: 0,
    rangedAttack: 0,
    stabDefence: 0,
    slashDefence: 0,
    crushDefence: 0,
    magicDefence: 0,
    rangedDefenceLight: 0,
    rangedDefenceStandard: 0,
    rangedDefenceHeavy: 0,
    meleeStrength: 0,
    rangedStrength: 0,
    magicDamagePercent: 0,
};
const defaultStyle = {
    attack: 0,
    strength: 0,
    defence: 0,
    ranged: 0,
    magic: 0,
};

const meleeDpsRequest = {
    attackerStats: {
        ...defaultStats,
        attack: 99,
        strength: 99,
    },
    defenderStats: {
        ...defaultStats,
        defence: 80,
    },
    attackerBonuses: {
        ...defaultBonuses,
        slashAttack: 132,
        meleeStrength: 118,
    },
    defenderBonuses: {
        ...defaultBonuses,
        slashDefence: 80,
    },
    attackerStyle: {
        ...defaultStyle,
        attack: 3,
        strength: 3,
    },
    defenderStyle: defaultStyle,
    attackType: module.AttackType.Slash,
    defenderKind: module.DefenderKind.Player,
    weaponSpeedTicks: 4,
    attackPrayerMultiplier: 1.0,
    strengthPrayerMultiplier: 1.0,
    defencePrayerMultiplier: 1.0,
    attackLevelMultiplier: 1.0,
    strengthLevelMultiplier: 1.0,
    defenceLevelMultiplier: 1.0,
    finalAttackRollMultiplier: 1.0,
    finalDefenceRollMultiplier: 1.0,
    finalDamageMultiplier: 1.0,
    magicBaseMaximumHit: 0,
};

const result = service.CalculateExpected(meleeDpsRequest);
const npcMeleeResult = service.CalculateExpected({
    ...meleeDpsRequest,
    defenderKind: module.DefenderKind.Npc,
});

assert.equal(result.attackRoll, 21560);
assert.equal(result.defenceRoll, 12672);
assert.equal(result.maximumHit, 31);
assert.equal(result.hitChance.toFixed(6), "0.706090");
assert.equal(result.expectedDamagePerAttack.toFixed(6), "10.944390");
assert.equal(result.secondsPerAttack, 2.4);
assert.equal(result.dps.toFixed(6), "4.560163");
assert.equal(npcMeleeResult.defenceRoll, 12816);
assert.equal(npcMeleeResult.hitChance.toFixed(6), "0.702750");

service.SetSeed(12345);
const firstSharedSample = service.SampleSingleAttack(meleeDpsRequest);
const secondSharedSample = service.SampleSingleAttack(meleeDpsRequest);
service.SetSeed(12345);
const replayedSharedSample = service.SampleSingleAttack(meleeDpsRequest);
const seededArgumentSample = service.SampleSingleAttackWithSeed(
    meleeDpsRequest,
    12345,
);
const nextSharedSample = service.SampleSingleAttack(meleeDpsRequest);
const aggregateSample = service.SampleAttacksWithSeed(
    meleeDpsRequest,
    5,
    12345,
);

const rangedDpsRequest = {
    ...meleeDpsRequest,
    attackerStats: {
        ...defaultStats,
        ranged: 99,
    },
    defenderStats: {
        ...defaultStats,
        defence: 70,
    },
    attackerBonuses: {
        ...defaultBonuses,
        rangedAttack: 100,
        rangedStrength: 80,
    },
    defenderBonuses: {
        ...defaultBonuses,
        rangedDefenceLight: 20,
        rangedDefenceStandard: 40,
        rangedDefenceHeavy: 60,
    },
    attackerStyle: {
        ...defaultStyle,
        ranged: 3,
    },
    attackType: module.AttackType.RangedHeavy,
    attackPrayerMultiplier: 1.10,
    strengthPrayerMultiplier: 1.05,
    attackLevelMultiplier: 1.02,
    strengthLevelMultiplier: 1.03,
    finalAttackRollMultiplier: 1.10,
    finalDamageMultiplier: 1.15,
};
const rangedResult = service.CalculateExpected(rangedDpsRequest);
const rangedLightResult = service.CalculateExpected({
    ...rangedDpsRequest,
    attackType: module.AttackType.RangedLight,
});
const rangedStandardResult = service.CalculateExpected({
    ...rangedDpsRequest,
    attackType: module.AttackType.RangedStandard,
});

const magicDpsRequest = {
    ...meleeDpsRequest,
    attackerStats: {
        ...defaultStats,
        magic: 90,
        strength: 99,
    },
    defenderStats: {
        ...defaultStats,
        defence: 65,
        magic: 66,
    },
    attackerBonuses: {
        ...defaultBonuses,
        magicAttack: 70,
        meleeStrength: 200,
        magicDamagePercent: 10,
    },
    defenderBonuses: {
        ...defaultBonuses,
        magicDefence: 40,
    },
    attackType: module.AttackType.Magic,
    weaponSpeedTicks: 5,
    magicBaseMaximumHit: 24,
    attackPrayerMultiplier: 1.05,
    finalDamageMultiplier: 1.20,
};
const magicResult = service.CalculateExpected(magicDpsRequest);
const npcMagicResult = service.CalculateExpected({
    ...magicDpsRequest,
    defenderKind: module.DefenderKind.Npc,
});

assert.equal(firstSharedSample.attackRoll, 21560);
assert.equal(firstSharedSample.defenceRoll, 12672);
assert.equal(firstSharedSample.maximumHit, 31);
assert.equal(firstSharedSample.hitChance.toFixed(6), "0.706090");
assert.equal(firstSharedSample.accuracyPassed, true);
assert.equal(firstSharedSample.sampledDamage, 29);
assert.equal(secondSharedSample.accuracyPassed, false);
assert.equal(secondSharedSample.sampledDamage, 0);
assert.equal(replayedSharedSample.sampledDamage, firstSharedSample.sampledDamage);
assert.equal(seededArgumentSample.sampledDamage, firstSharedSample.sampledDamage);
assert.equal(nextSharedSample.sampledDamage, secondSharedSample.sampledDamage);
assert.equal(aggregateSample.attackCount, 5);
assert.equal(aggregateSample.totalSampledDamage, 74);
assert.equal(aggregateSample.averageSampledDamagePerAttack.toFixed(6), "14.800000");
assert.equal(aggregateSample.sampledDps.toFixed(6), "6.166667");

assert.equal(rangedResult.attackRoll, 22008);
assert.equal(rangedResult.defenceRoll, 9672);
assert.equal(rangedResult.maximumHit, 31);
assert.equal(rangedResult.hitChance.toFixed(6), "0.780226");
assert.equal(rangedResult.dps.toFixed(6), "5.038961");
assert.equal(rangedLightResult.defenceRoll, 6552);
assert.equal(rangedLightResult.dps.toFixed(6), "5.496729");
assert.equal(rangedStandardResult.defenceRoll, 8112);
assert.equal(rangedStandardResult.dps.toFixed(6), "5.267845");

assert.equal(magicResult.attackRoll, 13668);
assert.equal(magicResult.defenceRoll, 7592);
assert.equal(magicResult.maximumHit, 31);
assert.equal(magicResult.hitChance.toFixed(6), "0.722218");
assert.equal(magicResult.dps.toFixed(6), "3.731460");
assert.equal(npcMagicResult.defenceRoll, 7800);
assert.equal(npcMagicResult.hitChance.toFixed(6), "0.714610");

const fixedScenarioExpectations = [
    {
        name: "Melee slash tracer",
        attackRoll: 21560,
        defenceRoll: 12672,
        maximumHit: 31,
        hitChance: "0.706090",
        expectedDamagePerAttack: "10.944390",
        secondsPerAttack: "2.4",
        dps: "4.560163",
        accuracyPassed: true,
        sampledDamage: 29,
        attackCount: 5,
        totalSampledDamage: 74,
        averageSampledDamagePerAttack: "14.800000",
        sampledDps: "6.166667",
    },
    {
        name: "NPC melee slash tracer",
        attackRoll: 21560,
        defenceRoll: 12816,
        maximumHit: 31,
        hitChance: "0.702750",
        expectedDamagePerAttack: "10.892630",
        secondsPerAttack: "2.4",
        dps: "4.538596",
        accuracyPassed: true,
        sampledDamage: 20,
        attackCount: 5,
        totalSampledDamage: 63,
        averageSampledDamagePerAttack: "12.600000",
        sampledDps: "5.250000",
    },
    {
        name: "Ranged light tracer",
        attackRoll: 22008,
        defenceRoll: 6552,
        maximumHit: 31,
        hitChance: "0.851106",
        expectedDamagePerAttack: "13.192149",
        secondsPerAttack: "2.4",
        dps: "5.496729",
        accuracyPassed: false,
        sampledDamage: 0,
        attackCount: 5,
        totalSampledDamage: 70,
        averageSampledDamagePerAttack: "14.000000",
        sampledDps: "5.833333",
    },
    {
        name: "Ranged standard tracer",
        attackRoll: 22008,
        defenceRoll: 8112,
        maximumHit: 31,
        hitChance: "0.815666",
        expectedDamagePerAttack: "12.642828",
        secondsPerAttack: "2.4",
        dps: "5.267845",
        accuracyPassed: true,
        sampledDamage: 16,
        attackCount: 5,
        totalSampledDamage: 68,
        averageSampledDamagePerAttack: "13.600000",
        sampledDps: "5.666667",
    },
    {
        name: "Ranged heavy tracer",
        attackRoll: 22008,
        defenceRoll: 9672,
        maximumHit: 31,
        hitChance: "0.780226",
        expectedDamagePerAttack: "12.093507",
        secondsPerAttack: "2.4",
        dps: "5.038961",
        accuracyPassed: true,
        sampledDamage: 23,
        attackCount: 5,
        totalSampledDamage: 82,
        averageSampledDamagePerAttack: "16.400000",
        sampledDps: "6.833333",
    },
    {
        name: "Magic fixed-spell tracer",
        attackRoll: 13668,
        defenceRoll: 7592,
        maximumHit: 31,
        hitChance: "0.722218",
        expectedDamagePerAttack: "11.194381",
        secondsPerAttack: "3.0",
        dps: "3.731460",
        accuracyPassed: true,
        sampledDamage: 18,
        attackCount: 5,
        totalSampledDamage: 49,
        averageSampledDamagePerAttack: "9.800000",
        sampledDps: "3.266667",
    },
    {
        name: "NPC magic fixed-spell tracer",
        attackRoll: 13668,
        defenceRoll: 7800,
        maximumHit: 31,
        hitChance: "0.714610",
        expectedDamagePerAttack: "11.076450",
        secondsPerAttack: "3.0",
        dps: "3.692150",
        accuracyPassed: false,
        sampledDamage: 0,
        attackCount: 5,
        totalSampledDamage: 22,
        averageSampledDamagePerAttack: "4.400000",
        sampledDps: "1.466667",
    },
];

const fixedScenarioResults = calculateDpsScenarioResults(
    module,
    createFixedDpsScenarios(module),
);

assert.equal(fixedScenarioResults.length, fixedScenarioExpectations.length);

for (const [index, expectation] of fixedScenarioExpectations.entries()) {
    const entry = fixedScenarioResults[index];

    assert.equal(entry.scenario.name, expectation.name);
    assert.equal(entry.result.attackRoll, expectation.attackRoll);
    assert.equal(entry.result.defenceRoll, expectation.defenceRoll);
    assert.equal(entry.result.maximumHit, expectation.maximumHit);
    assert.equal(entry.result.hitChance.toFixed(6), expectation.hitChance);
    assert.equal(
        entry.result.expectedDamagePerAttack.toFixed(6),
        expectation.expectedDamagePerAttack,
    );
    assert.equal(
        entry.result.secondsPerAttack.toFixed(1),
        expectation.secondsPerAttack,
    );
    assert.equal(entry.result.dps.toFixed(6), expectation.dps);
    assert.equal(
        entry.sampledResult.accuracyPassed,
        expectation.accuracyPassed,
    );
    assert.equal(entry.sampledResult.sampledDamage, expectation.sampledDamage);
    assert.equal(entry.aggregateResult.attackCount, expectation.attackCount);
    assert.equal(
        entry.aggregateResult.totalSampledDamage,
        expectation.totalSampledDamage,
    );
    assert.equal(
        entry.aggregateResult.averageSampledDamagePerAttack.toFixed(6),
        expectation.averageSampledDamagePerAttack,
    );
    assert.equal(
        entry.aggregateResult.sampledDps.toFixed(6),
        expectation.sampledDps,
    );
}
