import type {
    AttackType,
    CombatStats,
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
    return [
        {
            name: "Melee slash tracer",
            attackTypeLabel: "Slash",
            sampleSeed: 12345,
            sampleAttackCount: 5,
            request: createMeleeSlashRequest(module.AttackType.Slash),
        },
    ];
}

export function calculateDpsScenarioResults(
    module: EngineModule,
    scenarios = createFixedMeleeDpsScenarios(module),
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

function createMeleeSlashRequest(attackType: AttackType): DpsRequest {
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
