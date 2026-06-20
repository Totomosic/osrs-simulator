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
    equipmentSlotControls,
    getEquipmentModeWeaponOptions,
    getEquipmentModeSlotOptions,
    getNpcDefenceOptions,
    getSavedCombatCompositionOptions,
    loadSavedCombatCompositionsFromStorage,
    loadEquipmentDataset,
    persistSavedCombatCompositionsToStorage,
    renamePlayerAttackSetup,
    saveActivePlayerAttackSetupAsCombatComposition,
    savedCombatCompositionStorageKey,
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

class FakeNpcDefinitionVector {
    constructor(definitions) {
        this.definitions = definitions;
    }

    size() {
        return this.definitions.length;
    }

    get(index) {
        return this.definitions[index];
    }

    delete() {}
}

class FakeCombatCompositionRecordVector {
    constructor(records) {
        this.records = records;
    }

    size() {
        return this.records.length;
    }

    get(index) {
        return this.records[index];
    }

    delete() {}
}

class FakeEquipmentDatabase {
    static nextManifestJson = "";
    static nextEquipmentJson = "";
    static nextWeaponsJson = "";
    static nextCombatCompositionsJson = "";
    static nextNpcsJson = "";

    static LoadFromJsonDocuments(
        manifestJson,
        equipmentJson,
        weaponsJson,
        combatCompositionsJson,
        npcsJson,
    ) {
        FakeEquipmentDatabase.nextManifestJson = manifestJson;
        FakeEquipmentDatabase.nextEquipmentJson = equipmentJson;
        FakeEquipmentDatabase.nextWeaponsJson = weaponsJson;
        FakeEquipmentDatabase.nextCombatCompositionsJson =
            combatCompositionsJson;
        FakeEquipmentDatabase.nextNpcsJson = npcsJson;
        return {
            GetEquipmentDatabase() {
                return new FakeEquipmentDatabase();
            },
            GetWeaponDatabase() {
                return new FakeWeaponDatabase();
            },
            GetCombatCompositionDatabase() {
                return new FakeCombatCompositionDatabase();
            },
            GetNpcDatabase() {
                return new FakeNpcDatabase();
            },
        };
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

class FakeWeaponDatabase {
    GetWeaponRecord(id) {
        const record = fakeWeaponRecords.find(
            (candidate) => candidate.weapon.id === id,
        );
        if (record === undefined) {
            throw new Error(`missing weapon ${id}`);
        }

        return record;
    }
}

class FakeCombatCompositionDatabase {
    constructor() {
        this.records = fakeCombatCompositionRecords.map((record) => ({
            ...record,
        }));
    }

    GetCombatCompositionRecord(id) {
        const record = this.records.find(
            (candidate) => candidate.id === BigInt(id),
        );
        if (record === undefined) {
            throw new Error(`missing combat composition ${id}`);
        }

        return record;
    }

    GetCombatCompositionRecordsBySource(source) {
        return new FakeCombatCompositionRecordVector(
            this.records.filter((record) => record.source === source),
        );
    }

    LoadSavedCombatCompositionRecordsFromJson(json) {
        const document = JSON.parse(json);
        this.records = [
            ...this.records.filter((record) => record.source !== 1),
            ...document.savedCombatCompositions.map((record) => ({
                id: BigInt(record.id),
                name: record.name,
                source: 1,
                composition: {
                    stats: record.stats,
                    bonuses: {
                        ...createFakeEquipmentBonuses(),
                        ...record.bonuses,
                    },
                    attackType: module.AttackType.Slash,
                    magicBaseMaximumHit: record.magicBaseMaximumHit,
                    weapon: fakeWeaponRecords.find(
                        (candidate) => candidate.weapon.id === record.weaponId,
                    ).weapon,
                },
            })),
        ];
    }

    ExportSavedCombatCompositionRecordsToJson() {
        return JSON.stringify({
            version: 1,
            savedCombatCompositions: this.records
                .filter((record) => record.source === 1)
                .map((record) => ({
                    id: record.id.toString(),
                    name: record.name,
                    stats: record.composition.stats,
                    bonuses: record.composition.bonuses,
                    attackType: "slash",
                    magicBaseMaximumHit:
                        record.composition.magicBaseMaximumHit,
                    weaponId: Number(record.composition.weapon.id),
                })),
        });
    }

    CreateSavedCombatCompositionRecord(name, composition) {
        const id = 9000000000000000000n + BigInt(
            this.records.filter((record) => record.source === 1).length,
        );
        this.records.push({
            id,
            name,
            source: 1,
            composition,
        });
        return id;
    }

    UpdateSavedCombatCompositionRecord(id, name, composition) {
        const record = this.GetCombatCompositionRecord(id);
        record.name = name;
        record.composition = composition;
    }

    DeleteSavedCombatCompositionRecord(id) {
        const previousLength = this.records.length;
        this.records = this.records.filter(
            (record) => record.id !== BigInt(id) || record.source !== 1,
        );
        return this.records.length !== previousLength;
    }
}

class FakeNpcDatabase {
    GetNpcDefinition(id) {
        const definition = fakeNpcDefinitions.find(
            (candidate) => candidate.id === BigInt(id),
        );
        if (definition === undefined) {
            throw new Error(`missing NPC ${id}`);
        }

        return definition;
    }

    GetAllNpcDefinitions() {
        return new FakeNpcDefinitionVector(fakeNpcDefinitions);
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

    BuildCombatComposition(stats, attackType, magicBaseMaximumHit, weaponDatabase) {
        const bonuses = createFakeEquipmentBonuses();
        for (const piece of this.pieces) {
            for (const [key, value] of Object.entries(piece.bonuses)) {
                bonuses[key] += value;
            }
        }

        const weapon =
            weaponDatabase.GetWeaponRecord(
                this.pieces.find((piece) => piece.slot === module.EquipmentSlot.Weapon)
                    ?.weaponId ?? 0,
            ).weapon;

        return {
            attackType,
            stats,
            bonuses,
            magicBaseMaximumHit,
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
        Head: 0,
        Cape: 1,
        Amulet: 2,
        Weapon: 3,
        Body: 4,
        Shield: 5,
        Legs: 6,
        Hands: 7,
        Feet: 8,
        Ring: 9,
        Ammo: 10,
    },
    CombatCompositionSource: {
        BuiltIn: 0,
        Saved: 1,
    },
    DpsService: FakeDpsService,
    EquipmentDatabase: FakeEquipmentDatabase,
    DatabaseService: FakeEquipmentDatabase,
    EquipmentSet: FakeEquipmentSet,
};

const fakeNpcComposition = {
    attackType: module.AttackType.Slash,
    stats: {
        attack: 1,
        strength: 1,
        defence: 1,
        ranged: 1,
        magic: 1,
        hitpoints: 5,
    },
    bonuses: createFakeEquipmentBonuses({
        slashDefence: 2,
        rangedDefenceLight: 3,
    }),
    magicBaseMaximumHit: 0,
    weapon: { id: 0, range: 1, speed: 4 },
};

const fakeCombatCompositionRecords = [
    {
        id: 1000n,
        name: "Goblin",
        source: 0,
        composition: fakeNpcComposition,
    },
];

const fakeNpcDefinitions = [
    {
        id: 2000n,
        name: "Goblin",
        hasCombatLevel: true,
        combatLevel: 2,
        size: 1,
        speed: 1,
        combatCompositionId: 1000n,
    },
];

const fakeWeaponRecords = [
    {
        weapon: { id: 0, range: 1, speed: 4 },
        name: "Unarmed",
        attackCallbackName: "standard_attack",
    },
    {
        weapon: { id: 1, range: 1, speed: 4 },
        name: "Bronze scimitar",
        attackCallbackName: "standard_attack",
    },
    {
        weapon: { id: 2, range: 7, speed: 4 },
        name: "Maple shortbow",
        attackCallbackName: "standard_attack",
    },
];

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
        weaponId: 1,
    },
    {
        id: 1002,
        name: "Maple shortbow",
        slot: module.EquipmentSlot.Weapon,
        bonuses: createFakeEquipmentBonuses({
            rangedAttack: 29,
        }),
        hasWeapon: true,
        weaponId: 2,
    },
    {
        id: 1003,
        name: "Amulet of strength",
        slot: module.EquipmentSlot.Amulet,
        bonuses: createFakeEquipmentBonuses({
            meleeStrength: 10,
        }),
        hasWeapon: false,
        weaponId: 0,
    },
    {
        id: 1004,
        name: "Berserker ring",
        slot: module.EquipmentSlot.Ring,
        bonuses: createFakeEquipmentBonuses({
            meleeStrength: 4,
        }),
        hasWeapon: false,
        weaponId: 0,
    },
    {
        id: 1005,
        name: "Barrows gloves",
        slot: module.EquipmentSlot.Hands,
        bonuses: createFakeEquipmentBonuses({
            slashAttack: 12,
            meleeStrength: 12,
        }),
        hasWeapon: false,
        weaponId: 0,
    },
];

const equipmentDataset = loadEquipmentDataset(
    module,
    '{"version":1,"documents":{"equipment":"equipment.json","weapons":"weapons.json","combatCompositions":"combat_compositions.json","npcs":"npcs.json"}}',
    '{"version":1,"equipmentPieces":[]}',
    '{"version":1,"weapons":[]}',
    '{"version":1,"combatCompositions":[]}',
    '{"version":1,"npcs":[]}',
);
assert.equal(
    FakeEquipmentDatabase.nextManifestJson,
    '{"version":1,"documents":{"equipment":"equipment.json","weapons":"weapons.json","combatCompositions":"combat_compositions.json","npcs":"npcs.json"}}',
);
assert.equal(
    FakeEquipmentDatabase.nextEquipmentJson,
    '{"version":1,"equipmentPieces":[]}',
);
assert.equal(FakeEquipmentDatabase.nextWeaponsJson, '{"version":1,"weapons":[]}');
assert.equal(
    FakeEquipmentDatabase.nextCombatCompositionsJson,
    '{"version":1,"combatCompositions":[]}',
);
assert.equal(FakeEquipmentDatabase.nextNpcsJson, '{"version":1,"npcs":[]}');
assert.equal(equipmentDataset.npcDatabase !== undefined, true);

const state = createDefaultCalculatorState();
assert.equal(state.activePlayerAttackSetupIndex, 0);
assert.equal(state.playerAttackSetups.length, 1);
assert.equal(state.playerAttackSetups[0].name, "Baseline");
assert.equal(state.playerAttackSetups[0].mode, "manual");
assert.equal(state.playerAttackSetups[0].selectedSavedCombatCompositionId, "");
assert.deepEqual(Object.keys(state.playerAttackSetups[0].equipmentPieceIds), [
    "head",
    "cape",
    "amulet",
    "weapon",
    "body",
    "shield",
    "legs",
    "hands",
    "feet",
    "ring",
    "ammo",
]);
assert.equal(state.playerAttackSetups[0].equipmentPieceIds.weapon, "");
assert.equal(state.playerAttackSetups[0].equipmentPieceIds.amulet, "");
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
assert.equal(state.npcDefenceSetup.mode, "manual");
assert.equal(state.npcDefenceSetup.selectedNpcId, "");

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

const npcOptions = getNpcDefenceOptions(equipmentDataset);
assert.deepEqual(
    npcOptions.map((option) => [
        option.id,
        option.name,
        option.combatLevel,
    ]),
    [[2000n, "Goblin", 2]],
);

const npcBackedState = createDefaultCalculatorState();
npcBackedState.npcDefenceSetup.mode = "npc";
npcBackedState.npcDefenceSetup.selectedNpcId = 2000n;
npcBackedState.npcDefenceSetup.defence = 99;
npcBackedState.npcDefenceSetup.slashDefence = 99;
const npcBackedRequest = buildNpcDpsRequest(
    module,
    npcBackedState,
    npcBackedState.activePlayerAttackSetupIndex,
    equipmentDataset,
);
assert.equal(npcBackedRequest.defenderKind, module.DefenderKind.Npc);
assert.equal(npcBackedRequest.defenceComposition.stats.defence, 1);
assert.equal(npcBackedRequest.defenceComposition.stats.magic, 1);
assert.equal(npcBackedRequest.defenceComposition.bonuses.slashDefence, 2);
assert.equal(npcBackedRequest.defenceComposition.bonuses.rangedDefenceLight, 3);

const storage = new Map();
const storageAdapter = {
    getItem(key) {
        return storage.get(key) ?? null;
    },
    setItem(key, value) {
        storage.set(key, value);
    },
    removeItem(key) {
        storage.delete(key);
    },
};
const savedState = createDefaultCalculatorState();
savedState.playerAttackSetups[0].magicBaseMaximumHit = 24;
const savedId = saveActivePlayerAttackSetupAsCombatComposition(
    module,
    savedState,
    equipmentDataset,
    "Saved baseline",
    storageAdapter,
);
assert.equal(savedId, 9000000000000000000n);
assert.equal(savedState.playerAttackSetups[0].mode, "saved");
assert.equal(savedState.playerAttackSetups[0].selectedSavedCombatCompositionId, savedId);
assert.deepEqual(
    getSavedCombatCompositionOptions(equipmentDataset).map((option) => [
        option.id,
        option.name,
    ]),
    [[9000000000000000000n, "Saved baseline"]],
);
assert.equal(storage.get(savedCombatCompositionStorageKey).includes("Saved baseline"), true);

const savedBackedRequest = buildNpcDpsRequest(
    module,
    savedState,
    savedState.activePlayerAttackSetupIndex,
    equipmentDataset,
);
assert.equal(savedBackedRequest.attackComposition.stats.attack, 99);
assert.equal(savedBackedRequest.attackComposition.bonuses.slashAttack, 132);
assert.equal(savedBackedRequest.magicBaseMaximumHit, 24);

const reloadedDataset = loadEquipmentDataset(
    module,
    '{"version":1,"documents":{"equipment":"equipment.json","weapons":"weapons.json","combatCompositions":"combat_compositions.json","npcs":"npcs.json"}}',
    '{"version":1,"equipmentPieces":[]}',
    '{"version":1,"weapons":[]}',
    '{"version":1,"combatCompositions":[]}',
    '{"version":1,"npcs":[]}',
);
assert.equal(loadSavedCombatCompositionsFromStorage(reloadedDataset, storageAdapter), true);
assert.deepEqual(
    getSavedCombatCompositionOptions(reloadedDataset).map((option) => [
        option.id,
        option.name,
    ]),
    [[9000000000000000000n, "Saved baseline"]],
);
persistSavedCombatCompositionsToStorage(reloadedDataset, storageAdapter);
assert.equal(storage.get(savedCombatCompositionStorageKey).includes("Saved baseline"), true);

const weaponOptions = getEquipmentModeWeaponOptions(module, equipmentDataset);
assert.deepEqual(
    weaponOptions.map((option) => [option.id, option.name]),
    [
        [1001, "Bronze scimitar"],
        [1002, "Maple shortbow"],
    ],
);

const slotOptions = getEquipmentModeSlotOptions(module, equipmentDataset);
assert.deepEqual(
    slotOptions.map((slot) => slot.key),
    equipmentSlotControls.map((slot) => slot.key),
);
assert.deepEqual(
    slotOptions.find((slot) => slot.key === "weapon").options.map((option) => [
        option.id,
        option.name,
    ]),
    [
        [1001, "Bronze scimitar"],
        [1002, "Maple shortbow"],
    ],
);
assert.deepEqual(
    slotOptions.find((slot) => slot.key === "amulet").options.map((option) => [
        option.id,
        option.name,
    ]),
    [[1003, "Amulet of strength"]],
);
assert.deepEqual(
    slotOptions.find((slot) => slot.key === "ring").options.map((option) => [
        option.id,
        option.name,
    ]),
    [[1004, "Berserker ring"]],
);
assert.deepEqual(
    slotOptions.find((slot) => slot.key === "head").options,
    [],
);

const equipmentState = createDefaultCalculatorState();
equipmentState.playerAttackSetups[0].mode = "equipment";
equipmentState.playerAttackSetups[0].equipmentPieceIds.weapon = 1002;
setPlayerAttackType(equipmentState.playerAttackSetups[0], "ranged-heavy");
equipmentState.playerAttackSetups[0].ranged = 112;
equipmentState.playerAttackSetups[0].rangedAttack = 999;
equipmentState.playerAttackSetups[0].rangedStrength = 88;
equipmentState.playerAttackSetups[0].weaponSpeedTicks = 6;
const equipmentRequest = buildNpcDpsRequest(
    module,
    equipmentState,
    equipmentState.activePlayerAttackSetupIndex,
    equipmentDataset,
);
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

const fullEquipmentState = createDefaultCalculatorState();
fullEquipmentState.playerAttackSetups[0].mode = "equipment";
fullEquipmentState.playerAttackSetups[0].equipmentPieceIds.weapon = 1001;
fullEquipmentState.playerAttackSetups[0].equipmentPieceIds.amulet = 1003;
fullEquipmentState.playerAttackSetups[0].equipmentPieceIds.ring = 1004;
fullEquipmentState.playerAttackSetups[0].equipmentPieceIds.hands = 1005;
fullEquipmentState.playerAttackSetups[0].attack = 107;
fullEquipmentState.playerAttackSetups[0].strength = 118;
fullEquipmentState.playerAttackSetups[0].slashAttack = 999;
fullEquipmentState.playerAttackSetups[0].meleeStrength = 999;
fullEquipmentState.playerAttackSetups[0].weaponSpeedTicks = 7;
const fullEquipmentRequest = buildNpcDpsRequest(
    module,
    fullEquipmentState,
    fullEquipmentState.activePlayerAttackSetupIndex,
    equipmentDataset,
);
assert.equal(fullEquipmentRequest.attackComposition.stats.attack, 107);
assert.equal(fullEquipmentRequest.attackComposition.stats.strength, 118);
assert.equal(fullEquipmentRequest.attackComposition.attackType, module.AttackType.Slash);
assert.equal(fullEquipmentRequest.attackComposition.bonuses.slashAttack, 19);
assert.equal(fullEquipmentRequest.attackComposition.bonuses.meleeStrength, 32);
assert.equal(fullEquipmentRequest.attackComposition.weapon.id, 1);
assert.equal(fullEquipmentRequest.attackComposition.weapon.speed, 4);

const mixedModeState = createDefaultCalculatorState();
mixedModeState.playerAttackSetups[0].name = "Manual slash";
addPlayerAttackSetup(mixedModeState);
mixedModeState.playerAttackSetups[1].name = "Bow";
mixedModeState.playerAttackSetups[1].mode = "equipment";
mixedModeState.playerAttackSetups[1].equipmentPieceIds.weapon = 1002;
setPlayerAttackType(mixedModeState.playerAttackSetups[1], "ranged-heavy");
assert.equal(mixedModeState.playerAttackSetups[0].mode, "manual");
FakeDpsService.nextDpsValues = [4, 5];
const mixedModeRows = buildSetupResultRows(
    module,
    mixedModeState,
    equipmentDataset,
);
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
multiSetupState.playerAttackSetups[1].equipmentPieceIds.weapon = 1001;
assert.equal(multiSetupState.playerAttackSetups[0].equipmentPieceIds.weapon, "");
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
