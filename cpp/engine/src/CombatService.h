#pragma once

#include "Types.h"
#include "World.h"

#include <functional>
#include <string>
#include <unordered_map>

namespace osrssim
{

class CombatService
{
private:
    using AttackCallback =
        std::function<bool(World&, ActorId, ActorId, Tick, const WeaponDefinition&)>;

    mutable DpsService m_DpsService;
    AttackCallback m_GenericAttackCallback;
    std::unordered_map<std::string, AttackCallback> m_AttackCallbacksByName;
    std::unordered_map<WeaponId, AttackCallback> m_WeaponAttackCallbacks;

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
    void SetDpsSeed(unsigned int seed);
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
    static void ApplyDamage(
        World& world,
        ActorId targetId,
        int damage);
};

}  // namespace osrssim
