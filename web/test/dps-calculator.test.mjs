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
    addPlayerAttackSetup,
    buildNpcDpsRequest,
    buildManualNpcDefenceComposition,
    buildSetupResultRows,
    createDefaultCalculatorState,
    deletePlayerAttackSetup,
    getEquipmentModeWeaponOptions,
    renamePlayerAttackSetup,
    setPlayerAttackType,
    formatDpsNumber,
    formatDpsPercent,
} = await import(pathToFileURL(outfile));

class FakeDpsService {
    CalculateExpected(request) {
        const dps = FakeDpsService.nextDpsValues.shift() ?? 4.560162645324535;
        FakeDpsService.lastRequest = request;

        return {
            attackRoll: 21560,
            defenceRoll: 12672,
            maximumHit: 31,
            hitChance: 0.7060896999211539,
            expectedDamagePerAttack: 10.944390348778885,
            secondsPerAttack: 2.4,
            dps,
        };
    }
}

FakeDpsService.nextDpsValues = [];

class FakeEquipmentPieceVector {
    constructor(pieces) {
        this.pieces = pieces;
    }

    size() {
        return this.pieces.length;
    }

    get(index) {
        return this.pieces[index];
    }

    delete() {}
}

class FakeEquipmentDatabase {
    static LoadDefault() {
        return new FakeEquipmentDatabase();
    }

    GetEquipmentPiece(id) {
        const piece = fakeEquipmentPieces.find((candidate) => candidate.id === id);
        if (piece === undefined) {
            throw new Error(`missing equipment piece ${id}`);
        }

        return piece;
    }

    GetEquipmentPiecesBySlot(slot) {
        return new FakeEquipmentPieceVector(
            fakeEquipmentPieces.filter((piece) => piece.slot === slot),
        );
    }
}

class FakeEquipmentSet {
    constructor() {
        this.pieces = [];
    }

    SetEquipmentPiece(piece) {
        this.pieces = this.pieces.filter((candidate) => candidate.slot !== piece.slot);
        this.pieces.push(piece);
    }

    BuildAttackComposition(stats, attackType) {
        const bonuses = createFakeEquipmentBonuses();
        for (const piece of this.pieces) {
            for (const [key, value] of Object.entries(piece.bonuses)) {
                bonuses[key] += value;
            }
        }

        const weapon =
            this.pieces.find((piece) => piece.slot === module.EquipmentSlot.Weapon)
                ?.weapon ?? { id: 0, range: 1, speed: 4 };

        return {
            attackType,
            stats,
            bonuses,
            weapon,
        };
    }
}

function createFakeEquipmentBonuses(overrides = {}) {
    return {
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
        ...overrides,
    };
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
    EquipmentSlot: {
        Weapon: 3,
        Amulet: 2,
    },
    DpsService: FakeDpsService,
    EquipmentDatabase: FakeEquipmentDatabase,
    EquipmentSet: FakeEquipmentSet,
};

const fakeEquipmentPieces = [
    {
        id: 1001,
        name: "Bronze scimitar",
        slot: module.EquipmentSlot.Weapon,
        bonuses: createFakeEquipmentBonuses({
            slashAttack: 7,
            meleeStrength: 6,
        }),
        hasWeapon: true,
        weapon: { id: 1, range: 1, speed: 4 },
    },
    {
        id: 1002,
        name: "Maple shortbow",
        slot: module.EquipmentSlot.Weapon,
        bonuses: createFakeEquipmentBonuses({
            rangedAttack: 29,
        }),
        hasWeapon: true,
        weapon: { id: 2, range: 7, speed: 4 },
    },
    {
        id: 1003,
        name: "Amulet of strength",
        slot: module.EquipmentSlot.Amulet,
        bonuses: createFakeEquipmentBonuses({
            meleeStrength: 10,
        }),
        hasWeapon: false,
        weapon: { id: 0, range: 1, speed: 4 },
    },
];

