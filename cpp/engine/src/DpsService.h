#pragma once

#include "Types.h"

#include <random>

namespace osrssim
{

enum class AttackType
{
    Stab,
    Slash,
    Crush,
    Magic,
    RangedLight,
    RangedStandard,
    RangedHeavy
};

enum class DefenderKind
{
    Player,
    Npc
};

struct CombatStats
{
    int attack = 1;
    int strength = 1;
    int defence = 1;
    int ranged = 1;
    int magic = 1;
    int hitpoints = 10;
};

struct EquipmentBonuses
{
    int stabAttack = 0;
    int slashAttack = 0;
    int crushAttack = 0;
    int magicAttack = 0;
    int rangedAttack = 0;
    int stabDefence = 0;
    int slashDefence = 0;
    int crushDefence = 0;
    int magicDefence = 0;
    int rangedDefenceLight = 0;
    int rangedDefenceStandard = 0;
    int rangedDefenceHeavy = 0;
    int meleeStrength = 0;
    int rangedStrength = 0;
    double magicDamagePercent = 0.0;
};

struct StyleBonus
{
    int attack = 0;
    int strength = 0;
    int defence = 0;
    int ranged = 0;
    int magic = 0;
};

struct AttackComposition
{
    AttackType attackType = AttackType::Slash;
    CombatStats stats;
    EquipmentBonuses bonuses;
    WeaponDefinition weapon;
};

struct DefenceComposition
{
    CombatStats stats;
    EquipmentBonuses bonuses;
};

struct DpsRequest
{
    AttackComposition attackComposition;
    DefenceComposition defenceComposition;
    StyleBonus attackerStyle;
    StyleBonus defenderStyle;
    DefenderKind defenderKind = DefenderKind::Player;
    double attackPrayerMultiplier = 1.0;
    double strengthPrayerMultiplier = 1.0;
    double defencePrayerMultiplier = 1.0;
    double attackLevelMultiplier = 1.0;
    double strengthLevelMultiplier = 1.0;
    double defenceLevelMultiplier = 1.0;
    double finalAttackRollMultiplier = 1.0;
    double finalDefenceRollMultiplier = 1.0;
    double finalDamageMultiplier = 1.0;
    int magicBaseMaximumHit = 0;
};

struct DpsResult
{
    int attackRoll = 0;
    int defenceRoll = 0;
    int maximumHit = 0;
    double hitChance = 0.0;
    double expectedDamagePerAttack = 0.0;
    double secondsPerAttack = 0.0;
    double dps = 0.0;
};

struct DpsSampleResult
{
    int attackRoll = 0;
    int defenceRoll = 0;
    int maximumHit = 0;
    double hitChance = 0.0;
    double expectedDamagePerAttack = 0.0;
    double secondsPerAttack = 0.0;
    double dps = 0.0;
    bool accuracyPassed = false;
    int sampledDamage = 0;
};

struct DpsSampleAggregateResult
{
    int attackCount = 0;
    int totalSampledDamage = 0;
    double averageSampledDamagePerAttack = 0.0;
    double sampledDps = 0.0;
};

class DpsService
{
private:
    std::mt19937 m_RandomGenerator;

public:
    DpsService();

    DpsResult CalculateExpected(const DpsRequest& request) const;
    void SetSeed(unsigned int seed);
    DpsSampleResult SampleSingleAttack(const DpsRequest& request);
    DpsSampleResult SampleSingleAttack(
        const DpsRequest& request,
        unsigned int seed) const;
    DpsSampleAggregateResult SampleAttacks(
        const DpsRequest& request,
        int attackCount);
    DpsSampleAggregateResult SampleAttacks(
        const DpsRequest& request,
        int attackCount,
        unsigned int seed) const;

    static int CalculateEffectiveLevel(
        int level,
        double prayerMultiplier,
        double levelMultiplier,
        int styleBonus);
    static int CalculateAttackRoll(
        int effectiveAttackLevel,
        int offensiveEquipmentBonus,
        double finalAttackRollMultiplier);
    static int CalculateDefenceRoll(
        int effectiveDefenceLevel,
        int defensiveEquipmentBonus,
        double finalDefenceRollMultiplier);
    static int CalculateStandardMaximumHit(
        int effectiveStrengthLevel,
        int strengthEquipmentBonus,
        double finalDamageMultiplier);
    static double CalculateHitChance(int attackRoll, int defenceRoll);

private:
    static DpsSampleResult SampleSingleAttackWithGenerator(
        const DpsRequest& request,
        std::mt19937& generator,
        const DpsResult& expectedResult);
    static DpsSampleAggregateResult SampleAttacksWithGenerator(
        const DpsRequest& request,
        int attackCount,
        std::mt19937& generator,
        const DpsResult& expectedResult);
    static int SelectMeleeAttackBonus(
        AttackType attackType,
        const EquipmentBonuses& bonuses);
    static int SelectMeleeDefenceBonus(
        AttackType attackType,
        const EquipmentBonuses& bonuses);
    static int SelectAttackLevel(
        const DpsRequest& request);
    static int SelectAttackStyleBonus(
        const DpsRequest& request);
    static int SelectStrengthLevel(
        const DpsRequest& request);
    static int SelectStrengthStyleBonus(
        const DpsRequest& request);
    static int SelectAttackBonus(
        AttackType attackType,
        const EquipmentBonuses& bonuses);
    static int SelectDefenceBonus(
        AttackType attackType,
        const EquipmentBonuses& bonuses);
    static int SelectStrengthBonus(
        AttackType attackType,
        const EquipmentBonuses& bonuses);
    static int CalculateMaximumHit(
        const DpsRequest& request,
        int effectiveStrength);
    static int CalculateEffectiveDefenceLevel(const DpsRequest& request);
};

}  // namespace osrssim
