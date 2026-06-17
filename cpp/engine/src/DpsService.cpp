#include "DpsService.h"

#include <algorithm>
#include <cmath>
#include <random>

namespace osrssim
{
namespace
{

constexpr double SecondsPerTick = 0.6;

}  // namespace

DpsService::DpsService() : m_RandomGenerator(0)
{
}

DpsResult DpsService::CalculateExpected(const DpsRequest& request) const
{
    DpsResult result;

    const int effectiveAttack = CalculateEffectiveLevel(
        request.attackerStats.attack,
        request.attackPrayerMultiplier,
        request.attackLevelMultiplier,
        request.attackerStyle.attack);
    const int effectiveDefence = CalculateEffectiveLevel(
        request.defenderStats.defence,
        request.defencePrayerMultiplier,
        request.defenceLevelMultiplier,
        request.defenderStyle.defence);
    const int effectiveStrength = CalculateEffectiveLevel(
        request.attackerStats.strength,
        request.strengthPrayerMultiplier,
        request.strengthLevelMultiplier,
        request.attackerStyle.strength);

    result.attackRoll = CalculateAttackRoll(
        effectiveAttack,
        SelectMeleeAttackBonus(request.attackType, request.attackerBonuses),
        request.finalAttackRollMultiplier);
    result.defenceRoll = CalculateDefenceRoll(
        effectiveDefence,
        SelectMeleeDefenceBonus(request.attackType, request.defenderBonuses),
        request.finalDefenceRollMultiplier);
    result.maximumHit = CalculateStandardMaximumHit(
        effectiveStrength,
        request.attackerBonuses.meleeStrength,
        request.finalDamageMultiplier);
    result.hitChance =
        CalculateHitChance(result.attackRoll, result.defenceRoll);
    result.expectedDamagePerAttack =
        result.hitChance * (result.maximumHit / 2.0);
    result.secondsPerAttack = request.weaponSpeedTicks * SecondsPerTick;

    if (result.secondsPerAttack > 0.0)
    {
        result.dps =
            result.expectedDamagePerAttack / result.secondsPerAttack;
    }

    return result;
}

void DpsService::SetSeed(unsigned int seed)
{
    m_RandomGenerator.seed(seed);
}

DpsSampleResult DpsService::SampleSingleAttack(const DpsRequest& request)
{
    return SampleSingleAttackWithGenerator(
        request,
        m_RandomGenerator,
        CalculateExpected(request));
}

DpsSampleResult DpsService::SampleSingleAttack(
    const DpsRequest& request,
    unsigned int seed) const
{
    std::mt19937 generator(seed);

    return SampleSingleAttackWithGenerator(
        request,
        generator,
        CalculateExpected(request));
}

int DpsService::CalculateEffectiveLevel(
    int level,
    double prayerMultiplier,
    double levelMultiplier,
    int styleBonus)
{
    return static_cast<int>(
        std::floor(level * prayerMultiplier * levelMultiplier)) +
        styleBonus + 8;
}

int DpsService::CalculateAttackRoll(
    int effectiveAttackLevel,
    int offensiveEquipmentBonus,
    double finalAttackRollMultiplier)
{
    const int baseAttackRoll =
        effectiveAttackLevel * (offensiveEquipmentBonus + 64);

    return static_cast<int>(
        std::floor(baseAttackRoll * finalAttackRollMultiplier));
}

int DpsService::CalculateDefenceRoll(
    int effectiveDefenceLevel,
    int defensiveEquipmentBonus,
    double finalDefenceRollMultiplier)
{
    const int baseDefenceRoll =
        effectiveDefenceLevel * (defensiveEquipmentBonus + 64);

    return static_cast<int>(
        std::floor(baseDefenceRoll * finalDefenceRollMultiplier));
}

int DpsService::CalculateStandardMaximumHit(
    int effectiveStrengthLevel,
    int strengthEquipmentBonus,
    double finalDamageMultiplier)
{
    const int baseMaximumHit = static_cast<int>(std::floor(
        0.5 +
        effectiveStrengthLevel * (strengthEquipmentBonus + 64) / 640.0));

    return static_cast<int>(
        std::floor(baseMaximumHit * finalDamageMultiplier));
}

int DpsService::SelectMeleeAttackBonus(
    AttackType attackType,
    const EquipmentBonuses& bonuses)
{
    switch (attackType)
    {
        case AttackType::Stab:
            return bonuses.stabAttack;
        case AttackType::Slash:
            return bonuses.slashAttack;
        case AttackType::Crush:
            return bonuses.crushAttack;
    }

    return bonuses.slashAttack;
}

int DpsService::SelectMeleeDefenceBonus(
    AttackType attackType,
    const EquipmentBonuses& bonuses)
{
    switch (attackType)
    {
        case AttackType::Stab:
            return bonuses.stabDefence;
        case AttackType::Slash:
            return bonuses.slashDefence;
        case AttackType::Crush:
            return bonuses.crushDefence;
    }

    return bonuses.slashDefence;
}

DpsSampleResult DpsService::SampleSingleAttackWithGenerator(
    const DpsRequest& request,
    std::mt19937& generator,
    const DpsResult& expectedResult)
{
    DpsSampleResult result;

    result.attackRoll = expectedResult.attackRoll;
    result.defenceRoll = expectedResult.defenceRoll;
    result.maximumHit = expectedResult.maximumHit;
    result.hitChance = expectedResult.hitChance;
    result.expectedDamagePerAttack = expectedResult.expectedDamagePerAttack;
    result.secondsPerAttack = expectedResult.secondsPerAttack;
    result.dps = expectedResult.dps;

    std::uniform_int_distribution<int> attackRollDistribution(
        0,
        result.attackRoll);
    std::uniform_int_distribution<int> defenceRollDistribution(
        0,
        result.defenceRoll);

    result.accuracyPassed =
        attackRollDistribution(generator) > defenceRollDistribution(generator);

    if (result.accuracyPassed)
    {
        std::uniform_int_distribution<int> damageDistribution(
            0,
            result.maximumHit);
        result.sampledDamage = damageDistribution(generator);
    }

    return result;
}

double DpsService::CalculateHitChance(int attackRoll, int defenceRoll)
{
    if (attackRoll > defenceRoll)
    {
        return std::clamp(
            1.0 -
                (defenceRoll + 2.0) /
                    (2.0 * (attackRoll + 1.0)),
            0.0,
            1.0);
    }

    return std::clamp(
        attackRoll / (2.0 * (defenceRoll + 1.0)),
        0.0,
        1.0);
}

}  // namespace osrssim