const state = createDefaultCalculatorState();
assert.equal(state.activePlayerAttackSetupIndex, 0);
assert.equal(state.playerAttackSetups.length, 1);
assert.equal(state.playerAttackSetups[0].name, "Baseline");
assert.equal(state.playerAttackSetups[0].mode, "manual");
assert.equal(state.playerAttackSetups[0].equipmentWeaponPieceId, "");
assert.equal(state.playerAttackSetups[0].attackType, "slash");
assert.equal(state.playerAttackSetups[0].combatStylePreset, "melee-accurate");
assert.equal(state.playerAttackSetups[0].attack, 99);
assert.equal(state.playerAttackSetups[0].strength, 99);
assert.equal(state.playerAttackSetups[0].ranged, 99);
assert.equal(state.playerAttackSetups[0].magic, 99);
assert.equal(state.playerAttackSetups[0].stabAttack, 0);
assert.equal(state.playerAttackSetups[0].slashAttack, 132);
assert.equal(state.playerAttackSetups[0].crushAttack, 0);
assert.equal(state.playerAttackSetups[0].magicAttack, 0);
assert.equal(state.playerAttackSetups[0].rangedAttack, 0);
assert.equal(state.playerAttackSetups[0].meleeStrength, 118);
assert.equal(state.playerAttackSetups[0].rangedStrength, 0);
assert.equal(state.playerAttackSetups[0].magicDamagePercent, 0);
assert.equal(state.playerAttackSetups[0].magicBaseMaximumHit, 0);
assert.equal(state.playerAttackSetups[0].weaponSpeedTicks, 4);
assert.equal(state.npcDefenceSetup.defence, 80);
assert.equal(state.npcDefenceSetup.magic, 1);
assert.equal(state.npcDefenceSetup.stabDefence, 0);
assert.equal(state.npcDefenceSetup.slashDefence, 80);
assert.equal(state.npcDefenceSetup.crushDefence, 0);
assert.equal(state.npcDefenceSetup.magicDefence, 0);
assert.equal(state.npcDefenceSetup.rangedDefenceLight, 0);
assert.equal(state.npcDefenceSetup.rangedDefenceStandard, 0);
assert.equal(state.npcDefenceSetup.rangedDefenceHeavy, 0);

const defaultDefenceComposition = buildManualNpcDefenceComposition(
    state.npcDefenceSetup,
);
assert.equal(defaultDefenceComposition.stats.defence, 80);
assert.equal(defaultDefenceComposition.stats.magic, 1);
assert.equal(defaultDefenceComposition.bonuses.stabDefence, 0);
assert.equal(defaultDefenceComposition.bonuses.slashDefence, 80);
assert.equal(defaultDefenceComposition.bonuses.crushDefence, 0);
assert.equal(defaultDefenceComposition.bonuses.magicDefence, 0);
assert.equal(defaultDefenceComposition.bonuses.rangedDefenceLight, 0);
assert.equal(defaultDefenceComposition.bonuses.rangedDefenceStandard, 0);
assert.equal(defaultDefenceComposition.bonuses.rangedDefenceHeavy, 0);

const request = buildNpcDpsRequest(module, state);
assert.equal(request.attackComposition.attackType, module.AttackType.Slash);
assert.equal(request.defenderKind, module.DefenderKind.Npc);
assert.equal(request.attackComposition.stats.attack, 99);
assert.equal(request.attackComposition.stats.strength, 99);
assert.equal(request.attackComposition.bonuses.slashAttack, 132);
assert.equal(request.attackComposition.bonuses.meleeStrength, 118);
assert.equal(request.attackerStyle.attack, 3);
assert.equal(request.attackerStyle.strength, 0);
assert.equal(request.defenceComposition.stats.defence, 80);
assert.equal(request.defenceComposition.stats.magic, 1);
assert.equal(request.defenceComposition.bonuses.slashDefence, 80);
assert.equal(request.attackComposition.weapon.id, 0);
assert.equal(request.attackComposition.weapon.range, 1);
assert.equal(request.attackComposition.weapon.speed, 4);
assert.equal(request.attackPrayerMultiplier, 1);
assert.equal(request.finalDamageMultiplier, 1);
assert.equal(request.magicBaseMaximumHit, 0);

const weaponOptions = getEquipmentModeWeaponOptions(module);
assert.deepEqual(
    weaponOptions.map((option) => [option.id, option.name]),
    [
        [1001, "Bronze scimitar"],
        [1002, "Maple shortbow"],
    ],
);

