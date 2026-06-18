import type {
    AttackType,
    CombatStats,
    DefenderKind,
    DpsRequest,
    DpsResult,
    DpsSampleAggregateResult,
    DpsSampleResult,
    EngineModule,
    EquipmentBonuses,
    StyleBonus,
} from "./wasm/EngineModule";

export interface DpsScenario {
    name: string;
    attackTypeLabel: string;
    sampleSeed: number;
    sampleAttackCount: number;
    request: DpsRequest;
}

export interface DpsScenarioResult {
    scenario: DpsScenario;
    result: DpsResult;
    sampledResult: DpsSampleResult;
    aggregateResult: DpsSampleAggregateResult;
}

export type CalculatorAttackType =
    | "stab"
    | "slash"
    | "crush"
    | "magic"
    | "ranged-light"
    | "ranged-standard"
    | "ranged-heavy";

export type CombatStylePreset =
    | "melee-accurate"
    | "melee-aggressive"
    | "melee-controlled"
    | "melee-defensive"
    | "ranged-accurate"
    | "ranged-rapid"
    | "ranged-longrange"
    | "magic-standard"
    | "magic-defensive";

type CalculatorNumber = number | string;

export interface PlayerAttackSetup {
    name: string;
    attackType: CalculatorAttackType;
    combatStylePreset: CombatStylePreset;
    attack: CalculatorNumber;
    strength: CalculatorNumber;
    ranged: CalculatorNumber;
    magic: CalculatorNumber;
    stabAttack: CalculatorNumber;
    slashAttack: CalculatorNumber;
    crushAttack: CalculatorNumber;
    magicAttack: CalculatorNumber;
    rangedAttack: CalculatorNumber;
    meleeStrength: CalculatorNumber;
    rangedStrength: CalculatorNumber;
    magicDamagePercent: CalculatorNumber;
    magicBaseMaximumHit: CalculatorNumber;
    weaponSpeedTicks: CalculatorNumber;
}

export interface NpcDefenceSetup {
    defence: CalculatorNumber;
    magic: CalculatorNumber;
    stabDefence: CalculatorNumber;
    slashDefence: CalculatorNumber;
    crushDefence: CalculatorNumber;
    magicDefence: CalculatorNumber;
    rangedDefenceLight: CalculatorNumber;
    rangedDefenceStandard: CalculatorNumber;
    rangedDefenceHeavy: CalculatorNumber;
}

export interface DpsCalculatorState {
    playerAttackSetup: PlayerAttackSetup;
    npcDefenceSetup: NpcDefenceSetup;
}

export interface DpsResultRow {
    name: string;
    attackRoll: string;
    defenceRoll: string;
    hitChance: string;
    maximumHit: string;
    expectedDamagePerAttack: string;
    secondsPerAttack: string;
    dps: string;
}

export function createDefaultCalculatorState(): DpsCalculatorState {
    return {
        playerAttackSetup: {
            name: "Baseline",
            attackType: "slash",
            combatStylePreset: "melee-accurate",
            attack: 99,
            strength: 99,
            ranged: 99,
            magic: 99,
            stabAttack: 0,
            slashAttack: 132,
            crushAttack: 0,
            magicAttack: 0,
            rangedAttack: 0,
            meleeStrength: 118,
            rangedStrength: 0,
            magicDamagePercent: 0,
            magicBaseMaximumHit: 0,
            weaponSpeedTicks: 4,
        },
        npcDefenceSetup: {
            defence: 80,
            magic: 1,
            stabDefence: 0,
            slashDefence: 80,
            crushDefence: 0,
            magicDefence: 0,
            rangedDefenceLight: 0,
            rangedDefenceStandard: 0,
            rangedDefenceHeavy: 0,
        },
    };
}

export function setPlayerAttackType(
    setup: PlayerAttackSetup,
    attackType: CalculatorAttackType,
): void {
    setup.attackType = attackType;
    setup.combatStylePreset = getDefaultCombatStylePreset(attackType);
}

