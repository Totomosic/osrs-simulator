#include "CombatService.h"

#include <utility>

namespace osrssim
{

void CombatService::RegisterGenericAttackCallback(AttackCallback callback)
{
    m_GenericAttackCallback = std::move(callback);
}

void CombatService::RegisterWeaponAttackCallback(
    WeaponId weaponId,
    AttackCallback callback)
{
    m_WeaponAttackCallbacks[weaponId] = std::move(callback);
}

void CombatService::DecrementAttackTimers(World& world) const
{
    for (const auto& [actorId, player] : world.GetPlayers())
    {
        world.SetActorAttackTimer(actorId, player.actor.attackTimer - 1);
    }

    for (const auto& [actorId, npc] : world.GetNpcs())
    {
        world.SetActorAttackTimer(actorId, npc.actor.attackTimer - 1);
    }
}

bool CombatService::DispatchAttack(
    World& world,
    ActorId attackerId,
    ActorId targetId,
    Tick currentTick) const
{
    const WeaponDefinition* weapon =
        world.GetActorWeaponDefinition(attackerId);

    if (weapon == nullptr || world.GetActorCore(targetId) == nullptr)
    {
        return false;
    }

    const WeaponDefinition attackWeapon = *weapon;

    world.SetActorAttackTimer(attackerId, attackWeapon.speed);

    const auto weaponCallback =
        m_WeaponAttackCallbacks.find(attackWeapon.id);

    if (weaponCallback != m_WeaponAttackCallbacks.end())
    {
        weaponCallback->second(
            world,
            attackerId,
            targetId,
            currentTick,
            attackWeapon);
        return true;
    }

    if (m_GenericAttackCallback)
    {
        m_GenericAttackCallback(
            world,
            attackerId,
            targetId,
            currentTick,
            attackWeapon);
    }

    return true;
}

}  // namespace osrssim