const equipmentState = createDefaultCalculatorState();
equipmentState.playerAttackSetups[0].mode = "equipment";
equipmentState.playerAttackSetups[0].equipmentWeaponPieceId = 1002;
setPlayerAttackType(equipmentState.playerAttackSetups[0], "ranged-heavy");
equipmentState.playerAttackSetups[0].ranged = 112;
equipmentState.playerAttackSetups[0].rangedAttack = 999;
equipmentState.playerAttackSetups[0].rangedStrength = 88;
equipmentState.playerAttackSetups[0].weaponSpeedTicks = 6;
const equipmentRequest = buildNpcDpsRequest(module, equipmentState);
assert.equal(
    equipmentRequest.attackComposition.attackType,
    module.AttackType.RangedHeavy,
);
assert.equal(equipmentRequest.attackComposition.stats.ranged, 112);
assert.equal(equipmentRequest.attackComposition.bonuses.rangedAttack, 29);
assert.equal(equipmentRequest.attackComposition.bonuses.rangedStrength, 0);
assert.equal(equipmentRequest.attackComposition.weapon.id, 2);
assert.equal(equipmentRequest.attackComposition.weapon.range, 7);
assert.equal(equipmentRequest.attackComposition.weapon.speed, 4);

const mixedModeState = createDefaultCalculatorState();
mixedModeState.playerAttackSetups[0].name = "Manual slash";
addPlayerAttackSetup(mixedModeState);
mixedModeState.playerAttackSetups[1].name = "Bow";
mixedModeState.playerAttackSetups[1].mode = "equipment";
mixedModeState.playerAttackSetups[1].equipmentWeaponPieceId = 1002;
setPlayerAttackType(mixedModeState.playerAttackSetups[1], "ranged-heavy");
assert.equal(mixedModeState.playerAttackSetups[0].mode, "manual");
FakeDpsService.nextDpsValues = [4, 5];
const mixedModeRows = buildSetupResultRows(module, mixedModeState);
assert.equal(mixedModeRows.length, 2);
assert.equal(mixedModeRows[0].name, "Manual slash");
assert.equal(mixedModeRows[1].name, "Bow");
assert.equal(mixedModeRows[1].dpsDifference, "+25.00%");

const editedState = createDefaultCalculatorState();
editedState.playerAttackSetups[0].attack = 118.9;
editedState.playerAttackSetups[0].slashAttack = 150;
editedState.playerAttackSetups[0].weaponSpeedTicks = 5.8;
editedState.npcDefenceSetup.defence = 90;
editedState.npcDefenceSetup.slashDefence = 110;
const editedRequest = buildNpcDpsRequest(module, editedState);
assert.equal(editedRequest.attackComposition.stats.attack, 118);
assert.equal(editedRequest.attackComposition.bonuses.slashAttack, 150);
assert.equal(editedRequest.attackComposition.weapon.speed, 5);
assert.equal(editedRequest.defenceComposition.stats.defence, 90);
assert.equal(editedRequest.defenceComposition.bonuses.slashDefence, 110);

const stabState = createDefaultCalculatorState();
stabState.playerAttackSetups[0].attackType = "stab";
stabState.playerAttackSetups[0].stabAttack = -12;
stabState.playerAttackSetups[0].combatStylePreset = "melee-aggressive";
stabState.npcDefenceSetup.stabDefence = -5;
const stabRequest = buildNpcDpsRequest(module, stabState);
assert.equal(stabRequest.attackComposition.attackType, module.AttackType.Stab);
assert.equal(stabRequest.attackComposition.bonuses.stabAttack, -12);
assert.equal(stabRequest.attackComposition.bonuses.slashAttack, 0);
assert.equal(stabRequest.attackerStyle.attack, 0);
assert.equal(stabRequest.attackerStyle.strength, 3);
assert.equal(stabRequest.defenceComposition.bonuses.stabDefence, -5);

