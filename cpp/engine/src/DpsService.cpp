#include "DpsService.h"

#include <algorithm>
#include <cmath>
#include <random>

namespace osrssim
{
namespace
{

constexpr double SecondsPerTick = 0.6;
constexpr int PlayerMagicDefenceMagicPercent = 70;
constexpr int PlayerMagicDefenceDefencePercent = 30;

}  // namespace

DpsService::DpsService() : m_RandomGenerator(0)
{
}

DpsResult DpsService::CalculateExpected(const DpsRequest& request) const
{
    DpsResult result;

    const int effectiveAttack = CalculateEffectiveLevel(
        SelectAttackLevel(request),
        request.attackPrayerMultiplier,
        request.attackLevelMultiplier,
        SelectAttackStyleBonus(request));
    const int effectiveDefence = CalculateEffectiveDefenceLevel(request);
    const int effectiveStrength = CalculateEffectiveLevel(
        SelectStrengthLevel(request),
        request.strengthPrayerMultiplier,
        request.strengthLevelMultiplier,
        SelectStrengthStyleBonus(request));

    result.attackRoll = CalculateAttackRoll(
        effectiveAttack,
        SelectAttackBonus(
            request.attackComposition.attackType,
            request.attackComposition.bonuses),
        request.finalAttackRollMultiplier);
    result.defenceRoll = CalculateDefenceRoll(
        effectiveDefence,
        SelectDefenceBonus(
            request.attackComposition.attackType,
            request.defenceComposition.bonuses),
        request.finalDefenceRollMultiplier);
    result.maximumHit = CalculateMaximumHit(request, effectiveStrength);
    result.hitChance =
        CalculateHitChance(result.attackRoll, result.defenceRoll);
    result.expectedDamagePerAttack =
        result.hitChance * (result.maximumHit / 2.0);
    result.secondsPerAttack =
        request.attackComposition.weapon.speed * SecondsPerTick;

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

DpsSampleAggregateResult DpsService::SampleAttacks(
    const DpsRequest& request,
    int attackCount)
{
    return SampleAttacksWithGenerator(
        request,
        attackCount,
        m_RandomGenerator,
        CalculateExpected(request));
}

DpsSampleAggregateResult DpsService::SampleAttacks(
    const DpsRequest& request,
    int attackCount,
    unsigned int seed) const
{
    std::mt19937 generator(seed);

    return SampleAttacksWithGenerator(
        request,
        attackCount,
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
        case AttackType::Magic:
        case AttackType::RangedLight:
        case AttackType::RangedStandard:
        case AttackType::RangedHeavy:
            break;
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
        case AttackType::Magic:
        case AttackType::RangedLight:
        case AttackType::RangedStandard:
        case AttackType::RangedHeavy:
            break;
    }

    return bonuses.slashDefence;
}

int DpsService::SelectAttackLevel(const DpsRequest& request)
{
    switch (request.attackComposition.attackType)
    {
        case AttackType::Stab:
        case AttackType::Slash:
        case AttackType::Crush:
            return request.attackComposition.stats.attack;
        case AttackType::Magic:
            return request.attackComposition.stats.magic;
        case AttackType::RangedLight:
        case AttackType::RangedStandard:
        case AttackType::RangedHeavy:
            return request.attackComposition.stats.ranged;
    }

    return request.attackComposition.stats.attack;
}

int DpsService::SelectAttackStyleBonus(const DpsRequest& request)
{
    switch (request.attackComposition.attackType)
    {
        case AttackType::Stab:
        case AttackType::Slash:
        case AttackType::Crush:
            return request.attackerStyle.attack;
        case AttackType::Magic:
            return request.attackerStyle.magic;
        case AttackType::RangedLight:
        case AttackType::RangedStandard:
        case AttackType::RangedHeavy:
            return request.attackerStyle.ranged;
    }

    return request.attackerStyle.attack;
}

int DpsService::SelectStrengthLevel(const DpsRequest& request)
{
    switch (request.attackComposition.attackType)
    {
        case AttackType::Stab:
        case AttackType::Slash:
        case AttackType::Crush:
            return request.attackComposition.stats.strength;
        case AttackType::Magic:
            return 1;
        case AttackType::RangedLight:
        case AttackType::RangedStandard:
        case AttackType::RangedHeavy:
            return request.attackComposition.stats.ranged;
    }

    return request.attackComposition.stats.strength;
}

int DpsService::SelectStrengthStyleBonus(const DpsRequest& request)
{
    switch (request.attackComposition.attackType)
    {
        case AttackType::Stab:
        case AttackType::Slash:
        case AttackType::Crush:
            return request.attackerStyle.strength;
        case AttackType::Magic:
            return 0;
        case AttackType::RangedLight:
        case AttackType::RangedStandard:
        case AttackType::RangedHeavy:
            return request.attackerStyle.ranged;
    }

    return request.attackerStyle.strength;
}

int DpsService::SelectAttackBonus(
    AttackType attackType,
    const EquipmentBonuses& bonuses)
{
    switch (attackType)
    {
        case AttackType::Magic:
            return bonuses.magicAttack;
        case AttackType::RangedLight:
        case AttackType::RangedStandard:
        case AttackType::RangedHeavy:
            return bonuses.rangedAttack;
        case AttackType::Stab:
        case AttackType::Slash:
        case AttackType::Crush:
            return SelectMeleeAttackBonus(attackType, bonuses);
    }

    return bonuses.slashAttack;
}

int DpsService::SelectDefenceBonus(
    AttackType attackType,
    const EquipmentBonuses& bonuses)
{
    switch (attackType)
    {
        case AttackType::Magic:
            return bonuses.magicDefence;
        case AttackType::RangedLight:
            return bonuses.rangedDefenceLight;
        case AttackType::RangedStandard:
            return bonuses.rangedDefenceStandard;
        case AttackType::RangedHeavy:
            return bonuses.rangedDefenceHeavy;
        case AttackType::Stab:
        case AttackType::Slash:
        case AttackType::Crush:
            return SelectMeleeDefenceBonus(attackType, bonuses);
    }

    return bonuses.slashDefence;
}

int DpsService::SelectStrengthBonus(
    AttackType attackType,
    const EquipmentBonuses& bonuses)
{
    switch (attackType)
    {
        case AttackType::Stab:
        case AttackType::Slash:
        case AttackType::Crush:
            return bonuses.meleeStrength;
        case AttackType::RangedLight:
        case AttackType::RangedStandard:
        case AttackType::RangedHeavy:
            return bonuses.rangedStrength;
        case AttackType::Magic:
            return 0;
    }

    return bonuses.meleeStrength;
}

int DpsService::CalculateMaximumHit(
    const DpsRequest& request,
    int effectiveStrength)
{
    if (request.attackComposition.attackType == AttackType::Magic)
    {
        const double magicDamageMultiplier =
            1.0 + request.attackComposition.bonuses.magicDamagePercent / 100.0;

        return static_cast<int>(std::floor(
            request.magicBaseMaximumHit *
            magicDamageMultiplier *
            request.finalDamageMultiplier));
    }

    return CalculateStandardMaximumHit(
        effectiveStrength,
        SelectStrengthBonus(
            request.attackComposition.attackType,
            request.attackComposition.bonuses),
        request.finalDamageMultiplier);
}

int DpsService::CalculateEffectiveDefenceLevel(const DpsRequest& request)
{
    if (request.defenderKind == DefenderKind::Npc)
    {
        if (request.attackComposition.attackType == AttackType::Magic)
        {
            return request.defenceComposition.stats.magic + 9;
        }

        return request.defenceComposition.stats.defence + 9;
    }

    if (request.attackComposition.attackType == AttackType::Magic)
    {
        return (request.defenceComposition.stats.magic *
                PlayerMagicDefenceMagicPercent / 100) +
            static_cast<int>(std::floor(
                request.defenceComposition.stats.defence *
                request.defencePrayerMultiplier *
                request.defenceLevelMultiplier *
                PlayerMagicDefenceDefencePercent / 100.0)) +
            request.defenderStyle.defence + 8;
    }

    return CalculateEffectiveLevel(
        request.defenceComposition.stats.defence,
        request.defencePrayerMultiplier,
        request.defenceLevelMultiplier,
        request.defenderStyle.defence);
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

DpsSampleAggregateResult DpsService::SampleAttacksWithGenerator(
    const DpsRequest& request,
    int attackCount,
    std::mt19937& generator,
    const DpsResult& expectedResult)
{
    DpsSampleAggregateResult result;

    if (attackCount <= 0)
    {
        return result;
    }

    result.attackCount = attackCount;

    for (int attackIndex = 0; attackIndex < attackCount; ++attackIndex)
    {
        const DpsSampleResult sample = SampleSingleAttackWithGenerator(
            request,
            generator,
            expectedResult);

        result.totalSampledDamage += sample.sampledDamage;
    }

    result.averageSampledDamagePerAttack =
        static_cast<double>(result.totalSampledDamage) / attackCount;

    if (expectedResult.secondsPerAttack > 0.0)
    {
        result.sampledDps =
            result.averageSampledDamagePerAttack /
            expectedResult.secondsPerAttack;
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
