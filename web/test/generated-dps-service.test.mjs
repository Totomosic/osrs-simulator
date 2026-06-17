import assert from "node:assert/strict";
import { readFile } from "node:fs/promises";
import { dirname, resolve } from "node:path";
import { fileURLToPath } from "node:url";
import createGeneratedEngineModule from "../src/wasm/generated/EngineModule.js";

const root = dirname(dirname(fileURLToPath(import.meta.url)));
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
};

const result = service.CalculateExpected(meleeDpsRequest);

assert.equal(result.attackRoll, 21560);
assert.equal(result.defenceRoll, 12672);
assert.equal(result.maximumHit, 31);
assert.equal(result.hitChance.toFixed(6), "0.706090");
assert.equal(result.expectedDamagePerAttack.toFixed(6), "10.944390");
assert.equal(result.secondsPerAttack, 2.4);
assert.equal(result.dps.toFixed(6), "4.560163");

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
