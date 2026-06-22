#include "CombatService.h"

#include "LineOfSight.h"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace osrssim
{

CombatService::CombatService()
{
    RegisterAttackCallbackName(
        "standard_attack",
        [this](
            World& world,
            ActorId attackerId,
            ActorId targetId,
            Tick,
            const WeaponDefinition& attackWeapon)
        {
            return DefaultStandardAttack(
                world,
                attackerId,
                targetId,
                attackWeapon);
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

void CombatService::SetDpsSeed(unsigned int seed)
{
    m_DpsService.SetSeed(seed);
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
    const CombatComposition* combatComposition =
        world.GetActorCombatComposition(attackerId);
    const CombatComposition* targetCombatComposition =
        world.GetActorCombatComposition(targetId);

    if (attacker == nullptr || target == nullptr ||
        attackerMembership == nullptr || targetMembership == nullptr ||
        combatComposition == nullptr || targetCombatComposition == nullptr ||
        targetCombatComposition->stats.hitpoints <= 0 ||
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
        combatComposition->weapon.range);
}

bool CombatService::DispatchAttack(
    World& world,
    ActorId attackerId,
    ActorId targetId,
    Tick currentTick) const
{
    const CombatComposition* combatComposition =
        world.GetActorCombatComposition(attackerId);

    if (combatComposition == nullptr || world.GetActorCore(targetId) == nullptr)
    {
        return false;
    }

    const WeaponDefinition attackWeapon = combatComposition->weapon;

    const auto weaponCallback =
        m_WeaponAttackCallbacks.find(attackWeapon.id);
    bool attackSucceeded = false;

    if (weaponCallback != m_WeaponAttackCallbacks.end())
    {
        attackSucceeded = weaponCallback->second(
            world,
            attackerId,
            targetId,
            currentTick,
            attackWeapon);
    }
    else if (m_GenericAttackCallback)
    {
        attackSucceeded = m_GenericAttackCallback(
            world,
            attackerId,
            targetId,
            currentTick,
            attackWeapon);
    }
    else
    {
        attackSucceeded = DefaultStandardAttack(
            world,
            attackerId,
            targetId,
            attackWeapon);
    }

    if (attackSucceeded)
    {
        world.SetActorAttackTimer(attackerId, attackWeapon.speed);
    }

    return attackSucceeded;
}

bool CombatService::DefaultStandardAttack(
    World& world,
    ActorId attackerId,
    ActorId targetId,
    const WeaponDefinition& attackWeapon) const
{
    CombatQueue* targetQueue = world.GetActorCombatQueue(targetId);

    if (targetQueue == nullptr)
    {
        return false;
    }

    const int applyDamageDelay = CalculateApplyDamageDelay(
        world,
        attackerId,
        targetId);

    if (applyDamageDelay <= 0)
    {
        return false;
    }

    const DpsRequest request = BuildStandardDpsRequest(
        world,
        attackerId,
        targetId,
        attackWeapon);
    const DpsSampleResult damageRoll = m_DpsService.SampleSingleAttack(request);
    const ScenePosition source = world.GetActorFootprintCenter(attackerId);
    const ScenePosition targetCenter = world.GetActorFootprintCenter(targetId);
    const ProjectileMetadata projectile{
        attackWeapon.projectileId,
        source,
        targetId,
        targetCenter,
        applyDamageDelay};

    if (attackWeapon.projectileId > 0)
    {
        return targetQueue->AddEvent(
            applyDamageDelay,
            [&world, targetId, damage = damageRoll.sampledDamage]()
            {
                CombatService::ApplyDamage(world, targetId, damage);
            },
            projectile);
    }

    return targetQueue->AddEvent(
        applyDamageDelay,
        [&world, targetId, damage = damageRoll.sampledDamage]()
        {
            CombatService::ApplyDamage(world, targetId, damage);
        });
}

DpsRequest CombatService::BuildStandardDpsRequest(
    const World& world,
    ActorId attackerId,
    ActorId targetId,
    const WeaponDefinition& attackWeapon) const
{
    const CombatComposition* attackerComposition =
        world.GetActorCombatComposition(attackerId);
    const CombatComposition* defenderComposition =
        world.GetActorCombatComposition(targetId);

    if (attackerComposition == nullptr || defenderComposition == nullptr)
    {
        return DpsRequest{};
    }

    DpsRequest request;
    request.attackComposition.attackType = attackerComposition->attackType;
    request.attackComposition.stats = attackerComposition->stats;
    request.attackComposition.bonuses = attackerComposition->bonuses;
    request.attackComposition.weapon = attackWeapon;
    request.defenceComposition.stats = defenderComposition->stats;
    request.defenceComposition.bonuses = defenderComposition->bonuses;
    request.defenderKind = world.GetNpc(targetId) == nullptr
        ? DefenderKind::Player
        : DefenderKind::Npc;
    request.magicBaseMaximumHit = attackerComposition->magicBaseMaximumHit;

    return request;
}

int CombatService::CalculateApplyDamageDelay(
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

    if (attacker == nullptr || target == nullptr ||
        attackerMembership == nullptr || targetMembership == nullptr ||
        attackerMembership->sceneId != targetMembership->sceneId ||
        attackerMembership->coordinate.plane !=
            targetMembership->coordinate.plane)
    {
        return 0;
    }

    const int attackerMaxX =
        attackerMembership->coordinate.x + attacker->size - 1;
    const int attackerMaxY =
        attackerMembership->coordinate.y + attacker->size - 1;
    const int targetMaxX = targetMembership->coordinate.x + target->size - 1;
    const int targetMaxY = targetMembership->coordinate.y + target->size - 1;

    const int xDistance = std::max(
        0,
        std::max(
            targetMembership->coordinate.x - attackerMaxX,
            attackerMembership->coordinate.x - targetMaxX));
    const int yDistance = std::max(
        0,
        std::max(
            targetMembership->coordinate.y - attackerMaxY,
            attackerMembership->coordinate.y - targetMaxY));
    const int tileDistance = std::max(xDistance, yDistance);

    return 1 + (3 + tileDistance) / 6;
}

void CombatService::ApplyDamage(
    World& world,
    ActorId targetId,
    int damage)
{
    const CombatComposition* currentComposition =
        world.GetActorCombatComposition(targetId);

    if (currentComposition == nullptr)
    {
        return;
    }

    CombatComposition updatedComposition = *currentComposition;
    const int currentHitpoints = std::max(
        0,
        updatedComposition.stats.hitpoints);
    const int positiveDamage = std::max(0, damage);
    updatedComposition.stats.hitpoints -= std::min(
        currentHitpoints,
        positiveDamage);
    world.SetActorCombatComposition(targetId, updatedComposition);

    if (currentHitpoints > 0 && updatedComposition.stats.hitpoints <= 0)
    {
        QueueDeath(world, targetId);
    }
}

void CombatService::QueueDeath(
    World& world,
    ActorId targetId)
{
    CombatQueue* targetQueue = world.GetActorCombatQueue(targetId);

    if (targetQueue == nullptr)
    {
        return;
    }

    targetQueue->AddEvent(
        0,
        [&world, targetId]()
        {
            world.QueueActorRemoval(targetId);
        });
}

}  // namespace osrssim