export function buildNpcDpsRequest(
    module: EngineModule,
    state: DpsCalculatorState,
): DpsRequest {
    const { playerAttackSetup, npcDefenceSetup } = state;
    const attackType = playerAttackSetup.attackType;
    const attackerBonuses = {
        ...createDefaultEquipmentBonuses(),
        ...createAttackerBonusFields(playerAttackSetup),
    };
    const defenderBonuses = {
        ...createDefaultEquipmentBonuses(),
        ...createDefenderBonusFields(npcDefenceSetup),
    };

    return {
        attackerStats: {
            ...createDefaultCombatStats(),
            attack: sanitizeWholeNumber(playerAttackSetup.attack, 1),
            strength: sanitizeWholeNumber(playerAttackSetup.strength, 1),
            ranged: sanitizeWholeNumber(playerAttackSetup.ranged, 1),
            magic: sanitizeWholeNumber(playerAttackSetup.magic, 1),
        },
        defenderStats: {
            ...createDefaultCombatStats(),
            defence: sanitizeWholeNumber(npcDefenceSetup.defence, 1),
            magic: sanitizeWholeNumber(npcDefenceSetup.magic, 1),
        },
        attackerBonuses,
        defenderBonuses,
        attackerStyle: createCombatStyleBonus(playerAttackSetup.combatStylePreset),
        defenderStyle: createDefaultStyleBonus(),
        attackType: resolveModuleAttackType(module, attackType),
        defenderKind: module.DefenderKind.Npc,
        weaponSpeedTicks: sanitizeWholeNumber(
            playerAttackSetup.weaponSpeedTicks,
            1,
        ),
        attackPrayerMultiplier: 1.0,
        strengthPrayerMultiplier: 1.0,
        defencePrayerMultiplier: 1.0,
        attackLevelMultiplier: 1.0,
        strengthLevelMultiplier: 1.0,
        defenceLevelMultiplier: 1.0,
        finalAttackRollMultiplier: 1.0,
        finalDefenceRollMultiplier: 1.0,
        finalDamageMultiplier: 1.0,
        magicBaseMaximumHit: sanitizeWholeNumber(
            playerAttackSetup.magicBaseMaximumHit,
            0,
        ),
    };
}

export function buildSingleSetupResultRow(
    module: EngineModule,
    state: DpsCalculatorState,
): DpsResultRow {
    const service = new module.DpsService();
    const result = service.CalculateExpected(buildNpcDpsRequest(module, state));

    return createDpsResultRow(state.playerAttackSetup.name, result);
}

export function createDpsResultRow(
    name: string,
    result: DpsResult,
): DpsResultRow {
    return {
        name,
        attackRoll: result.attackRoll.toString(),
        defenceRoll: result.defenceRoll.toString(),
        hitChance: formatDpsPercent(result.hitChance),
        maximumHit: result.maximumHit.toString(),
        expectedDamagePerAttack: formatDpsNumber(
            result.expectedDamagePerAttack,
        ),
        secondsPerAttack: formatDpsNumber(result.secondsPerAttack, 1),
        dps: formatDpsNumber(result.dps),
    };
}

export function createFixedMeleeDpsScenarios(
    module: EngineModule,
): DpsScenario[] {
    return createFixedDpsScenarios(module);
}

export function createFixedDpsScenarios(
    module: EngineModule,
): DpsScenario[] {
    return [
        {
            name: "Melee slash tracer",
            attackTypeLabel: "Slash",
            sampleSeed: 12345,
            sampleAttackCount: 5,
            request: createMeleeSlashRequest(
                module.AttackType.Slash,
                module.DefenderKind.Player,
            ),
        },
        {
            name: "NPC melee slash tracer",
            attackTypeLabel: "Slash",
            sampleSeed: 12346,
            sampleAttackCount: 5,
            request: createMeleeSlashRequest(
                module.AttackType.Slash,
                module.DefenderKind.Npc,
            ),
        },
        {
            name: "Ranged light tracer",
            attackTypeLabel: "Ranged Light",
            sampleSeed: 23456,
            sampleAttackCount: 5,
            request: createRangedRequest(
                module.AttackType.RangedLight,
                module.DefenderKind.Player,
            ),
        },
        {
            name: "Ranged standard tracer",
            attackTypeLabel: "Ranged Standard",
            sampleSeed: 23457,
            sampleAttackCount: 5,
            request: createRangedRequest(
                module.AttackType.RangedStandard,
                module.DefenderKind.Player,
            ),
        },
        {
            name: "Ranged heavy tracer",
            attackTypeLabel: "Ranged Heavy",
            sampleSeed: 23458,
            sampleAttackCount: 5,
            request: createRangedRequest(
                module.AttackType.RangedHeavy,
                module.DefenderKind.Player,
            ),
        },
        {
            name: "Magic fixed-spell tracer",
            attackTypeLabel: "Magic",
            sampleSeed: 34567,
            sampleAttackCount: 5,
            request: createMagicRequest(
                module.AttackType.Magic,
                module.DefenderKind.Player,
            ),
        },
        {
            name: "NPC magic fixed-spell tracer",
            attackTypeLabel: "Magic",
            sampleSeed: 34568,
            sampleAttackCount: 5,
            request: createMagicRequest(
                module.AttackType.Magic,
                module.DefenderKind.Npc,
            ),
        },
    ];
}

