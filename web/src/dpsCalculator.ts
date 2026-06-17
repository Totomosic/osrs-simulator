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