const crushState = createDefaultCalculatorState();
crushState.playerAttackSetups[0].attackType = "crush";
crushState.playerAttackSetups[0].combatStylePreset = "melee-controlled";
crushState.playerAttackSetups[0].crushAttack = 84;
crushState.npcDefenceSetup.crushDefence = 37;
const crushRequest = buildNpcDpsRequest(module, crushState);
assert.equal(crushRequest.attackComposition.attackType, module.AttackType.Crush);
assert.equal(crushRequest.attackComposition.bonuses.crushAttack, 84);
assert.equal(crushRequest.attackerStyle.attack, 1);
assert.equal(crushRequest.attackerStyle.strength, 1);
assert.equal(crushRequest.attackerStyle.defence, 1);
assert.equal(crushRequest.defenceComposition.bonuses.crushDefence, 37);

const rangedState = createDefaultCalculatorState();
setPlayerAttackType(rangedState.playerAttackSetups[0], "ranged-heavy");
rangedState.playerAttackSetups[0].ranged = 112;
rangedState.playerAttackSetups[0].rangedAttack = 167;
rangedState.playerAttackSetups[0].rangedStrength = 89;
rangedState.playerAttackSetups[0].combatStylePreset = "ranged-longrange";
rangedState.npcDefenceSetup.rangedDefenceHeavy = 64;
const rangedRequest = buildNpcDpsRequest(module, rangedState);
assert.equal(rangedState.playerAttackSetups[0].combatStylePreset, "ranged-longrange");
assert.equal(rangedRequest.attackComposition.attackType, module.AttackType.RangedHeavy);
assert.equal(rangedRequest.attackComposition.stats.ranged, 112);
assert.equal(rangedRequest.attackComposition.bonuses.rangedAttack, 167);
assert.equal(rangedRequest.attackComposition.bonuses.rangedStrength, 89);
assert.equal(rangedRequest.attackerStyle.ranged, 0);
assert.equal(rangedRequest.attackerStyle.defence, 3);
assert.equal(rangedRequest.defenceComposition.bonuses.rangedDefenceHeavy, 64);

const rangedLightState = createDefaultCalculatorState();
rangedLightState.playerAttackSetups[0].attackType = "ranged-light";
assert.equal(
    buildNpcDpsRequest(module, rangedLightState).attackComposition.attackType,
    module.AttackType.RangedLight,
);

const rangedStandardState = createDefaultCalculatorState();
rangedStandardState.playerAttackSetups[0].attackType = "ranged-standard";
rangedStandardState.playerAttackSetups[0].combatStylePreset = "ranged-rapid";
assert.equal(
    buildNpcDpsRequest(module, rangedStandardState).attackComposition.attackType,
    module.AttackType.RangedStandard,
);
assert.equal(
    buildNpcDpsRequest(module, rangedStandardState).attackerStyle.ranged,
    0,
);

const magicState = createDefaultCalculatorState();
setPlayerAttackType(magicState.playerAttackSetups[0], "magic");
magicState.playerAttackSetups[0].magic = 94;
magicState.playerAttackSetups[0].magicAttack = 42.7;
magicState.playerAttackSetups[0].magicDamagePercent = 12.5;
magicState.playerAttackSetups[0].magicBaseMaximumHit = 24.9;
magicState.playerAttackSetups[0].combatStylePreset = "magic-defensive";
magicState.npcDefenceSetup.magic = 82.6;
magicState.npcDefenceSetup.magicDefence = -7;
const magicRequest = buildNpcDpsRequest(module, magicState);
assert.equal(magicState.playerAttackSetups[0].combatStylePreset, "magic-defensive");
assert.equal(magicRequest.attackComposition.attackType, module.AttackType.Magic);
assert.equal(magicRequest.attackComposition.stats.magic, 94);
assert.equal(magicRequest.attackComposition.bonuses.magicAttack, 42);
assert.equal(magicRequest.attackComposition.bonuses.magicDamagePercent, 12.5);
assert.equal(magicRequest.attackerStyle.magic, 0);
assert.equal(magicRequest.attackerStyle.defence, 3);
assert.equal(magicRequest.defenceComposition.stats.magic, 82);
assert.equal(magicRequest.defenceComposition.bonuses.magicDefence, -7);
assert.equal(magicRequest.magicBaseMaximumHit, 24);

