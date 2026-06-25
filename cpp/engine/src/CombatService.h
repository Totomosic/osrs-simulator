#pragma once

#include "Types.h"
#include "World.h"

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace osrssim
{

class CombatService
{
private:
    using AttackCallback =
        std::function<bool(World&, ActorId, ActorId, Tick, const WeaponDefinition&)>;

    struct AttackCallbackBinding
    {
        AttackCallback callback;
        std::string callbackName;
    };

public:
    struct QueuedDamageEventObservation
    {
        std::uint64_t id = 0;
        std::uint64_t attackId = 0;
        ActorId targetId = 0;
        int damage = 0;
        int delayTicks = 0;
    };

    struct AttackObservation
    {
        std::uint64_t id = 0;
        Tick tick = 0;
        ActorId attackerId = 0;
        ActorId targetId = 0;
        std::string callback;
        std::vector<QueuedDamageEventObservation> queuedDamageEvents;
        std::optional<ProjectileMetadata> projectile;
    };

    struct DamageApplicationObservation
    {
        std::uint64_t damageEventId = 0;
        std::uint64_t attackId = 0;
        Tick tick = 0;
        ActorId targetId = 0;
        int queuedDamage = 0;
        int appliedDamage = 0;
    };

    class Observer
    {
    public:
        virtual ~Observer() = default;
        virtual void OnAttackQueued(const AttackObservation& attack) = 0;
        virtual void OnDamageApplied(
            const DamageApplicationObservation& damageApplication) = 0;
    };

private:
    mutable DpsService m_DpsService;
    AttackCallbackBinding m_GenericAttackCallback;
    std::unordered_map<std::string, AttackCallback> m_AttackCallbacksByName;
    std::unordered_map<WeaponId, AttackCallbackBinding> m_WeaponAttackCallbacks;
    mutable std::uint64_t m_NextAttackId = 1;
    mutable std::uint64_t m_NextDamageEventId = 1;
    mutable std::optional<std::string> m_CurrentAttackCallbackName;
    mutable bool m_CurrentDispatchRecordedAttack = false;
    std::vector<Observer*> m_Observers;

public:
    CombatService();

    void RegisterGenericAttackCallback(AttackCallback callback);
    void RegisterAttackCallbackName(
        const std::string& callbackName,
        AttackCallback callback);
    bool HasAttackCallbackName(const std::string& callbackName) const;
    void RegisterWeaponAttackCallback(
        WeaponId weaponId,
        AttackCallback callback);
    void BindWeaponAttackCallbackName(
        WeaponId weaponId,
        const std::string& callbackName);
    void AddObserver(Observer& observer);
    void RemoveObserver(Observer& observer);
    void SetDpsSeed(unsigned int seed);
    AttackObservation CreateAttackObservation(
        const World& world,
        ActorId attackerId,
        ActorId targetId,
        Tick currentTick) const;
    bool QueueStructuredDamageEvent(
        World& world,
        AttackObservation& attack,
        ActorId targetId,
        int damage,
        int delayTicks) const;
    bool QueueStructuredDamageEvent(
        World& world,
        AttackObservation& attack,
        ActorId targetId,
        int damage,
        int delayTicks,
        ProjectileMetadata projectile) const;
    void RecordAttackObservation(const AttackObservation& attack) const;
    void DecrementAttackTimers(World& world) const;
    bool CanAttackActorTarget(
        const World& world,
        ActorId attackerId,
        ActorId targetId) const;
    bool DispatchAttack(
        World& world,
        ActorId attackerId,
        ActorId targetId,
        Tick currentTick) const;

private:
    bool DefaultStandardAttack(
        World& world,
        ActorId attackerId,
        ActorId targetId,
        const WeaponDefinition& attackWeapon) const;
    DpsRequest BuildStandardDpsRequest(
        const World& world,
        ActorId attackerId,
        ActorId targetId,
        const WeaponDefinition& attackWeapon) const;
    int CalculateApplyDamageDelay(
        const World& world,
        ActorId attackerId,
        ActorId targetId) const;
    static int ClampDamageToCurrentHitpoints(
        const World& world,
        ActorId targetId,
        int damage);
    void ApplyDamage(
        World& world,
        ActorId targetId,
        int damage,
        std::uint64_t damageEventId = 0,
        std::uint64_t attackId = 0) const;
    static void QueueDeath(
        World& world,
        ActorId targetId);
    void NotifyAttackQueued(const AttackObservation& attack) const;
    void NotifyDamageApplied(
        const DamageApplicationObservation& damageApplication) const;
};

}  // namespace osrssim
