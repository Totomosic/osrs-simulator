#include "CombatService.h"

#include "LineOfSight.h"

#include <stdexcept>
#include <utility>

namespace osrssim
{

CombatService::CombatService()
{
    RegisterAttackCallbackName(
        "standard_attack",
        [](
            World&,
            ActorId,
            ActorId,
            Tick,
            const WeaponDefinition&)
        {
        });
}

void CombatService::RegisterGenericAttackCallback(AttackCallback callback)
{
    m_GenericAttackCallback = std::move(callback);
}

void CombatService::RegisterAttackCallbackName(
    const std::string& callbackName,
    AttackCallback callback)
{
    if (callbackName.empty())
    {
        throw std::invalid_argument("attack callback name must not be empty");
    }

    m_AttackCallbacksByName[callbackName] = std::move(callback);
}

bool CombatService::HasAttackCallbackName(
    const std::string& callbackName) const
{
    return m_AttackCallbacksByName.contains(callbackName);
}

void CombatService::RegisterWeaponAttackCallback(
    WeaponId weaponId,
    AttackCallback callback)
{
    m_WeaponAttackCallbacks[weaponId] = std::move(callback);
}

void CombatService::BindWeaponAttackCallbackName(
    WeaponId weaponId,
    const std::string& callbackName)
{
    const auto callback = m_AttackCallbacksByName.find(callbackName);
    if (callback == m_AttackCallbacksByName.end())
    {
        throw std::invalid_argument("attack callback name is not registered");
    }

    m_WeaponAttackCallbacks[weaponId] = callback->second;
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

bool CombatService::CanAttackActorTarget(
    const World& world,
    ActorId attackerId,
    ActorId targetId) const
{
    const ActorCore* attacker = world.GetActorCore(attackerId);
    const ActorCore* target = world.GetActorCore(targetId);
    const SceneMembership* attackerMembership =
        world.GetSceneMembership(attackerId);
    const SceneMembership* targetMembership =
        world.GetSceneMembership(targetId);
    const WeaponDefinition* weapon =
        world.GetActorWeaponDefinition(attackerId);

    if (attacker == nullptr || target == nullptr ||
        attackerMembership == nullptr || targetMembership == nullptr ||
        weapon == nullptr ||
        attackerMembership->sceneId != targetMembership->sceneId ||
        attackerMembership->coordinate.plane !=
            targetMembership->coordinate.plane)
    {
        return false;
    }

    const Scene* scene = world.TryGetScene(attackerMembership->sceneId);

    if (scene == nullptr)
    {
        return false;
    }

    LineOfSight lineOfSight(*scene);
    return lineOfSight.HasLineOfSight(
        attackerMembership->coordinate,
        attacker->size,
        targetMembership->coordinate,
        target->size,
        weapon->range);
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