const clampedState = createDefaultCalculatorState();
clampedState.playerAttackSetups[0].attack = 0;
clampedState.playerAttackSetups[0].strength = -2;
clampedState.playerAttackSetups[0].weaponSpeedTicks = 0;
clampedState.playerAttackSetups[0].magicDamagePercent = -0.25;
clampedState.playerAttackSetups[0].magicBaseMaximumHit = -1;
clampedState.npcDefenceSetup.defence = "";
const clampedRequest = buildNpcDpsRequest(module, clampedState);
assert.equal(clampedRequest.attackComposition.stats.attack, 1);
assert.equal(clampedRequest.attackComposition.stats.strength, 1);
assert.equal(clampedRequest.attackComposition.weapon.speed, 1);
assert.equal(clampedRequest.attackComposition.bonuses.magicDamagePercent, 0);
assert.equal(clampedRequest.magicBaseMaximumHit, 0);
assert.equal(clampedRequest.defenceComposition.stats.defence, 1);

const resetState = createDefaultCalculatorState();
resetState.playerAttackSetups[0].combatStylePreset = "melee-defensive";
setPlayerAttackType(resetState.playerAttackSetups[0], "ranged-standard");
assert.equal(resetState.playerAttackSetups[0].combatStylePreset, "ranged-accurate");
setPlayerAttackType(resetState.playerAttackSetups[0], "magic");
assert.equal(resetState.playerAttackSetups[0].combatStylePreset, "magic-standard");
setPlayerAttackType(resetState.playerAttackSetups[0], "slash");
assert.equal(resetState.playerAttackSetups[0].combatStylePreset, "melee-accurate");

const resultRow = buildSetupResultRows(module, editedState)[0];
assert.deepEqual(FakeDpsService.lastRequest, editedRequest);
assert.equal(resultRow.name, "Baseline");
assert.equal(resultRow.dpsDifference, "Baseline");
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

const multiSetupState = createDefaultCalculatorState();
multiSetupState.playerAttackSetups[0].name = "Slash baseline";
multiSetupState.playerAttackSetups[0].slashAttack = 132;
addPlayerAttackSetup(multiSetupState);
assert.equal(multiSetupState.activePlayerAttackSetupIndex, 1);
assert.equal(multiSetupState.playerAttackSetups.length, 2);
assert.equal(multiSetupState.playerAttackSetups[1].name, "Setup 2");
assert.equal(multiSetupState.playerAttackSetups[1].slashAttack, 132);
multiSetupState.playerAttackSetups[1].slashAttack = 160;
assert.equal(multiSetupState.playerAttackSetups[0].slashAttack, 132);

renamePlayerAttackSetup(multiSetupState, 1, "Strength swap");
assert.equal(multiSetupState.playerAttackSetups[1].name, "Strength swap");

deletePlayerAttackSetup(multiSetupState, 0);
assert.equal(multiSetupState.playerAttackSetups.length, 1);
assert.equal(multiSetupState.playerAttackSetups[0].name, "Strength swap");
deletePlayerAttackSetup(multiSetupState, 0);
assert.equal(multiSetupState.playerAttackSetups.length, 1);

for (let index = multiSetupState.playerAttackSetups.length; index < 12; index += 1) {
    addPlayerAttackSetup(multiSetupState);
}
assert.equal(multiSetupState.playerAttackSetups.length, 12);
assert.equal(addPlayerAttackSetup(multiSetupState), false);
assert.equal(multiSetupState.playerAttackSetups.length, 12);

const comparisonState = createDefaultCalculatorState();
addPlayerAttackSetup(comparisonState);
addPlayerAttackSetup(comparisonState);
FakeDpsService.nextDpsValues = [4, 5, 3];
const comparisonRows = buildSetupResultRows(module, comparisonState);
assert.equal(comparisonRows.length, 3);
assert.equal(comparisonRows[0].dpsDifference, "Baseline");
assert.equal(comparisonRows[1].dpsDifference, "+25.00%");
assert.equal(comparisonRows[2].dpsDifference, "-25.00%");

FakeDpsService.nextDpsValues = [0, 5];
const zeroBaselineRows = buildSetupResultRows(module, comparisonState).slice(0, 2);
assert.equal(zeroBaselineRows[0].dpsDifference, "Baseline");
assert.equal(zeroBaselineRows[1].dpsDifference, "n/a");
