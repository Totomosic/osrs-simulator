#pragma once

#include "Types.h"
#include "World.h"

#include <functional>
#include <unordered_map>

namespace osrssim
{

class CombatService
{
private:
    using AttackCallback =
        std::function<void(World&, ActorId, ActorId, Tick, const WeaponDefinition&)>;

    AttackCallback m_GenericAttackCallback;
    std::unordered_map<WeaponId, AttackCallback> m_WeaponAttackCallbacks;

public:
    void RegisterGenericAttackCallback(AttackCallback callback);
    void RegisterWeaponAttackCallback(
        WeaponId weaponId,
        AttackCallback callback);
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
