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
    buildNpcDpsRequest,
    buildSingleSetupResultRow,
    createDefaultCalculatorState,
    formatDpsNumber,
    formatDpsPercent,
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
    DefenderKind: {
        Player: 0,
        Npc: 1,
    },
    DpsService: FakeDpsService,
};

const state = createDefaultCalculatorState();
assert.equal(state.playerAttackSetup.name, "Baseline");
assert.equal(state.playerAttackSetup.attack, 99);
assert.equal(state.playerAttackSetup.strength, 99);
assert.equal(state.playerAttackSetup.slashAttack, 132);
assert.equal(state.playerAttackSetup.meleeStrength, 118);
assert.equal(state.playerAttackSetup.attackStyleBonus, 3);
assert.equal(state.playerAttackSetup.strengthStyleBonus, 3);
assert.equal(state.playerAttackSetup.weaponSpeedTicks, 4);
assert.equal(state.npcDefenceSetup.defence, 80);
assert.equal(state.npcDefenceSetup.slashDefence, 80);

const request = buildNpcDpsRequest(module, state);
assert.equal(request.attackType, module.AttackType.Slash);
assert.equal(request.defenderKind, module.DefenderKind.Npc);
assert.equal(request.attackerStats.attack, 99);
assert.equal(request.attackerStats.strength, 99);
assert.equal(request.attackerBonuses.slashAttack, 132);
assert.equal(request.attackerBonuses.meleeStrength, 118);
assert.equal(request.attackerStyle.attack, 3);
assert.equal(request.attackerStyle.strength, 3);
assert.equal(request.defenderStats.defence, 80);
assert.equal(request.defenderBonuses.slashDefence, 80);
assert.equal(request.weaponSpeedTicks, 4);
assert.equal(request.attackPrayerMultiplier, 1);
assert.equal(request.finalDamageMultiplier, 1);
assert.equal(request.magicBaseMaximumHit, 0);

const editedState = createDefaultCalculatorState();
editedState.playerAttackSetup.attack = 118;
editedState.playerAttackSetup.slashAttack = 150;
editedState.playerAttackSetup.weaponSpeedTicks = 5;
editedState.npcDefenceSetup.defence = 90;
editedState.npcDefenceSetup.slashDefence = 110;
const editedRequest = buildNpcDpsRequest(module, editedState);
assert.equal(editedRequest.attackerStats.attack, 118);
assert.equal(editedRequest.attackerBonuses.slashAttack, 150);
assert.equal(editedRequest.weaponSpeedTicks, 5);
assert.equal(editedRequest.defenderStats.defence, 90);
assert.equal(editedRequest.defenderBonuses.slashDefence, 110);

const resultRow = buildSingleSetupResultRow(module, editedState);
assert.deepEqual(FakeDpsService.lastRequest, editedRequest);
assert.equal(resultRow.name, "Baseline");
assert.equal(resultRow.attackRoll, "21560");
assert.equal(resultRow.defenceRoll, "12672");
assert.equal(resultRow.hitChance, "70.61%");
assert.equal(resultRow.maximumHit, "31");
assert.equal(resultRow.expectedDamagePerAttack, "10.944");
assert.equal(resultRow.secondsPerAttack, "2.4");
assert.equal(resultRow.dps, "4.560");

assert.equal(formatDpsNumber(4.560162645324535), "4.560");
assert.equal(formatDpsNumber(2.4, 1), "2.4");
assert.equal(formatDpsPercent(0.7060896999211539), "70.61%");
