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
    m_GenericAttackCallback = {
        std::move(callback),
        "anonymous_attack_callback"};
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
    m_WeaponAttackCallbacks[weaponId] = {
        std::move(callback),
        "anonymous_attack_callback"};
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

    m_WeaponAttackCallbacks[weaponId] = {
        callback->second,
        callbackName};
}

void CombatService::AddObserver(Observer& observer)
{
    if (std::find(m_Observers.begin(), m_Observers.end(), &observer) ==
        m_Observers.end())
    {
        m_Observers.push_back(&observer);
    }
}

void CombatService::RemoveObserver(Observer& observer)
{
    m_Observers.erase(
        std::remove(m_Observers.begin(), m_Observers.end(), &observer),
        m_Observers.end());
}

void CombatService::SetDpsSeed(unsigned int seed)
{
    m_DpsService.SetSeed(seed);
}

CombatService::AttackObservation CombatService::CreateAttackObservation(
    const World&,
    ActorId attackerId,
    ActorId targetId,
    Tick currentTick) const
{
    AttackObservation attack;
    attack.id = m_NextAttackId++;
    attack.tick = currentTick;
    attack.attackerId = attackerId;
    attack.targetId = targetId;
    attack.callback =
        m_CurrentAttackCallbackName.value_or("anonymous_attack_callback");
    return attack;
}

bool CombatService::QueueStructuredDamageEvent(
    World& world,
    AttackObservation& attack,
    ActorId targetId,
    int damage,
    int delayTicks) const
{
    const int queuedDamage = ClampDamageToCurrentHitpoints(
        world,
        targetId,
        damage);
    const std::uint64_t damageEventId = m_NextDamageEventId++;
    const QueuedDamageEventObservation queuedDamageEvent{
        damageEventId,
        attack.id,
        targetId,
        queuedDamage,
        delayTicks};
    const bool queued = world.QueueActorCombatEvent(
        targetId,
        delayTicks,
        [this, &world, targetId, damage = queuedDamage, damageEventId, attackId = attack.id]()
        {
            ApplyDamage(world, targetId, damage, damageEventId, attackId);
        });

    if (queued)
    {
        attack.queuedDamageEvents.push_back(queuedDamageEvent);
    }

    return queued;
}

bool CombatService::QueueStructuredDamageEvent(
    World& world,
    AttackObservation& attack,
    ActorId targetId,
    int damage,
    int delayTicks,
    ProjectileMetadata projectile) const
{
    const int queuedDamage = ClampDamageToCurrentHitpoints(
        world,
        targetId,
        damage);
    const std::uint64_t damageEventId = m_NextDamageEventId++;
    const QueuedDamageEventObservation queuedDamageEvent{
        damageEventId,
        attack.id,
        targetId,
        queuedDamage,
        delayTicks};
    const bool queued = world.QueueActorCombatEvent(
        targetId,
        delayTicks,
        [this, &world, targetId, damage = queuedDamage, damageEventId, attackId = attack.id]()
        {
            ApplyDamage(world, targetId, damage, damageEventId, attackId);
        },
        projectile);

    if (queued)
    {
        attack.queuedDamageEvents.push_back(queuedDamageEvent);
        attack.projectile = projectile;
    }

    return queued;
}

void CombatService::RecordAttackObservation(
    const AttackObservation& attack) const
{
    NotifyAttackQueued(attack);
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
        m_CurrentAttackCallbackName = weaponCallback->second.callbackName;
        m_CurrentDispatchRecordedAttack = false;
        attackSucceeded = weaponCallback->second.callback(
            world,
            attackerId,
            targetId,
            currentTick,
            attackWeapon);
    }
    else if (m_GenericAttackCallback.callback)
    {
        m_CurrentAttackCallbackName = m_GenericAttackCallback.callbackName;
        m_CurrentDispatchRecordedAttack = false;
        attackSucceeded = m_GenericAttackCallback.callback(
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

    if (attackSucceeded && m_CurrentAttackCallbackName.has_value() &&
        !m_CurrentDispatchRecordedAttack)
    {
        RecordAttackObservation(
            CreateAttackObservation(
                world,
                attackerId,
                targetId,
                currentTick));
    }

    m_CurrentAttackCallbackName.reset();
    m_CurrentDispatchRecordedAttack = false;

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
    const int queuedDamage = ClampDamageToCurrentHitpoints(
        world,
        targetId,
        damageRoll.sampledDamage);
    const std::uint64_t attackId = m_NextAttackId++;
    const std::uint64_t damageEventId = m_NextDamageEventId++;
    const ScenePosition source = world.GetActorFootprintCenter(attackerId);
    const ScenePosition targetCenter = world.GetActorFootprintCenter(targetId);
    const ProjectileMetadata projectile{
        attackWeapon.projectileId,
        source,
        targetId,
        targetCenter,
        applyDamageDelay};
    const QueuedDamageEventObservation queuedDamageEvent{
        damageEventId,
        attackId,
        targetId,
        queuedDamage,
        applyDamageDelay};
    AttackObservation attack;
    attack.id = attackId;
    attack.tick = world.GetCurrentTick().value_or(0);
    attack.attackerId = attackerId;
    attack.targetId = targetId;
    attack.callback = "standard_attack";
    attack.queuedDamageEvents.push_back(queuedDamageEvent);

    if (attackWeapon.projectileId > 0)
    {
        attack.projectile = projectile;
        const bool queued = world.QueueActorCombatEvent(
            targetId,
            applyDamageDelay,
            [this, &world, targetId, damage = queuedDamage, damageEventId, attackId]()
            {
                ApplyDamage(world, targetId, damage, damageEventId, attackId);
            },
            projectile);

        if (queued)
        {
            NotifyAttackQueued(attack);
        }

        return queued;
    }

    const bool queued = world.QueueActorCombatEvent(
        targetId,
        applyDamageDelay,
        [this, &world, targetId, damage = queuedDamage, damageEventId, attackId]()
        {
            ApplyDamage(world, targetId, damage, damageEventId, attackId);
        });

    if (queued)
    {
        NotifyAttackQueued(attack);
    }

    return queued;
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

int CombatService::ClampDamageToCurrentHitpoints(
    const World& world,
    ActorId targetId,
    int damage)
{
    const CombatComposition* currentComposition =
        world.GetActorCombatComposition(targetId);

    if (currentComposition == nullptr)
    {
        return 0;
    }

    return std::min(
        std::max(0, currentComposition->stats.hitpoints),
        std::max(0, damage));
}

void CombatService::ApplyDamage(
    World& world,
    ActorId targetId,
    int damage,
    std::uint64_t damageEventId,
    std::uint64_t attackId) const
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
    const int appliedDamage = std::min(
        currentHitpoints,
        positiveDamage);
    updatedComposition.stats.hitpoints -= appliedDamage;
    world.SetActorCombatComposition(targetId, updatedComposition);

    if (damageEventId != 0 && attackId != 0)
    {
        NotifyDamageApplied(
            {damageEventId,
             attackId,
             world.GetCurrentTick().value_or(0),
             targetId,
             positiveDamage,
             appliedDamage});
    }

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

void CombatService::NotifyAttackQueued(const AttackObservation& attack) const
{
    m_CurrentDispatchRecordedAttack = true;

    for (Observer* observer : m_Observers)
    {
        observer->OnAttackQueued(attack);
    }
}

void CombatService::NotifyDamageApplied(
    const DamageApplicationObservation& damageApplication) const
{
    for (Observer* observer : m_Observers)
    {
        observer->OnDamageApplied(damageApplication);
    }
}

}  // namespace osrssim
