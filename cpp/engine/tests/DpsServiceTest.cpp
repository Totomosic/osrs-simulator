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

    osrssim::DpsRequest compositionRequest;

    compositionRequest.attackComposition.attackType =
        osrssim::AttackType::Slash;
    compositionRequest.attackComposition.stats.attack = 99;
    compositionRequest.attackComposition.stats.strength = 99;
    compositionRequest.attackComposition.bonuses.slashAttack = 132;
    compositionRequest.attackComposition.bonuses.meleeStrength = 118;
    compositionRequest.attackComposition.weapon = {42, 1, 4};
    compositionRequest.attackerStyle.attack = 3;
    compositionRequest.attackerStyle.strength = 3;
    compositionRequest.defenceComposition.stats.defence = 80;
    compositionRequest.defenceComposition.bonuses.slashDefence = 80;

    const osrssim::DpsResult compositionResult =
        service.CalculateExpected(compositionRequest);

    assert(compositionResult.attackRoll == 21560);
    assert(compositionResult.defenceRoll == 12672);
    assert(compositionResult.maximumHit == 31);
    assert(NearlyEqual(compositionResult.hitChance, 0.7060896999211539));
    assert(NearlyEqual(
        compositionResult.expectedDamagePerAttack,
        10.944390348778885));
    assert(NearlyEqual(compositionResult.secondsPerAttack, 2.4));
    assert(NearlyEqual(compositionResult.dps, 4.560162645324535));

    osrssim::DpsRequest playerDefenderRequest;

    playerDefenderRequest.defenderKind = osrssim::DefenderKind::Player;
    playerDefenderRequest.attackComposition.attackType =
        osrssim::AttackType::Slash;
    playerDefenderRequest.attackComposition.stats.attack = 99;
    playerDefenderRequest.attackComposition.stats.strength = 99;
    playerDefenderRequest.attackComposition.bonuses.slashAttack = 132;
    playerDefenderRequest.attackComposition.bonuses.meleeStrength = 118;
    playerDefenderRequest.attackerStyle.attack = 3;
    playerDefenderRequest.attackerStyle.strength = 3;
    playerDefenderRequest.defenceComposition.stats.defence = 80;
    playerDefenderRequest.defenceComposition.bonuses.slashDefence = 80;
    playerDefenderRequest.defenderStyle.defence = 3;
    playerDefenderRequest.defencePrayerMultiplier = 1.15;
    playerDefenderRequest.defenceLevelMultiplier = 1.10;

    const osrssim::DpsResult playerDefenderResult =
        service.CalculateExpected(playerDefenderRequest);

    osrssim::DpsRequest npcDefenderRequest = playerDefenderRequest;
    npcDefenderRequest.defenderKind = osrssim::DefenderKind::Npc;

    const osrssim::DpsResult npcDefenderResult =
        service.CalculateExpected(npcDefenderRequest);

    assert(playerDefenderResult.defenceRoll == 16128);
    assert(npcDefenderResult.defenceRoll == 12816);
    assert(playerDefenderResult.hitChance < npcDefenderResult.hitChance);

    osrssim::DpsRequest playerMagicDefenderRequest = playerDefenderRequest;

    playerMagicDefenderRequest.defenderKind = osrssim::DefenderKind::Player;
    playerMagicDefenderRequest.attackComposition.attackType =
        osrssim::AttackType::Magic;
    playerMagicDefenderRequest.attackComposition.stats.magic = 90;
    playerMagicDefenderRequest.attackComposition.bonuses.magicAttack = 70;
    playerMagicDefenderRequest
        .attackComposition
        .bonuses
        .magicDamagePercent = 10.0;
    playerMagicDefenderRequest.defenceComposition.stats.defence = 70;
    playerMagicDefenderRequest.defenceComposition.stats.magic = 90;
    playerMagicDefenderRequest.defenceComposition.bonuses.magicDefence = 40;
    playerMagicDefenderRequest.magicBaseMaximumHit = 24;

    const osrssim::DpsResult playerMagicDefenderResult =
        service.CalculateExpected(playerMagicDefenderRequest);

    osrssim::DpsRequest npcMagicDefenderRequest = playerMagicDefenderRequest;
    npcMagicDefenderRequest.defenderKind = osrssim::DefenderKind::Npc;

    const osrssim::DpsResult npcMagicDefenderResult =
        service.CalculateExpected(npcMagicDefenderRequest);

    assert(playerMagicDefenderResult.defenceRoll == 10400);
    assert(npcMagicDefenderResult.defenceRoll == 10296);
    assert(
        playerMagicDefenderResult.hitChance <
        npcMagicDefenderResult.hitChance);

    osrssim::DpsRequest request;

    request.attackComposition.attackType = osrssim::AttackType::Slash;
    request.attackComposition.stats.attack = 99;
    request.attackComposition.stats.strength = 99;
    request.attackComposition.bonuses.slashAttack = 132;
    request.attackComposition.bonuses.meleeStrength = 118;
    request.attackerStyle.attack = 3;
    request.attackerStyle.strength = 3;
    request.defenceComposition.stats.defence = 80;
    request.defenceComposition.bonuses.slashDefence = 80;
    request.attackComposition.weapon.speed = 4;

    const osrssim::DpsResult result = service.CalculateExpected(request);

    assert(result.attackRoll == 21560);
    assert(result.defenceRoll == 12672);
    assert(result.maximumHit == 31);
    assert(NearlyEqual(result.hitChance, 0.7060896999211539));
    assert(NearlyEqual(result.expectedDamagePerAttack, 10.944390348778885));
    assert(NearlyEqual(result.secondsPerAttack, 2.4));
    assert(NearlyEqual(result.dps, 4.560162645324535));

    osrssim::DpsRequest rangedRequest;

    rangedRequest.attackComposition.attackType =
        osrssim::AttackType::RangedHeavy;
    rangedRequest.attackComposition.stats.ranged = 99;
    rangedRequest.attackComposition.bonuses.rangedAttack = 100;
    rangedRequest.attackComposition.bonuses.rangedStrength = 80;
    rangedRequest.attackerStyle.ranged = 3;
    rangedRequest.defenceComposition.stats.defence = 70;
    rangedRequest.defenceComposition.bonuses.rangedDefenceLight = 20;
    rangedRequest.defenceComposition.bonuses.rangedDefenceStandard = 40;
    rangedRequest.defenceComposition.bonuses.rangedDefenceHeavy = 60;
    rangedRequest.attackComposition.weapon.speed = 4;
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

    rangedRequest.attackComposition.attackType =
        osrssim::AttackType::RangedLight;
    const osrssim::DpsResult rangedLightResult =
        service.CalculateExpected(rangedRequest);

    rangedRequest.attackComposition.attackType =
        osrssim::AttackType::RangedStandard;
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

    magicRequest.attackComposition.attackType = osrssim::AttackType::Magic;
    magicRequest.attackComposition.stats.magic = 90;
    magicRequest.attackComposition.stats.strength = 99;
    magicRequest.attackComposition.bonuses.magicAttack = 70;
    magicRequest.attackComposition.bonuses.meleeStrength = 200;
    magicRequest.attackComposition.bonuses.magicDamagePercent = 10.0;
    magicRequest.defenceComposition.stats.defence = 65;
    magicRequest.defenceComposition.stats.magic = 66;
    magicRequest.defenceComposition.bonuses.magicDefence = 40;
    magicRequest.attackComposition.weapon.speed = 5;
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