export function calculateDpsScenarioResults(
    module: EngineModule,
    scenarios = createFixedDpsScenarios(module),
): DpsScenarioResult[] {
    const service = new module.DpsService();

    return scenarios.map((scenario) => ({
        scenario,
        result: service.CalculateExpected(scenario.request),
        sampledResult: service.SampleSingleAttackWithSeed(
            scenario.request,
            scenario.sampleSeed,
        ),
        aggregateResult: service.SampleAttacksWithSeed(
            scenario.request,
            scenario.sampleAttackCount,
            scenario.sampleSeed,
        ),
    }));
}

export function formatDpsNumber(value: number, fractionDigits = 3): string {
    return value.toFixed(fractionDigits);
}

export function formatDpsPercent(value: number): string {
    return `${formatDpsNumber(value * 100, 2)}%`;
}

export function getDefaultCombatStylePreset(
    attackType: CalculatorAttackType,
): CombatStylePreset {
    if (isRangedAttackType(attackType)) {
        return "ranged-accurate";
    }

    if (attackType === "magic") {
        return "magic-standard";
    }

    return "melee-accurate";
}

export function isMeleeAttackType(attackType: CalculatorAttackType): boolean {
    return (
        attackType === "stab" ||
        attackType === "slash" ||
        attackType === "crush"
    );
}

export function isRangedAttackType(attackType: CalculatorAttackType): boolean {
    return (
        attackType === "ranged-light" ||
        attackType === "ranged-standard" ||
        attackType === "ranged-heavy"
    );
}

function createAttackerBonusFields(
    setup: PlayerAttackSetup,
): Partial<EquipmentBonuses> {
    const common = {
        meleeStrength: sanitizeWholeNumber(setup.meleeStrength),
        rangedStrength: sanitizeWholeNumber(setup.rangedStrength),
        magicDamagePercent: sanitizeDecimalNumber(setup.magicDamagePercent, 0),
    };

    switch (setup.attackType) {
        case "stab":
            return {
                ...common,
                stabAttack: sanitizeWholeNumber(setup.stabAttack),
            };
        case "slash":
            return {
                ...common,
                slashAttack: sanitizeWholeNumber(setup.slashAttack),
            };
        case "crush":
            return {
                ...common,
                crushAttack: sanitizeWholeNumber(setup.crushAttack),
            };
        case "magic":
            return {
                ...common,
                magicAttack: sanitizeWholeNumber(setup.magicAttack),
            };
        case "ranged-light":
        case "ranged-standard":
        case "ranged-heavy":
            return {
                ...common,
                rangedAttack: sanitizeWholeNumber(setup.rangedAttack),
            };
    }
}

function createDefenderBonusFields(
    setup: NpcDefenceSetup,
): Partial<EquipmentBonuses> {
    return {
        stabDefence: sanitizeWholeNumber(setup.stabDefence),
        slashDefence: sanitizeWholeNumber(setup.slashDefence),
        crushDefence: sanitizeWholeNumber(setup.crushDefence),
        magicDefence: sanitizeWholeNumber(setup.magicDefence),
        rangedDefenceLight: sanitizeWholeNumber(setup.rangedDefenceLight),
        rangedDefenceStandard: sanitizeWholeNumber(
            setup.rangedDefenceStandard,
        ),
        rangedDefenceHeavy: sanitizeWholeNumber(setup.rangedDefenceHeavy),
    };
}

function createCombatStyleBonus(preset: CombatStylePreset): StyleBonus {
    switch (preset) {
        case "melee-accurate":
            return { ...createDefaultStyleBonus(), attack: 3 };
        case "melee-aggressive":
            return { ...createDefaultStyleBonus(), strength: 3 };
        case "melee-controlled":
            return {
                ...createDefaultStyleBonus(),
                attack: 1,
                strength: 1,
                defence: 1,
            };
        case "melee-defensive":
            return { ...createDefaultStyleBonus(), defence: 3 };
        case "ranged-accurate":
            return { ...createDefaultStyleBonus(), ranged: 3 };
        case "ranged-rapid":
            return createDefaultStyleBonus();
        case "ranged-longrange":
            return { ...createDefaultStyleBonus(), defence: 3 };
        case "magic-standard":
            return createDefaultStyleBonus();
        case "magic-defensive":
            return { ...createDefaultStyleBonus(), defence: 3 };
    }
}

function resolveModuleAttackType(
    module: EngineModule,
    attackType: CalculatorAttackType,
): AttackType {
    switch (attackType) {
        case "stab":
            return module.AttackType.Stab;
        case "slash":
            return module.AttackType.Slash;
        case "crush":
            return module.AttackType.Crush;
        case "magic":
            return module.AttackType.Magic;
        case "ranged-light":
            return module.AttackType.RangedLight;
        case "ranged-standard":
            return module.AttackType.RangedStandard;
        case "ranged-heavy":
            return module.AttackType.RangedHeavy;
    }
}

