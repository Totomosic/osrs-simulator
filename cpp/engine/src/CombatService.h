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
};

}  // namespace osrssim
