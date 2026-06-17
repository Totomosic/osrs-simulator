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
}