function sanitizeWholeNumber(value: CalculatorNumber, minimum?: number): number {
    const numericValue = Number(value);
    const wholeValue = Number.isFinite(numericValue)
        ? Math.trunc(numericValue)
        : (minimum ?? 0);

    if (minimum === undefined) {
        return wholeValue;
    }

    return Math.max(minimum, wholeValue);
}

function sanitizeDecimalNumber(value: CalculatorNumber, minimum?: number): number {
    const numericValue = Number(value);
    const decimalValue = Number.isFinite(numericValue)
        ? numericValue
        : (minimum ?? 0);

    if (minimum === undefined) {
        return decimalValue;
    }

    return Math.max(minimum, decimalValue);
}

function createMeleeSlashRequest(
    attackType: AttackType,
    defenderKind: DefenderKind,
): DpsRequest {
    return {
        attackerStats: {
            ...createDefaultCombatStats(),
            attack: 99,
            strength: 99,
        },
        defenderStats: {
            ...createDefaultCombatStats(),
            defence: 80,
        },
        attackerBonuses: {
            ...createDefaultEquipmentBonuses(),
            slashAttack: 132,
            meleeStrength: 118,
        },
        defenderBonuses: {
            ...createDefaultEquipmentBonuses(),
            slashDefence: 80,
        },
        attackerStyle: {
            ...createDefaultStyleBonus(),
            attack: 3,
            strength: 3,
        },
        defenderStyle: createDefaultStyleBonus(),
        attackType,
        defenderKind,
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
}

function createRangedRequest(
    attackType: AttackType,
    defenderKind: DefenderKind,
): DpsRequest {
    return {
        attackerStats: {
            ...createDefaultCombatStats(),
            ranged: 99,
        },
        defenderStats: {
            ...createDefaultCombatStats(),
            defence: 70,
        },
        attackerBonuses: {
            ...createDefaultEquipmentBonuses(),
            rangedAttack: 100,
            rangedStrength: 80,
        },
        defenderBonuses: {
            ...createDefaultEquipmentBonuses(),
            rangedDefenceLight: 20,
            rangedDefenceStandard: 40,
            rangedDefenceHeavy: 60,
        },
        attackerStyle: {
            ...createDefaultStyleBonus(),
            ranged: 3,
        },
        defenderStyle: createDefaultStyleBonus(),
        attackType,
        defenderKind,
        weaponSpeedTicks: 4,
        attackPrayerMultiplier: 1.10,
        strengthPrayerMultiplier: 1.05,
        defencePrayerMultiplier: 1.0,
        attackLevelMultiplier: 1.02,
        strengthLevelMultiplier: 1.03,
        defenceLevelMultiplier: 1.0,
        finalAttackRollMultiplier: 1.10,
        finalDefenceRollMultiplier: 1.0,
        finalDamageMultiplier: 1.15,
        magicBaseMaximumHit: 0,
    };
}

function createMagicRequest(
    attackType: AttackType,
    defenderKind: DefenderKind,
): DpsRequest {
    return {
        attackerStats: {
            ...createDefaultCombatStats(),
            strength: 99,
            magic: 90,
        },
        defenderStats: {
            ...createDefaultCombatStats(),
            defence: 65,
            magic: 66,
        },
        attackerBonuses: {
            ...createDefaultEquipmentBonuses(),
            magicAttack: 70,
            meleeStrength: 200,
            magicDamagePercent: 10,
        },
        defenderBonuses: {
            ...createDefaultEquipmentBonuses(),
            magicDefence: 40,
        },
        attackerStyle: createDefaultStyleBonus(),
        defenderStyle: createDefaultStyleBonus(),
        attackType,
        defenderKind,
        weaponSpeedTicks: 5,
        attackPrayerMultiplier: 1.05,
        strengthPrayerMultiplier: 1.0,
        defencePrayerMultiplier: 1.0,
        attackLevelMultiplier: 1.0,
        strengthLevelMultiplier: 1.0,
        defenceLevelMultiplier: 1.0,
        finalAttackRollMultiplier: 1.0,
        finalDefenceRollMultiplier: 1.0,
        finalDamageMultiplier: 1.20,
        magicBaseMaximumHit: 24,
    };
}

function createDefaultCombatStats(): CombatStats {
    return {
        attack: 1,
        strength: 1,
        defence: 1,
        ranged: 1,
        magic: 1,
        hitpoints: 10,
    };
}

function createDefaultEquipmentBonuses(): EquipmentBonuses {
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
    };
}

function createDefaultStyleBonus(): StyleBonus {
    return {
        attack: 0,
        strength: 0,
        defence: 0,
        ranged: 0,
        magic: 0,
    };
}
