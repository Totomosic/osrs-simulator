#include "DpsService.h"

#include <cassert>
#include <cmath>

namespace
{

bool NearlyEqual(double left, double right)
{
    return std::abs(left - right) < 0.000001;
}

}  // namespace

int main()
{
    osrssim::DpsService service;

    assert(
        osrssim::DpsService::CalculateEffectiveLevel(99, 1.15, 1.10, 3) ==
        136);
    assert(
        osrssim::DpsService::CalculateAttackRoll(136, 132, 1.15) ==
        30654);
    assert(
        osrssim::DpsService::CalculateDefenceRoll(97, 80, 0.90) ==
        12571);
    assert(
        osrssim::DpsService::CalculateStandardMaximumHit(136, 118, 1.10) ==
        42);
    assert(NearlyEqual(
        osrssim::DpsService::CalculateHitChance(30654, 12571),
        0.7949274180394715));
    assert(NearlyEqual(
        osrssim::DpsService::CalculateHitChance(10000, 12571),
        0.39770919503658925));

    osrssim::DpsRequest request;

    request.attackType = osrssim::AttackType::Slash;
    request.attackerStats.attack = 99;
    request.attackerStats.strength = 99;
    request.attackerBonuses.slashAttack = 132;
    request.attackerBonuses.meleeStrength = 118;
    request.attackerStyle.attack = 3;
    request.attackerStyle.strength = 3;
    request.defenderStats.defence = 80;
    request.defenderBonuses.slashDefence = 80;
    request.weaponSpeedTicks = 4;

    const osrssim::DpsResult result = service.CalculateExpected(request);

    assert(result.attackRoll == 21560);
    assert(result.defenceRoll == 12672);
    assert(result.maximumHit == 31);
    assert(NearlyEqual(result.hitChance, 0.7060896999211539));
    assert(NearlyEqual(result.expectedDamagePerAttack, 10.944390348778885));
    assert(NearlyEqual(result.secondsPerAttack, 2.4));
    assert(NearlyEqual(result.dps, 4.560162645324535));

    osrssim::DpsRequest rangedRequest;

    rangedRequest.attackType = osrssim::AttackType::RangedHeavy;
    rangedRequest.attackerStats.ranged = 99;
    rangedRequest.attackerBonuses.rangedAttack = 100;
    rangedRequest.attackerBonuses.rangedStrength = 80;
    rangedRequest.attackerStyle.ranged = 3;
    rangedRequest.defenderStats.defence = 70;
    rangedRequest.defenderBonuses.rangedDefenceLight = 20;
    rangedRequest.defenderBonuses.rangedDefenceStandard = 40;
    rangedRequest.defenderBonuses.rangedDefenceHeavy = 60;
    rangedRequest.weaponSpeedTicks = 4;
    rangedRequest.attackPrayerMultiplier = 1.10;
    rangedRequest.strengthPrayerMultiplier = 1.05;
    rangedRequest.attackLevelMultiplier = 1.02;
    rangedRequest.strengthLevelMultiplier = 1.03;
    rangedRequest.finalAttackRollMultiplier = 1.10;
    rangedRequest.finalDamageMultiplier = 1.15;

    const osrssim::DpsResult rangedResult =
        service.CalculateExpected(rangedRequest);

    assert(rangedResult.attackRoll == 22008);
    assert(rangedResult.defenceRoll == 9672);
    assert(rangedResult.maximumHit == 31);
    assert(NearlyEqual(rangedResult.hitChance, 0.7802262710709256));
    assert(NearlyEqual(rangedResult.dps, 5.038961333999728));

    rangedRequest.attackType = osrssim::AttackType::RangedLight;
    const osrssim::DpsResult rangedLightResult =
        service.CalculateExpected(rangedRequest);

    rangedRequest.attackType = osrssim::AttackType::RangedStandard;
    const osrssim::DpsResult rangedStandardResult =
        service.CalculateExpected(rangedRequest);

    assert(rangedLightResult.attackRoll == 22008);
    assert(rangedLightResult.defenceRoll == 6552);
    assert(rangedLightResult.maximumHit == 31);
    assert(NearlyEqual(rangedLightResult.dps, 5.496728611022764));
    assert(rangedStandardResult.attackRoll == 22008);
    assert(rangedStandardResult.defenceRoll == 8112);
    assert(rangedStandardResult.maximumHit == 31);
    assert(NearlyEqual(rangedStandardResult.dps, 5.267844972511246));

    osrssim::DpsRequest magicRequest;

    magicRequest.attackType = osrssim::AttackType::Magic;
    magicRequest.attackerStats.magic = 90;
    magicRequest.attackerStats.strength = 99;
    magicRequest.attackerBonuses.magicAttack = 70;
    magicRequest.attackerBonuses.meleeStrength = 200;
    magicRequest.attackerBonuses.magicDamagePercent = 10.0;
    magicRequest.defenderStats.defence = 65;
    magicRequest.defenderBonuses.magicDefence = 40;
    magicRequest.weaponSpeedTicks = 5;
    magicRequest.magicBaseMaximumHit = 24;
    magicRequest.attackPrayerMultiplier = 1.05;
    magicRequest.finalDamageMultiplier = 1.20;

    const osrssim::DpsResult magicResult =
        service.CalculateExpected(magicRequest);

    assert(magicResult.attackRoll == 13668);
    assert(magicResult.defenceRoll == 7592);
    assert(magicResult.maximumHit == 31);
    assert(NearlyEqual(magicResult.hitChance, 0.7222181578754847));
    assert(NearlyEqual(magicResult.dps, 3.731460482356671));

    service.SetSeed(12345);
    const osrssim::DpsSampleResult firstSharedSample =
        service.SampleSingleAttack(request);
    const osrssim::DpsSampleResult secondSharedSample =
        service.SampleSingleAttack(request);

    service.SetSeed(12345);
    const osrssim::DpsSampleResult replayedSharedSample =
        service.SampleSingleAttack(request);
    const osrssim::DpsSampleResult seededArgumentSample =
        service.SampleSingleAttack(request, 12345);
    const osrssim::DpsSampleResult nextSharedSample =
        service.SampleSingleAttack(request);

    assert(firstSharedSample.attackRoll == result.attackRoll);
    assert(firstSharedSample.defenceRoll == result.defenceRoll);
    assert(firstSharedSample.maximumHit == result.maximumHit);
    assert(NearlyEqual(firstSharedSample.hitChance, result.hitChance));
    assert(firstSharedSample.accuracyPassed);
    assert(firstSharedSample.sampledDamage == 10);

    assert(secondSharedSample.accuracyPassed);
    assert(secondSharedSample.sampledDamage == 1);
    assert(replayedSharedSample.sampledDamage == firstSharedSample.sampledDamage);
    assert(seededArgumentSample.sampledDamage == firstSharedSample.sampledDamage);
    assert(nextSharedSample.sampledDamage == secondSharedSample.sampledDamage);

    service.SetSeed(12345);
    const osrssim::DpsSampleAggregateResult aggregateSample =
        service.SampleAttacks(request, 5);

    assert(aggregateSample.attackCount == 5);
    assert(aggregateSample.totalSampledDamage == 44);
    assert(NearlyEqual(aggregateSample.averageSampledDamagePerAttack, 8.8));
    assert(NearlyEqual(aggregateSample.sampledDps, 3.666666666666667));

    const osrssim::DpsSampleResult sampleAfterAggregate =
        service.SampleSingleAttack(request);
    assert(sampleAfterAggregate.sampledDamage == 23);

    service.SetSeed(12345);
    const osrssim::DpsSampleAggregateResult seededAggregateSample =
        service.SampleAttacks(request, 5, 12345);
    const osrssim::DpsSampleResult sampleAfterSeededAggregate =
        service.SampleSingleAttack(request);

    assert(seededAggregateSample.totalSampledDamage == 44);
    assert(sampleAfterSeededAggregate.sampledDamage == firstSharedSample.sampledDamage);
}
