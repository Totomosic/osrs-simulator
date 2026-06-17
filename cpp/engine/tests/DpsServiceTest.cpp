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
}
