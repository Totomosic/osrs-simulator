#include "Engine.h"

#include <cassert>
#include <memory>
#include <stdexcept>
#include <vector>

namespace
{
struct AttackObservation
{
    int count = 0;
    osrssim::ActorId attackerId = 0;
    osrssim::ActorId targetId = 0;
    osrssim::Tick tick = 0;
};

void RegisterAttackObservation(
    osrssim::Engine& engine,
    AttackObservation& observation)
{
    engine.GetCombatService().RegisterGenericAttackCallback(
        [&observation](
            osrssim::World&,
            osrssim::ActorId attacker,
            osrssim::ActorId target,
            osrssim::Tick currentTick,
            const osrssim::WeaponDefinition&)
        {
            ++observation.count;
            observation.attackerId = attacker;
            observation.targetId = target;
            observation.tick = currentTick;
            return true;
        });
}

osrssim::CombatComposition CombatCompositionWithWeapon(
    osrssim::WeaponDefinition weapon)
{
    osrssim::CombatComposition combatComposition;
    combatComposition.weapon = weapon;
    return combatComposition;
}

osrssim::CombatComposition StandardMeleeComposition(
    osrssim::WeaponDefinition weapon,
    int hitpoints)
{
    osrssim::CombatComposition combatComposition;
    combatComposition.stats.attack = 99;
    combatComposition.stats.strength = 99;
    combatComposition.stats.defence = 80;
    combatComposition.stats.hitpoints = hitpoints;
    combatComposition.baseStats = combatComposition.stats;
    combatComposition.bonuses.slashAttack = 132;
    combatComposition.bonuses.slashDefence = 80;
    combatComposition.bonuses.meleeStrength = 118;
    combatComposition.attackType = osrssim::AttackType::Slash;
    combatComposition.weapon = weapon;
    return combatComposition;
}

osrssim::DpsRequest StandardMeleeRequest(
    const osrssim::CombatComposition& attacker,
    const osrssim::CombatComposition& defender,
    osrssim::DefenderKind defenderKind)
{
    osrssim::DpsRequest request;
    request.attackComposition.attackType = attacker.attackType;
    request.attackComposition.stats = attacker.stats;
    request.attackComposition.bonuses = attacker.bonuses;
    request.attackComposition.weapon = attacker.weapon;
    request.defenceComposition.stats = defender.stats;
    request.defenceComposition.bonuses = defender.bonuses;
    request.defenderKind = defenderKind;
    request.magicBaseMaximumHit = attacker.magicBaseMaximumHit;
    return request;
}

class CountingBehavior : public osrssim::behavior::NpcBehavior
{
private:
    int* m_DestroyedCount = nullptr;
    bool m_CanBeShared = false;

public:
    CountingBehavior(int& destroyedCount, bool canBeShared = false)
        : m_DestroyedCount(&destroyedCount),
          m_CanBeShared(canBeShared)
    {
    }

    ~CountingBehavior() override
    {
        ++*m_DestroyedCount;
    }

    bool CanBeShared() const override
    {
        return m_CanBeShared;
    }

    void Update(
        osrssim::behavior::NpcBehaviorContext&,
        osrssim::ActorId) override
    {
    }
};
}  // namespace

int main()
{
    {
        osrssim::Engine engine;
        const osrssim::behavior::NpcBehavior* defaultBehavior =
            engine.GetNpcBehavior(0);

        assert(defaultBehavior != nullptr);
        assert(defaultBehavior->CanBeShared());
        assert(engine.GetNpcBehaviorCount() == 1);
        assert(engine.GetNpcBehavior(1) == nullptr);
    }

    {
        osrssim::Engine engine;
        osrssim::World& world = engine.GetWorld();
        osrssim::ActorId playerId = world.CreatePlayer(1, 1, osrssim::CombatComposition{}).value();

        assert(world.PlaceActor(
            playerId,
            world.GetDefaultSceneId(),
            {10, 10, 0}));

        assert(engine.SetPlayerSceneCoordinateMovementTarget(
            playerId,
            {11, 10, 0}));

        assert(engine.GetCurrentTick() == 0);
        assert(world.GetSceneMembership(playerId)->coordinate ==
               (osrssim::SceneCoordinate{10, 10, 0}));
        assert(world.GetPlayer(playerId)->movementTarget.has_value());
        assert(world.GetPlayer(playerId)->movementTarget->sceneCoordinate ==
               (osrssim::SceneCoordinate{11, 10, 0}));

        engine.Step();

        assert(world.GetSceneMembership(playerId)->coordinate ==
               (osrssim::SceneCoordinate{11, 10, 0}));
    }

    {
        osrssim::Engine engine;
        osrssim::World& world = engine.GetWorld();
        osrssim::ActorId playerId = world.CreatePlayer(1, 1, osrssim::CombatComposition{}).value();
        osrssim::ActorId npcId = world.CreateNpc(1, 1, osrssim::CombatComposition{}).value();
        int firedCount = 0;

        assert(world.PlaceActor(
            playerId,
            world.GetDefaultSceneId(),
            {10, 10, 0}));
        assert(world.PlaceActor(npcId, world.GetDefaultSceneId(), {20, 20, 0}));
        assert(engine.QueuePlayerMoveToSceneCoordinate(playerId, {11, 10, 0}));
        assert(world.SetActorSceneCoordinateMovementTarget(npcId, {21, 20, 0}));
        assert(world.GetActorCombatQueue(playerId)->AddEvent(
            1,
            [&world, playerId, &firedCount]()
            {
                ++firedCount;
                assert(world.SetActorSpeed(playerId, 0));
            }));
        assert(world.GetActorCombatQueue(npcId)->AddEvent(
            1,
            [&world, npcId, &firedCount]()
            {
                ++firedCount;
                assert(world.SetActorSpeed(npcId, 0));
            }));

        engine.Step();

        assert(firedCount == 2);
        assert(world.GetSceneMembership(playerId)->coordinate ==
               (osrssim::SceneCoordinate{10, 10, 0}));
        assert(world.GetSceneMembership(npcId)->coordinate ==
               (osrssim::SceneCoordinate{20, 20, 0}));
    }

    {
        osrssim::Engine engine;
        osrssim::World& world = engine.GetWorld();
        osrssim::ActorId playerId = world.CreatePlayer(1, 1, osrssim::CombatComposition{}).value();
        osrssim::ActorId targetId = world.CreateNpc(1, 1, osrssim::CombatComposition{}).value();
        AttackObservation attack;

        assert(world.PlaceActor(
            playerId,
            world.GetDefaultSceneId(),
            {10, 10, 0}));
        assert(world.PlaceActor(
            targetId,
            world.GetDefaultSceneId(),
            {11, 10, 0}));
        assert(world.SetActorCombatComposition(playerId, CombatCompositionWithWeapon({42, 1, 5})));
        assert(world.SetActorMovementTarget(playerId, targetId));
        assert(world.GetActorCombatQueue(playerId)->AddEvent(
            1,
            [&world, playerId]()
            {
                assert(world.SetActorAttackTimer(playerId, 3));
            }));
        RegisterAttackObservation(engine, attack);

        engine.Step();

        assert(attack.count == 0);
        assert(world.GetActorAttackTimer(playerId) == 3);
    }

    {
        osrssim::Engine engine;
        osrssim::World& world = engine.GetWorld();
        osrssim::ActorId playerId = world.CreatePlayer(1, 1, osrssim::CombatComposition{}).value();
        osrssim::ActorId npcId = world.CreateNpc(1, 1, osrssim::CombatComposition{}).value();
        osrssim::ActorId unplacedNpcId = world.CreateNpc(1, 1, osrssim::CombatComposition{}).value();

        assert(world.PlaceActor(
            playerId,
            world.GetDefaultSceneId(),
            {10, 10, 0}));
        assert(world.PlaceActor(npcId, world.GetDefaultSceneId(), {20, 20, 0}));
        assert(world.SetActorAttackTimer(playerId, 3));
        assert(world.SetActorAttackTimer(npcId, 0));
        assert(world.SetActorAttackTimer(unplacedNpcId, -2));
        assert(engine.QueuePlayerMoveToSceneCoordinate(playerId, {11, 10, 0}));

        engine.Step();

        assert(world.GetSceneMembership(playerId)->coordinate ==
               (osrssim::SceneCoordinate{11, 10, 0}));
        assert(world.GetActorAttackTimer(playerId) == 2);
        assert(world.GetActorAttackTimer(npcId) == -1);
        assert(world.GetActorAttackTimer(unplacedNpcId) == -3);
    }

    {
        osrssim::Engine engine;
        osrssim::World& world = engine.GetWorld();
        osrssim::ActorId attackerId = world.CreatePlayer(1, 1, osrssim::CombatComposition{}).value();
        osrssim::ActorId targetId = world.CreateNpc(1, 1, osrssim::CombatComposition{}).value();
        int genericAttackCount = 0;
        osrssim::ActorId callbackAttackerId = 0;
        osrssim::ActorId callbackTargetId = 0;
        osrssim::Tick callbackTick = 0;
        osrssim::WeaponDefinition callbackWeapon;
        int callbackAttackTimer = 0;

        assert(world.SetActorCombatComposition(attackerId, CombatCompositionWithWeapon({42, 7, 5})));
        engine.GetCombatService().RegisterGenericAttackCallback(
            [&genericAttackCount,
             &callbackAttackerId,
             &callbackTargetId,
             &callbackTick,
             &callbackWeapon,
             &callbackAttackTimer](
                osrssim::World& callbackWorld,
                osrssim::ActorId attacker,
                osrssim::ActorId target,
                osrssim::Tick currentTick,
                const osrssim::WeaponDefinition& weapon)
            {
                ++genericAttackCount;
                callbackAttackerId = attacker;
                callbackTargetId = target;
                callbackTick = currentTick;
                callbackWeapon = weapon;
                callbackAttackTimer =
                    callbackWorld.GetActorAttackTimer(attacker);
                return true;
            });

        assert(engine.GetCombatService().DispatchAttack(
            world,
            attackerId,
            targetId,
            9));

        assert(genericAttackCount == 1);
        assert(callbackAttackerId == attackerId);
        assert(callbackTargetId == targetId);
        assert(callbackTick == 9);
        assert(callbackWeapon == (osrssim::WeaponDefinition{42, 7, 5}));
        assert(callbackAttackTimer == 0);
        assert(world.GetActorAttackTimer(attackerId) == 5);
    }

    {
        osrssim::Engine engine;
        osrssim::World& world = engine.GetWorld();
        osrssim::ActorId attackerId = world.CreatePlayer(1, 1, osrssim::CombatComposition{}).value();
        osrssim::ActorId targetId = world.CreateNpc(1, 1, osrssim::CombatComposition{}).value();
        int genericAttackCount = 0;
        int weaponAttackCount = 0;

        assert(world.SetActorCombatComposition(attackerId, CombatCompositionWithWeapon({42, 7, 5})));
        engine.GetCombatService().RegisterGenericAttackCallback(
            [&genericAttackCount](
                osrssim::World&,
                osrssim::ActorId,
                osrssim::ActorId,
                osrssim::Tick,
                const osrssim::WeaponDefinition&)
            {
                ++genericAttackCount;
                return true;
            });
        engine.GetCombatService().RegisterWeaponAttackCallback(
            42,
            [&weaponAttackCount](
                osrssim::World&,
                osrssim::ActorId,
                osrssim::ActorId,
                osrssim::Tick,
                const osrssim::WeaponDefinition&)
            {
                ++weaponAttackCount;
                return true;
            });

        assert(engine.GetCombatService().DispatchAttack(
            world,
            attackerId,
            targetId,
            1));

        assert(genericAttackCount == 0);
        assert(weaponAttackCount == 1);
        assert(world.GetActorAttackTimer(attackerId) == 5);
    }

    {
        osrssim::Engine engine;
        osrssim::World& world = engine.GetWorld();
        osrssim::ActorId attackerId = world.CreatePlayer(1, 1, osrssim::CombatComposition{}).value();
        osrssim::ActorId targetId = world.CreateNpc(1, 1, osrssim::CombatComposition{}).value();
        int failedAttackCount = 0;

        assert(world.SetActorCombatComposition(attackerId, CombatCompositionWithWeapon({42, 7, 5})));
        assert(world.SetActorAttackTimer(attackerId, -2));
        engine.GetCombatService().RegisterGenericAttackCallback(
            [&failedAttackCount](
                osrssim::World&,
                osrssim::ActorId,
                osrssim::ActorId,
                osrssim::Tick,
                const osrssim::WeaponDefinition&)
            {
                ++failedAttackCount;
                return false;
            });

        assert(!engine.GetCombatService().DispatchAttack(
            world,
            attackerId,
            targetId,
            1));

        assert(failedAttackCount == 1);
        assert(world.GetActorAttackTimer(attackerId) == -2);
    }

    {
        osrssim::Engine engine;
        osrssim::World& world = engine.GetWorld();
        osrssim::ActorId attackerId = world.CreatePlayer(1, 1, osrssim::CombatComposition{}).value();
        osrssim::ActorId targetId = world.CreateNpc(1, 1, osrssim::CombatComposition{}).value();

        assert(world.SetActorCombatComposition(attackerId, CombatCompositionWithWeapon({7, 3, 2})));
        assert(world.PlaceActor(
            attackerId,
            world.GetDefaultSceneId(),
            {10, 10, 0}));
        assert(world.PlaceActor(
            targetId,
            world.GetDefaultSceneId(),
            {11, 10, 0}));

        assert(engine.GetCombatService().DispatchAttack(
            world,
            attackerId,
            targetId,
            1));

        assert(world.GetActorAttackTimer(attackerId) == 2);

        assert(world.SetActorCombatComposition(attackerId, CombatCompositionWithWeapon({8, 3, 4})));
        assert(world.SetActorCombatComposition(targetId, osrssim::CombatComposition{}));
        assert(world.SetActorAttackTimer(attackerId, 0));
        engine.GetCombatService().BindWeaponAttackCallbackName(
            8,
            "standard_attack");

        assert(engine.GetCombatService().DispatchAttack(
            world,
            attackerId,
            targetId,
            2));

        assert(world.GetActorAttackTimer(attackerId) == 4);
        assert(!engine.GetCombatService().DispatchAttack(
            world,
            attackerId,
            999,
            1));
    }

    {
        osrssim::Engine engine;
        osrssim::World& world = engine.GetWorld();
        const osrssim::WeaponDefinition weapon{42, 8, 5, 321};
        const osrssim::CombatComposition attackerComposition =
            StandardMeleeComposition(weapon, 99);
        const osrssim::CombatComposition defenderComposition =
            StandardMeleeComposition({0, 1, 4}, 99);
        const osrssim::ActorId attackerId =
            world.CreatePlayer(1, 1, attackerComposition).value();
        const osrssim::ActorId targetId =
            world.CreateNpc(1, 1, defenderComposition).value();
        osrssim::DpsService expectedDpsService;

        engine.GetCombatService().SetDpsSeed(12345);
        expectedDpsService.SetSeed(12345);
        const osrssim::DpsSampleResult expectedSample =
            expectedDpsService.SampleSingleAttack(StandardMeleeRequest(
                attackerComposition,
                defenderComposition,
                osrssim::DefenderKind::Npc));

        assert(world.PlaceActor(
            attackerId,
            world.GetDefaultSceneId(),
            {10, 10, 0}));
        assert(world.PlaceActor(
            targetId,
            world.GetDefaultSceneId(),
            {13, 10, 0}));
        assert(engine.QueuePlayerMoveToActor(attackerId, targetId));

        engine.Step();

        assert(world.GetActorAttackTimer(attackerId) == weapon.speed);
        assert(world.GetActorCombatQueue(targetId)->GetEventCount() == 1);
        {
            const std::vector<osrssim::ProjectileSnapshot> projectiles =
                world.GetProjectileSnapshots();
            assert(projectiles.size() == 1);
            assert(projectiles[0].projectileId == 321);
            assert(projectiles[0].source == (osrssim::ScenePosition{10.5, 10.5, 0}));
            assert(projectiles[0].targetActorId == targetId);
            assert(projectiles[0].lastKnownTargetCenter ==
                   (osrssim::ScenePosition{13.5, 10.5, 0}));
            assert(projectiles[0].elapsedTicks == 0);
            assert(projectiles[0].totalTicks == 2);
        }
        assert(
            world.GetActorCombatComposition(targetId)->stats.hitpoints ==
            defenderComposition.stats.hitpoints);

        engine.Step();

        {
            const std::vector<osrssim::ProjectileSnapshot> projectiles =
                world.GetProjectileSnapshots();
            assert(projectiles.size() == 1);
            assert(projectiles[0].source == (osrssim::ScenePosition{10.5, 10.5, 0}));
            assert(projectiles[0].elapsedTicks == 1);
            assert(projectiles[0].totalTicks == 2);
        }
        assert(
            world.GetActorCombatComposition(targetId)->stats.hitpoints ==
            defenderComposition.stats.hitpoints);

        engine.Step();

        assert(
            world.GetActorCombatComposition(targetId)->stats.hitpoints ==
            defenderComposition.stats.hitpoints - expectedSample.sampledDamage);
        assert(
            world.GetActorCombatComposition(targetId)->baseStats.hitpoints ==
            defenderComposition.stats.hitpoints);
        assert(world.GetActorCombatQueue(targetId)->GetEventCount() == 0);
        assert(world.GetProjectileSnapshots().empty());
    }

    {
        osrssim::Engine engine;
        osrssim::World& world = engine.GetWorld();
        const osrssim::WeaponDefinition weapon{42, 8, 5, 321};
        const osrssim::CombatComposition attackerComposition =
            StandardMeleeComposition(weapon, 99);
        const osrssim::CombatComposition defenderComposition =
            StandardMeleeComposition({0, 1, 4}, 99);
        const osrssim::ActorId targetId =
            world.CreatePlayer(1, 1, defenderComposition).value();
        const osrssim::ActorId attackerId =
            world.CreateNpc(1, 1, attackerComposition).value();

        assert(world.PlaceActor(
            targetId,
            world.GetDefaultSceneId(),
            {10, 10, 0}));
        assert(world.PlaceActor(
            attackerId,
            world.GetDefaultSceneId(),
            {13, 10, 0}));
        assert(world.SetActorMovementTarget(attackerId, targetId));

        engine.Step();

        {
            const std::vector<osrssim::ProjectileSnapshot> projectiles =
                world.GetProjectileSnapshots();
            assert(projectiles.size() == 1);
            assert(projectiles[0].projectileId == 321);
            assert(projectiles[0].source == (osrssim::ScenePosition{13.5, 10.5, 0}));
            assert(projectiles[0].targetActorId == targetId);
            assert(projectiles[0].lastKnownTargetCenter ==
                   (osrssim::ScenePosition{10.5, 10.5, 0}));
            assert(projectiles[0].elapsedTicks == 0);
            assert(projectiles[0].totalTicks == 2);
        }
    }

    {
        osrssim::Engine engine;
        osrssim::World& world = engine.GetWorld();
        const osrssim::WeaponDefinition weapon{79, 8, 5, 0};
        const osrssim::CombatComposition attackerComposition =
            StandardMeleeComposition(weapon, 99);
        const osrssim::CombatComposition defenderComposition =
            StandardMeleeComposition({0, 1, 4}, 99);
        const osrssim::ActorId targetId =
            world.CreatePlayer(1, 1, defenderComposition).value();
        const osrssim::ActorId attackerId =
            world.CreateNpc(1, 1, attackerComposition).value();

        engine.GetCombatService().RegisterAttackCallbackName(
            "custom_immediate_attack",
            [](
                osrssim::World& callbackWorld,
                osrssim::ActorId,
                osrssim::ActorId target,
                osrssim::Tick,
                const osrssim::WeaponDefinition&)
            {
                return callbackWorld.QueueActorCombatEvent(
                    target,
                    0,
                    [&callbackWorld, target]()
                    {
                        osrssim::CombatComposition combatComposition =
                            *callbackWorld.GetActorCombatComposition(target);
                        combatComposition.stats.hitpoints -= 5;
                        callbackWorld.SetActorCombatComposition(
                            target,
                            combatComposition);
                    });
            });
        engine.GetCombatService().BindWeaponAttackCallbackName(
            weapon.id,
            "custom_immediate_attack");

        assert(world.PlaceActor(
            targetId,
            world.GetDefaultSceneId(),
            {10, 10, 0}));
        assert(world.PlaceActor(
            attackerId,
            world.GetDefaultSceneId(),
            {13, 10, 0}));
        assert(world.SetActorMovementTarget(attackerId, targetId));

        engine.Step();

        assert(
            world.GetActorCombatComposition(targetId)->stats.hitpoints ==
            defenderComposition.stats.hitpoints - 5);
        assert(world.GetActorCombatQueue(targetId)->GetEventCount() == 0);
    }

    {
        osrssim::Engine engine;
        osrssim::World& world = engine.GetWorld();
        const osrssim::WeaponDefinition weapon{42, 8, 5, 0};
        const osrssim::CombatComposition attackerComposition =
            StandardMeleeComposition(weapon, 99);
        const osrssim::CombatComposition defenderComposition =
            StandardMeleeComposition({0, 1, 4}, 99);
        const osrssim::ActorId attackerId =
            world.CreatePlayer(1, 1, attackerComposition).value();
        const osrssim::ActorId targetId =
            world.CreateNpc(1, 1, defenderComposition).value();

        assert(world.PlaceActor(
            attackerId,
            world.GetDefaultSceneId(),
            {10, 10, 0}));
        assert(world.PlaceActor(
            targetId,
            world.GetDefaultSceneId(),
            {13, 10, 0}));

        assert(engine.GetCombatService().DispatchAttack(
            world,
            attackerId,
            targetId,
            1));

        assert(world.GetActorCombatQueue(targetId)->GetEventCount() == 1);
        assert(world.GetProjectileSnapshots().empty());
    }

    {
        osrssim::Engine engine;
        osrssim::World& world = engine.GetWorld();
        const osrssim::WeaponDefinition weapon{77, 8, 5, 0};
        const osrssim::CombatComposition attackerComposition =
            StandardMeleeComposition(weapon, 99);
        const osrssim::CombatComposition defenderComposition =
            StandardMeleeComposition({0, 1, 4}, 99);
        const osrssim::ActorId attackerId =
            world.CreatePlayer(1, 1, attackerComposition).value();
        const osrssim::ActorId targetId =
            world.CreateNpc(1, 1, defenderComposition).value();

        engine.GetCombatService().RegisterAttackCallbackName(
            "custom_projectile_attack",
            [](
                osrssim::World& callbackWorld,
                osrssim::ActorId attacker,
                osrssim::ActorId target,
                osrssim::Tick,
                const osrssim::WeaponDefinition&)
            {
                return callbackWorld.QueueActorCombatEvent(
                    target,
                    2,
                    [&callbackWorld, target]()
                    {
                        osrssim::CombatComposition combatComposition =
                            *callbackWorld.GetActorCombatComposition(target);
                        combatComposition.stats.hitpoints -= 4;
                        callbackWorld.SetActorCombatComposition(
                            target,
                            combatComposition);
                    },
                    osrssim::ProjectileMetadata{
                        777,
                        callbackWorld.GetActorFootprintCenter(attacker),
                        target,
                        callbackWorld.GetActorFootprintCenter(target)});
            });
        engine.GetCombatService().BindWeaponAttackCallbackName(
            weapon.id,
            "custom_projectile_attack");

        assert(world.PlaceActor(
            attackerId,
            world.GetDefaultSceneId(),
            {10, 10, 0}));
        assert(world.PlaceActor(
            targetId,
            world.GetDefaultSceneId(),
            {13, 10, 0}));

        assert(engine.GetCombatService().DispatchAttack(
            world,
            attackerId,
            targetId,
            1));

        assert(world.GetActorCombatQueue(targetId)->GetEventCount() == 1);
        {
            const std::vector<osrssim::ProjectileSnapshot> projectiles =
                world.GetProjectileSnapshots();
            assert(projectiles.size() == 1);
            assert(projectiles[0].projectileId == 777);
            assert(projectiles[0].source == (osrssim::ScenePosition{10.5, 10.5, 0}));
            assert(projectiles[0].targetActorId == targetId);
            assert(projectiles[0].lastKnownTargetCenter ==
                   (osrssim::ScenePosition{13.5, 10.5, 0}));
            assert(projectiles[0].elapsedTicks == 0);
            assert(projectiles[0].totalTicks == 2);
        }
        assert(
            world.GetActorCombatComposition(targetId)->stats.hitpoints ==
            defenderComposition.stats.hitpoints);

        engine.Step();

        {
            const std::vector<osrssim::ProjectileSnapshot> projectiles =
                world.GetProjectileSnapshots();
            assert(projectiles.size() == 1);
            assert(projectiles[0].elapsedTicks == 1);
            assert(projectiles[0].totalTicks == 2);
        }
        assert(
            world.GetActorCombatComposition(targetId)->stats.hitpoints ==
            defenderComposition.stats.hitpoints);

        engine.Step();

        assert(
            world.GetActorCombatComposition(targetId)->stats.hitpoints ==
            defenderComposition.stats.hitpoints - 4);
        assert(world.GetActorCombatQueue(targetId)->GetEventCount() == 0);
        assert(world.GetProjectileSnapshots().empty());
    }

    {
        osrssim::Engine engine;
        osrssim::World& world = engine.GetWorld();
        const osrssim::WeaponDefinition weapon{78, 8, 5, 0};
        const osrssim::CombatComposition attackerComposition =
            StandardMeleeComposition(weapon, 99);
        const osrssim::CombatComposition defenderComposition =
            StandardMeleeComposition({0, 1, 4}, 99);
        const osrssim::ActorId attackerId =
            world.CreatePlayer(1, 1, attackerComposition).value();
        const osrssim::ActorId targetId =
            world.CreateNpc(1, 1, defenderComposition).value();

        engine.GetCombatService().RegisterAttackCallbackName(
            "custom_delayed_attack",
            [](
                osrssim::World& callbackWorld,
                osrssim::ActorId,
                osrssim::ActorId target,
                osrssim::Tick,
                const osrssim::WeaponDefinition&)
            {
                return callbackWorld.QueueActorCombatEvent(
                    target,
                    1,
                    [&callbackWorld, target]()
                    {
                        osrssim::CombatComposition combatComposition =
                            *callbackWorld.GetActorCombatComposition(target);
                        combatComposition.stats.hitpoints -= 3;
                        callbackWorld.SetActorCombatComposition(
                            target,
                            combatComposition);
                    });
            });
        engine.GetCombatService().BindWeaponAttackCallbackName(
            weapon.id,
            "custom_delayed_attack");

        assert(world.PlaceActor(
            attackerId,
            world.GetDefaultSceneId(),
            {10, 10, 0}));
        assert(world.PlaceActor(
            targetId,
            world.GetDefaultSceneId(),
            {13, 10, 0}));

        assert(engine.GetCombatService().DispatchAttack(
            world,
            attackerId,
            targetId,
            1));

        assert(world.GetActorCombatQueue(targetId)->GetEventCount() == 1);
        assert(world.GetProjectileSnapshots().empty());

        engine.Step();

        assert(
            world.GetActorCombatComposition(targetId)->stats.hitpoints ==
            defenderComposition.stats.hitpoints - 3);
        assert(world.GetActorCombatQueue(targetId)->GetEventCount() == 0);
        assert(world.GetProjectileSnapshots().empty());
    }

    {
        osrssim::Engine engine;
        osrssim::World& world = engine.GetWorld();
        const osrssim::WeaponDefinition weapon{42, 8, 5};
        const osrssim::CombatComposition attackerComposition =
            StandardMeleeComposition(weapon, 99);
        osrssim::CombatComposition defenderComposition =
            StandardMeleeComposition({0, 1, 4}, 99);
        osrssim::DpsService expectedDpsService;

        engine.GetCombatService().SetDpsSeed(12345);
        expectedDpsService.SetSeed(12345);
        const osrssim::DpsSampleResult expectedSample =
            expectedDpsService.SampleSingleAttack(StandardMeleeRequest(
                attackerComposition,
                defenderComposition,
                osrssim::DefenderKind::Npc));

        assert(expectedSample.sampledDamage > 1);
        defenderComposition.stats.hitpoints = expectedSample.sampledDamage - 1;

        const osrssim::ActorId attackerId =
            world.CreatePlayer(1, 1, attackerComposition).value();
        const osrssim::ActorId targetId =
            world.CreateNpc(1, 1, defenderComposition).value();

        assert(world.PlaceActor(
            attackerId,
            world.GetDefaultSceneId(),
            {10, 10, 0}));
        assert(world.PlaceActor(
            targetId,
            world.GetDefaultSceneId(),
            {11, 10, 0}));

        assert(engine.GetCombatService().DispatchAttack(
            world,
            attackerId,
            targetId,
            1));

        engine.Step();

        assert(world.GetActorCombatComposition(targetId)->stats.hitpoints == 0);
    }

    {
        osrssim::Engine engine;
        osrssim::World& world = engine.GetWorld();
        const osrssim::WeaponDefinition weapon{42, 8, 5};
        const osrssim::CombatComposition attackerComposition =
            StandardMeleeComposition(weapon, 99);
        osrssim::CombatComposition defenderComposition =
            StandardMeleeComposition({0, 1, 4}, 99);
        osrssim::DpsService expectedDpsService;

        engine.GetCombatService().SetDpsSeed(12345);
        expectedDpsService.SetSeed(12345);
        const osrssim::DpsSampleResult expectedSample =
            expectedDpsService.SampleSingleAttack(StandardMeleeRequest(
                attackerComposition,
                defenderComposition,
                osrssim::DefenderKind::Npc));

        assert(expectedSample.sampledDamage > 0);
        defenderComposition.stats.hitpoints = expectedSample.sampledDamage;

        const osrssim::ActorId attackerId =
            world.CreatePlayer(1, 1, attackerComposition).value();
        const osrssim::ActorId targetId =
            world.CreateNpc(1, 1, defenderComposition).value();
        osrssim::Scene* scene = world.TryGetScene(world.GetDefaultSceneId());

        assert(scene != nullptr);
        assert(world.PlaceActor(
            attackerId,
            world.GetDefaultSceneId(),
            {10, 10, 0}));
        assert(world.PlaceActor(
            targetId,
            world.GetDefaultSceneId(),
            {11, 10, 0}));

        assert(engine.GetCombatService().DispatchAttack(
            world,
            attackerId,
            targetId,
            1));

        engine.Step();

        assert(world.GetNpc(targetId) != nullptr);
        assert(world.GetSceneMembership(targetId) != nullptr);
        assert(scene->TryGetTile({11, 10, 0})->IsOccupied());
        assert(world.GetActorCombatComposition(targetId)->stats.hitpoints == 0);

        engine.Step();

        assert(world.GetNpc(targetId) == nullptr);
        assert(world.GetActorCore(targetId) == nullptr);
        assert(world.GetSceneMembership(targetId) == nullptr);
        assert(!scene->TryGetTile({11, 10, 0})->IsOccupied());
    }

    {
        osrssim::Engine engine;
        osrssim::World& world = engine.GetWorld();
        const osrssim::WeaponDefinition weapon{42, 8, 5};
        const osrssim::CombatComposition attackerComposition =
            StandardMeleeComposition(weapon, 99);
        osrssim::CombatComposition defenderComposition =
            StandardMeleeComposition({0, 1, 4}, 1);
        const osrssim::ActorId attackerId =
            world.CreatePlayer(1, 1, attackerComposition).value();
        const osrssim::ActorId targetId =
            world.CreateNpc(1, 1, defenderComposition).value();

        assert(world.PlaceActor(
            attackerId,
            world.GetDefaultSceneId(),
            {10, 10, 0}));
        assert(world.PlaceActor(
            targetId,
            world.GetDefaultSceneId(),
            {11, 10, 0}));

        engine.GetCombatService().SetDpsSeed(12345);
        assert(engine.GetCombatService().DispatchAttack(
            world,
            attackerId,
            targetId,
            1));
        assert(world.SetActorAttackTimer(attackerId, 0));
        assert(engine.GetCombatService().DispatchAttack(
            world,
            attackerId,
            targetId,
            1));
        assert(world.GetActorCombatQueue(targetId)->GetEventCount() == 2);

        engine.Step();

        assert(world.GetActorCombatComposition(targetId)->stats.hitpoints == 0);
        assert(world.GetActorCombatQueue(targetId)->GetEventCount() == 1);

        engine.Step();

        assert(world.GetNpc(targetId) == nullptr);
    }

    {
        osrssim::Engine engine;
        osrssim::World& world = engine.GetWorld();
        const osrssim::WeaponDefinition weapon{42, 8, 5};
        const osrssim::CombatComposition attackerComposition =
            StandardMeleeComposition(weapon, 99);
        osrssim::CombatComposition defenderComposition =
            StandardMeleeComposition({0, 1, 4}, 99);
        osrssim::DpsService expectedDpsService;
        int destroyedCount = 0;

        engine.GetCombatService().SetDpsSeed(12345);
        expectedDpsService.SetSeed(12345);
        const osrssim::DpsSampleResult expectedSample =
            expectedDpsService.SampleSingleAttack(StandardMeleeRequest(
                attackerComposition,
                defenderComposition,
                osrssim::DefenderKind::Npc));

        assert(expectedSample.sampledDamage > 0);
        defenderComposition.stats.hitpoints = expectedSample.sampledDamage;

        const osrssim::ActorId attackerId =
            world.CreatePlayer(1, 1, attackerComposition).value();
        const osrssim::ActorId targetId =
            engine.CreateNpc(
                1,
                1,
                defenderComposition,
                std::make_unique<CountingBehavior>(destroyedCount))
                .value();
        const osrssim::NpcBehaviorId behaviorId =
            world.GetNpc(targetId)->behaviorId;

        assert(world.PlaceActor(
            attackerId,
            world.GetDefaultSceneId(),
            {10, 10, 0}));
        assert(world.PlaceActor(
            targetId,
            world.GetDefaultSceneId(),
            {11, 10, 0}));

        assert(engine.GetCombatService().DispatchAttack(
            world,
            attackerId,
            targetId,
            1));

        engine.Step();
        engine.Step();

        assert(world.GetNpc(targetId) == nullptr);
        assert(engine.GetNpcBehavior(behaviorId) == nullptr);
        assert(destroyedCount == 1);
    }

    {
        osrssim::Engine engine;
        osrssim::World& world = engine.GetWorld();
        const osrssim::WeaponDefinition weapon{42, 8, 5};
        const osrssim::CombatComposition attackerComposition =
            StandardMeleeComposition(weapon, 99);
        osrssim::CombatComposition defenderComposition =
            StandardMeleeComposition({0, 1, 4}, 99);
        osrssim::DpsService expectedDpsService;

        engine.GetCombatService().SetDpsSeed(12345);
        expectedDpsService.SetSeed(12345);
        const osrssim::DpsSampleResult expectedSample =
            expectedDpsService.SampleSingleAttack(StandardMeleeRequest(
                attackerComposition,
                defenderComposition,
                osrssim::DefenderKind::Player));

        assert(expectedSample.sampledDamage > 0);
        defenderComposition.stats.hitpoints = expectedSample.sampledDamage;

        const osrssim::ActorId attackerId =
            world.CreateNpc(1, 1, attackerComposition).value();
        const osrssim::ActorId targetId =
            world.CreatePlayer(1, 1, defenderComposition).value();

        assert(world.PlaceActor(
            attackerId,
            world.GetDefaultSceneId(),
            {10, 10, 0}));
        assert(world.PlaceActor(
            targetId,
            world.GetDefaultSceneId(),
            {11, 10, 0}));

        assert(engine.GetCombatService().DispatchAttack(
            world,
            attackerId,
            targetId,
            1));

        engine.Step();
        engine.Step();

        assert(world.GetPlayer(targetId) == nullptr);
        assert(world.GetActorCore(targetId) == nullptr);
    }

    {
        osrssim::Engine engine;
        osrssim::World& world = engine.GetWorld();
        osrssim::CombatComposition zeroHitpointsAttacker =
            StandardMeleeComposition({42, 8, 5}, 0);
        osrssim::CombatComposition zeroHitpointsTarget =
            StandardMeleeComposition({0, 1, 4}, 0);
        osrssim::CombatComposition livingTarget =
            StandardMeleeComposition({0, 1, 4}, 10);
        const osrssim::ActorId attackerId =
            world.CreatePlayer(1, 1, zeroHitpointsAttacker).value();
        const osrssim::ActorId zeroTargetId =
            world.CreateNpc(1, 1, zeroHitpointsTarget).value();
        const osrssim::ActorId livingTargetId =
            world.CreateNpc(1, 1, livingTarget).value();

        assert(world.PlaceActor(
            attackerId,
            world.GetDefaultSceneId(),
            {10, 10, 0}));
        assert(world.PlaceActor(
            zeroTargetId,
            world.GetDefaultSceneId(),
            {11, 10, 0}));
        assert(world.PlaceActor(
            livingTargetId,
            world.GetDefaultSceneId(),
            {10, 11, 0}));

        assert(!engine.GetCombatService().CanAttackActorTarget(
            world,
            attackerId,
            zeroTargetId));
        assert(engine.GetCombatService().CanAttackActorTarget(
            world,
            attackerId,
            livingTargetId));
    }

    {
        osrssim::Engine engine;
        osrssim::World& world = engine.GetWorld();
        osrssim::ActorId playerId = world.CreatePlayer(1, 1, osrssim::CombatComposition{}).value();

        assert(world.PlaceActor(
            playerId,
            world.GetDefaultSceneId(),
            {10, 10, 0}));

        assert(engine.QueuePlayerMoveToSceneCoordinate(playerId, {11, 10, 0}));
        assert(engine.QueuePlayerMoveToSceneCoordinate(playerId, {10, 11, 0}));

        engine.Step();

        assert(engine.GetCurrentTick() == 1);
        assert(world.GetSceneMembership(playerId)->coordinate ==
               (osrssim::SceneCoordinate{10, 11, 0}));
    }

    {
        osrssim::Engine engine;
        osrssim::World& world = engine.GetWorld();
        osrssim::Scene* scene = world.TryGetScene(world.GetDefaultSceneId());
        osrssim::ActorId playerId = world.CreatePlayer(1, 1, osrssim::CombatComposition{}).value();

        assert(scene != nullptr);
        assert(world.PlaceActor(
            playerId,
            world.GetDefaultSceneId(),
            {10, 10, 0}));

        scene->TryGetTile({10, 11, 0})
            ->AddFlag(osrssim::TileFlag::BlockMovementObject);

        assert(engine.QueuePlayerMoveToSceneCoordinate(playerId, {11, 10, 0}));
        assert(engine.QueuePlayerMoveToSceneCoordinate(playerId, {10, 11, 0}));

        engine.Step();

        assert(world.GetSceneMembership(playerId)->coordinate ==
               (osrssim::SceneCoordinate{10, 10, 0}));
        assert(!world.GetPlayer(playerId)->movementTarget.has_value());
    }

    {
        osrssim::Engine engine;
        osrssim::World& world = engine.GetWorld();
        osrssim::ActorId npcId = world.CreateNpc(1, 1, osrssim::CombatComposition{}).value();

        assert(world.PlaceActor(npcId, world.GetDefaultSceneId(), {10, 10, 0}));
        assert(!engine.QueuePlayerMoveToSceneCoordinate(npcId, {11, 10, 0}));

        engine.Step();

        assert(world.GetSceneMembership(npcId)->coordinate ==
               (osrssim::SceneCoordinate{10, 10, 0}));
    }

    {
        osrssim::Engine engine;
        osrssim::World& world = engine.GetWorld();
        osrssim::ActorId playerId = world.CreatePlayer(1, 1, osrssim::CombatComposition{}).value();

        assert(engine.QueuePlayerMoveToSceneCoordinate(playerId, {11, 10, 0}));

        engine.Step();

        assert(world.GetSceneMembership(playerId) == nullptr);
        assert(!world.GetPlayer(playerId)->movementTarget.has_value());
    }

    {
        osrssim::Engine engine;
        osrssim::World& world = engine.GetWorld();
        osrssim::ActorId npcId = world.CreateNpc(1, 1, osrssim::CombatComposition{}).value();
        osrssim::ActorId playerId = world.CreatePlayer(1, 1, osrssim::CombatComposition{}).value();

        assert(world.PlaceActor(npcId, world.GetDefaultSceneId(), {10, 10, 0}));
        assert(world.PlaceActor(
            playerId,
            world.GetDefaultSceneId(),
            {11, 10, 0}));

        assert(world.SetActorSceneCoordinateMovementTarget(npcId, {11, 10, 0}));
        assert(engine.QueuePlayerMoveToSceneCoordinate(playerId, {12, 10, 0}));

        engine.Step();

        assert(world.GetSceneMembership(npcId)->coordinate ==
               (osrssim::SceneCoordinate{10, 10, 0}));
        assert(world.GetSceneMembership(playerId)->coordinate ==
               (osrssim::SceneCoordinate{12, 10, 0}));
        assert(world.GetNpc(npcId)->movementTarget.has_value());

        engine.Step();

        assert(world.GetSceneMembership(npcId)->coordinate ==
               (osrssim::SceneCoordinate{11, 10, 0}));
        assert(world.GetSceneMembership(playerId)->coordinate ==
               (osrssim::SceneCoordinate{12, 10, 0}));
        assert(!world.GetNpc(npcId)->movementTarget.has_value());
    }

    {
        osrssim::Engine engine;
        osrssim::World& world = engine.GetWorld();
        osrssim::ActorId firstNpcId = world.CreateNpc(1, 1, osrssim::CombatComposition{}).value();
        osrssim::ActorId secondNpcId = world.CreateNpc(1, 1, osrssim::CombatComposition{}).value();

        assert(firstNpcId < secondNpcId);
        assert(world.PlaceActor(
            firstNpcId,
            world.GetDefaultSceneId(),
            {10, 10, 0}));
        assert(world.PlaceActor(
            secondNpcId,
            world.GetDefaultSceneId(),
            {12, 10, 0}));

        assert(world.SetActorSceneCoordinateMovementTarget(
            firstNpcId,
            {11, 10, 0}));
        assert(world.SetActorSceneCoordinateMovementTarget(
            secondNpcId,
            {11, 10, 0}));

        engine.Step();

        assert(world.GetSceneMembership(firstNpcId)->coordinate ==
               (osrssim::SceneCoordinate{11, 10, 0}));
        assert(world.GetSceneMembership(secondNpcId)->coordinate ==
               (osrssim::SceneCoordinate{12, 10, 0}));
        assert(!world.GetNpc(firstNpcId)->movementTarget.has_value());
        assert(world.GetNpc(secondNpcId)->movementTarget.has_value());
    }

    {
        osrssim::Engine engine;
        osrssim::World& world = engine.GetWorld();
        osrssim::ActorId playerId = world.CreatePlayer(1, 0, osrssim::CombatComposition{}).value();

        assert(world.PlaceActor(
            playerId,
            world.GetDefaultSceneId(),
            {10, 10, 0}));

        assert(engine.QueuePlayerMoveToSceneCoordinate(playerId, {11, 10, 0}));

        engine.Step();

        assert(world.GetSceneMembership(playerId)->coordinate ==
               (osrssim::SceneCoordinate{10, 10, 0}));
    }

    {
        osrssim::Engine engine;
        osrssim::World& world = engine.GetWorld();
        osrssim::Scene* scene = world.TryGetScene(world.GetDefaultSceneId());
        osrssim::ActorId firstPlayerId = world.CreatePlayer(1, 1, osrssim::CombatComposition{}).value();
        osrssim::ActorId secondPlayerId = world.CreatePlayer(1, 1, osrssim::CombatComposition{}).value();

        assert(scene != nullptr);
        assert(firstPlayerId < secondPlayerId);
        assert(world.PlaceActor(
            firstPlayerId,
            world.GetDefaultSceneId(),
            {10, 10, 0}));
        assert(world.PlaceActor(
            secondPlayerId,
            world.GetDefaultSceneId(),
            {11, 10, 0}));

        assert(engine.QueuePlayerMoveToSceneCoordinate(
            firstPlayerId,
            {11, 10, 0}));
        assert(engine.QueuePlayerMoveToSceneCoordinate(
            secondPlayerId,
            {12, 10, 0}));

        engine.Step();

        assert(world.GetSceneMembership(firstPlayerId)->coordinate ==
               (osrssim::SceneCoordinate{11, 10, 0}));
        assert(world.GetSceneMembership(secondPlayerId)->coordinate ==
               (osrssim::SceneCoordinate{12, 10, 0}));
        assert(!scene->TryGetTile({11, 10, 0})->IsOccupied());
        assert(scene->TryGetTile({12, 10, 0})->IsOccupied());
    }

    {
        osrssim::Engine engine;
        osrssim::World& world = engine.GetWorld();
        osrssim::ActorId playerId = world.CreatePlayer(1, 2, osrssim::CombatComposition{}).value();
        osrssim::ActorId targetId = world.CreateNpc(2, 1, osrssim::CombatComposition{}).value();

        assert(world.PlaceActor(
            playerId,
            world.GetDefaultSceneId(),
            {10, 10, 0}));
        assert(world.PlaceActor(
            targetId,
            world.GetDefaultSceneId(),
            {13, 10, 0}));

        assert(engine.QueuePlayerMoveToActor(playerId, targetId));

        engine.Step();

        assert(world.GetSceneMembership(playerId)->coordinate ==
               (osrssim::SceneCoordinate{12, 10, 0}));
        assert(world.GetPlayer(playerId)->movementTarget.has_value());
    }

    {
        osrssim::Engine engine;
        osrssim::World& world = engine.GetWorld();
        osrssim::ActorId npcId = world.CreateNpc(1, 1, osrssim::CombatComposition{}).value();
        osrssim::ActorId targetId = world.CreatePlayer(1, 1, osrssim::CombatComposition{}).value();

        assert(world.PlaceActor(npcId, world.GetDefaultSceneId(), {10, 10, 0}));
        assert(world.PlaceActor(
            targetId,
            world.GetDefaultSceneId(),
            {12, 10, 0}));

        assert(!engine.QueuePlayerMoveToActor(npcId, targetId));
        assert(!engine.QueuePlayerMoveToActor(targetId, 999));

        engine.Step();

        assert(world.GetSceneMembership(npcId)->coordinate ==
               (osrssim::SceneCoordinate{10, 10, 0}));
        assert(world.GetSceneMembership(targetId)->coordinate ==
               (osrssim::SceneCoordinate{12, 10, 0}));
    }

    {
        osrssim::Engine engine;
        osrssim::World& world = engine.GetWorld();
        osrssim::Scene* scene = world.TryGetScene(world.GetDefaultSceneId());
        osrssim::ActorId playerId = world.CreatePlayer(1, 2, osrssim::CombatComposition{}).value();
        osrssim::ActorId npcId = world.CreateNpc(4, 1, osrssim::CombatComposition{}).value();
        osrssim::CollisionProfile blockingObject;
        blockingObject.blocksMovement = true;
        blockingObject.blocksLineOfSight = true;

        assert(scene != nullptr);
        assert(scene->PlaceGameObject(
            {12, 4, 0},
            200,
            osrssim::CardinalDirection::North,
            3,
            3,
            blockingObject));
        assert(world.PlaceActor(
            playerId,
            world.GetDefaultSceneId(),
            {8, 11, 0}));
        assert(world.PlaceActor(
            npcId,
            world.GetDefaultSceneId(),
            {18, 20, 0}));
        assert(world.SetActorMovementTarget(npcId, playerId));
        assert(!world.CanPlayerUseSceneCoordinateMovementTarget(
            playerId,
            {12, 4, 0}));
        assert(engine.QueuePlayerMoveToSceneCoordinate(playerId, {10, 11, 0}));

        engine.Step();

        assert(engine.GetCurrentTick() == 1);
        assert(world.GetSceneMembership(playerId)->coordinate ==
               (osrssim::SceneCoordinate{10, 11, 0}));
        assert(world.GetNpc(npcId)->movementTarget->actorId == playerId);
        assert(scene->TryGetTile({14, 6, 0})->gameObject.has_value());
        assert(scene->TryGetTile({14, 6, 0})
                   ->HasFlag(osrssim::TileFlag::BlockMovementObject));
    }

    {
        osrssim::Engine engine;
        osrssim::World& world = engine.GetWorld();
        osrssim::ActorId playerId = world.CreatePlayer(1, 2, osrssim::CombatComposition{}).value();
        osrssim::ActorId targetId = world.CreateNpc(1, 1, osrssim::CombatComposition{}).value();
        AttackObservation attack;

        assert(world.PlaceActor(
            playerId,
            world.GetDefaultSceneId(),
            {10, 10, 0}));
        assert(world.PlaceActor(
            targetId,
            world.GetDefaultSceneId(),
            {13, 10, 0}));
        assert(world.SetActorCombatComposition(playerId, CombatCompositionWithWeapon({42, 3, 5})));
        assert(engine.QueuePlayerMoveToActor(playerId, targetId));
        RegisterAttackObservation(engine, attack);

        engine.Step();

        assert(attack.count == 1);
        assert(attack.attackerId == playerId);
        assert(attack.targetId == targetId);
        assert(attack.tick == 1);
        assert(world.GetSceneMembership(playerId)->coordinate ==
               (osrssim::SceneCoordinate{10, 10, 0}));
        assert(world.GetActorAttackTimer(playerId) == 5);
        assert(world.GetPlayer(playerId)->movementTarget.has_value());
        assert(world.GetPlayer(playerId)->movementTarget->actorId == targetId);
    }

    {
        osrssim::Engine engine;
        osrssim::World& world = engine.GetWorld();
        osrssim::ActorId playerId = world.CreatePlayer(1, 2, osrssim::CombatComposition{}).value();
        osrssim::ActorId targetId = world.CreateNpc(1, 1, osrssim::CombatComposition{}).value();
        AttackObservation attack;

        assert(world.PlaceActor(
            playerId,
            world.GetDefaultSceneId(),
            {10, 10, 0}));
        assert(world.PlaceActor(
            targetId,
            world.GetDefaultSceneId(),
            {13, 10, 0}));
        assert(world.SetActorCombatComposition(playerId, CombatCompositionWithWeapon({42, 3, 5})));
        assert(world.SetActorAttackTimer(playerId, 2));
        assert(engine.QueuePlayerMoveToActor(playerId, targetId));
        RegisterAttackObservation(engine, attack);

        engine.Step();

        assert(attack.count == 0);
        assert(world.GetSceneMembership(playerId)->coordinate ==
               (osrssim::SceneCoordinate{10, 10, 0}));
        assert(world.GetActorAttackTimer(playerId) == 1);
        assert(world.GetPlayer(playerId)->movementTarget.has_value());
        assert(world.GetPlayer(playerId)->movementTarget->actorId == targetId);
    }

    {
        osrssim::Engine engine;
        osrssim::World& world = engine.GetWorld();
        osrssim::ActorId npcId = world.CreateNpc(1, 2, osrssim::CombatComposition{}).value();
        osrssim::ActorId targetId = world.CreatePlayer(1, 1, osrssim::CombatComposition{}).value();
        AttackObservation attack;

        assert(world.PlaceActor(npcId, world.GetDefaultSceneId(), {10, 10, 0}));
        assert(world.PlaceActor(
            targetId,
            world.GetDefaultSceneId(),
            {10, 13, 0}));
        assert(world.SetActorCombatComposition(npcId, CombatCompositionWithWeapon({42, 3, 5})));
        assert(world.SetActorMovementTarget(npcId, targetId));
        RegisterAttackObservation(engine, attack);

        engine.Step();

        assert(attack.count == 1);
        assert(attack.attackerId == npcId);
        assert(attack.targetId == targetId);
        assert(world.GetSceneMembership(npcId)->coordinate ==
               (osrssim::SceneCoordinate{10, 10, 0}));
        assert(world.GetNpc(npcId)->movementTarget.has_value());
        assert(world.GetNpc(npcId)->movementTarget->actorId == targetId);
    }

    {
        osrssim::Engine engine;
        osrssim::World& world = engine.GetWorld();
        osrssim::ActorId playerId = world.CreatePlayer(1, 2, osrssim::CombatComposition{}).value();
        osrssim::ActorId targetId = world.CreateNpc(1, 1, osrssim::CombatComposition{}).value();
        AttackObservation attack;

        assert(world.PlaceActor(
            playerId,
            world.GetDefaultSceneId(),
            {10, 10, 0}));
        assert(world.PlaceActor(
            targetId,
            world.GetDefaultSceneId(),
            {12, 10, 0}));
        assert(world.SetActorCombatComposition(playerId, CombatCompositionWithWeapon({42, 5, 5})));
        assert(engine.QueuePlayerMoveToSceneCoordinate(playerId, {12, 10, 0}));
        RegisterAttackObservation(engine, attack);

        engine.Step();

        assert(attack.count == 0);
        assert(world.GetSceneMembership(playerId)->coordinate ==
               (osrssim::SceneCoordinate{12, 10, 0}));
    }

    {
        osrssim::Engine engine;
        osrssim::World& world = engine.GetWorld();
        osrssim::ActorId playerId = world.CreatePlayer(1, 2, osrssim::CombatComposition{}).value();
        osrssim::ActorId targetId = world.CreateNpc(1, 1, osrssim::CombatComposition{}).value();
        AttackObservation attack;

        assert(world.PlaceActor(
            playerId,
            world.GetDefaultSceneId(),
            {10, 10, 0}));
        assert(world.PlaceActor(
            targetId,
            world.GetDefaultSceneId(),
            {14, 10, 0}));
        assert(world.SetActorCombatComposition(playerId, CombatCompositionWithWeapon({42, 3, 5})));
        assert(engine.QueuePlayerMoveToActor(playerId, targetId));
        RegisterAttackObservation(engine, attack);

        engine.Step();

        assert(attack.count == 1);
        assert(attack.attackerId == playerId);
        assert(attack.targetId == targetId);
        assert(attack.tick == 1);
        assert(world.GetSceneMembership(playerId)->coordinate ==
               (osrssim::SceneCoordinate{12, 10, 0}));
        assert(world.GetActorAttackTimer(playerId) == 5);
        assert(world.GetPlayer(playerId)->movementTarget.has_value());
        assert(world.GetPlayer(playerId)->movementTarget->actorId == targetId);
    }

    {
        osrssim::Engine engine;
        osrssim::World& world = engine.GetWorld();
        osrssim::ActorId playerId = world.CreatePlayer(1, 2, osrssim::CombatComposition{}).value();
        osrssim::ActorId targetId = world.CreateNpc(1, 1, osrssim::CombatComposition{}).value();
        AttackObservation attack;

        assert(world.PlaceActor(
            playerId,
            world.GetDefaultSceneId(),
            {10, 10, 0}));
        assert(world.PlaceActor(
            targetId,
            world.GetDefaultSceneId(),
            {14, 10, 0}));
        assert(world.SetActorCombatComposition(playerId, CombatCompositionWithWeapon({42, 3, 5})));
        assert(world.SetActorAttackTimer(playerId, 3));
        assert(engine.QueuePlayerMoveToActor(playerId, targetId));
        RegisterAttackObservation(engine, attack);

        engine.Step();

        assert(attack.count == 0);
        assert(world.GetSceneMembership(playerId)->coordinate ==
               (osrssim::SceneCoordinate{12, 10, 0}));
        assert(world.GetActorAttackTimer(playerId) == 2);
        assert(world.GetPlayer(playerId)->movementTarget.has_value());
        assert(world.GetPlayer(playerId)->movementTarget->actorId == targetId);

        engine.Step();

        assert(attack.count == 0);
        assert(world.GetSceneMembership(playerId)->coordinate ==
               (osrssim::SceneCoordinate{12, 10, 0}));
        assert(world.GetActorAttackTimer(playerId) == 1);
        assert(world.GetPlayer(playerId)->movementTarget.has_value());
        assert(world.GetPlayer(playerId)->movementTarget->actorId == targetId);
    }

    {
        osrssim::Engine engine;
        osrssim::World& world = engine.GetWorld();
        osrssim::ActorId playerId = world.CreatePlayer(1, 2, osrssim::CombatComposition{}).value();
        osrssim::ActorId targetId = world.CreateNpc(1, 1, osrssim::CombatComposition{}).value();
        AttackObservation attack;

        assert(world.PlaceActor(
            playerId,
            world.GetDefaultSceneId(),
            {10, 10, 0}));
        assert(world.PlaceActor(
            targetId,
            world.GetDefaultSceneId(),
            {13, 10, 0}));
        assert(world.SetActorCombatComposition(playerId, CombatCompositionWithWeapon({42, 3, 5})));
        assert(world.SetActorMovementTarget(playerId, targetId));
        RegisterAttackObservation(engine, attack);

        assert(world.UpdatePlayerMovement(playerId));

        assert(attack.count == 0);
        assert(world.GetSceneMembership(playerId)->coordinate ==
               (osrssim::SceneCoordinate{12, 10, 0}));
    }

    {
        osrssim::Engine engine;
        osrssim::World& world = engine.GetWorld();
        osrssim::ActorId targetId = world.CreatePlayer(1, 1, osrssim::CombatComposition{}).value();
        osrssim::ActorId npcId = world.CreateNpc(1, 1, osrssim::CombatComposition{}).value();
        AttackObservation attack;

        assert(world.PlaceActor(
            targetId,
            world.GetDefaultSceneId(),
            {10, 10, 0}));
        assert(world.PlaceActor(
            npcId,
            world.GetDefaultSceneId(),
            {10, 10, 0}));
        assert(world.SetActorMovementTarget(npcId, targetId));
        RegisterAttackObservation(engine, attack);

        engine.Step();

        assert(attack.count == 0);
        assert(world.GetSceneMembership(npcId)->coordinate ==
               (osrssim::SceneCoordinate{9, 10, 0}));
        assert(world.GetActorAttackTimer(npcId) == -1);
        assert(world.GetNpc(npcId)->movementTarget.has_value());
        assert(world.GetNpc(npcId)->movementTarget->actorId == targetId);
    }

    {
        osrssim::Engine engine;
        osrssim::World& world = engine.GetWorld();
        osrssim::ActorId playerId = world.CreatePlayer(1, 0, osrssim::CombatComposition{}).value();
        osrssim::ActorId targetId = world.CreateNpc(1, 1, osrssim::CombatComposition{}).value();
        AttackObservation attack;

        assert(world.PlaceActor(
            playerId,
            world.GetDefaultSceneId(),
            {10, 10, 0}));
        assert(world.PlaceActor(
            targetId,
            world.GetDefaultSceneId(),
            {11, 11, 0}));
        assert(engine.QueuePlayerMoveToActor(playerId, targetId));
        RegisterAttackObservation(engine, attack);

        engine.Step();

        assert(attack.count == 0);
        assert(world.GetSceneMembership(playerId)->coordinate ==
               (osrssim::SceneCoordinate{10, 10, 0}));
        assert(world.GetActorAttackTimer(playerId) == -1);
        assert(world.GetPlayer(playerId)->movementTarget.has_value());
        assert(world.GetPlayer(playerId)->movementTarget->actorId == targetId);
    }

    {
        osrssim::Engine engine;
        osrssim::World& world = engine.GetWorld();
        osrssim::Scene* scene = world.TryGetScene(world.GetDefaultSceneId());
        osrssim::ActorId playerId = world.CreatePlayer(1, 1, osrssim::CombatComposition{}).value();
        osrssim::ActorId targetId = world.CreateNpc(1, 1, osrssim::CombatComposition{}).value();
        AttackObservation attack;

        assert(scene != nullptr);
        assert(world.PlaceActor(
            playerId,
            world.GetDefaultSceneId(),
            {10, 10, 0}));
        assert(world.PlaceActor(
            targetId,
            world.GetDefaultSceneId(),
            {14, 10, 0}));
        assert(world.SetActorCombatComposition(playerId, CombatCompositionWithWeapon({42, 5, 5})));
        scene->TryGetTile({12, 10, 0})
            ->AddFlag(osrssim::TileFlag::BlockLineOfSightFull);
        assert(engine.QueuePlayerMoveToActor(playerId, targetId));
        RegisterAttackObservation(engine, attack);

        engine.Step();

        assert(attack.count == 0);
        assert(world.GetSceneMembership(playerId)->coordinate ==
               (osrssim::SceneCoordinate{11, 10, 0}));
        assert(world.GetActorAttackTimer(playerId) == -1);
        assert(world.GetPlayer(playerId)->movementTarget.has_value());
        assert(world.GetPlayer(playerId)->movementTarget->actorId == targetId);
    }

    {
        osrssim::Engine engine;
        osrssim::World& world = engine.GetWorld();
        osrssim::ActorId playerId = world.CreatePlayer(1, 1, osrssim::CombatComposition{}).value();
        osrssim::ActorId targetId = world.CreateNpc(1, 1, osrssim::CombatComposition{}).value();
        AttackObservation attack;

        assert(world.PlaceActor(
            playerId,
            world.GetDefaultSceneId(),
            {10, 10, 0}));
        assert(world.PlaceActor(
            targetId,
            world.GetDefaultSceneId(),
            {11, 10, 0}));
        assert(engine.QueuePlayerMoveToActor(playerId, targetId));
        assert(world.RemoveActorSceneMembership(playerId));
        RegisterAttackObservation(engine, attack);

        engine.Step();

        assert(attack.count == 0);
        assert(world.GetSceneMembership(playerId) == nullptr);
        assert(world.GetActorAttackTimer(playerId) == -1);
        assert(!world.GetPlayer(playerId)->movementTarget.has_value());
    }

    {
        osrssim::Engine engine;
        osrssim::World& world = engine.GetWorld();
        osrssim::ActorId playerId = world.CreatePlayer(1, 1, osrssim::CombatComposition{}).value();
        osrssim::ActorId targetId = world.CreateNpc(1, 1, osrssim::CombatComposition{}).value();
        AttackObservation attack;

        assert(world.PlaceActor(
            playerId,
            world.GetDefaultSceneId(),
            {10, 10, 0}));
        assert(world.PlaceActor(
            targetId,
            world.GetDefaultSceneId(),
            {11, 10, 0}));
        assert(engine.QueuePlayerMoveToActor(playerId, targetId));
        assert(world.PlaceActor(
            targetId,
            world.GetDefaultSceneId(),
            {11, 10, 1}));
        RegisterAttackObservation(engine, attack);

        engine.Step();

        assert(attack.count == 0);
        assert(world.GetSceneMembership(playerId)->coordinate ==
               (osrssim::SceneCoordinate{10, 10, 0}));
        assert(world.GetActorAttackTimer(playerId) == -1);
        assert(!world.GetPlayer(playerId)->movementTarget.has_value());
    }

    {
        osrssim::Engine engine;
        osrssim::World& world = engine.GetWorld();
        osrssim::ActorId playerId = world.CreatePlayer(1, 1, osrssim::CombatComposition{}).value();
        osrssim::ActorId targetId = world.CreateNpc(1, 1, osrssim::CombatComposition{}).value();
        AttackObservation attack;

        assert(world.PlaceActor(
            playerId,
            world.GetDefaultSceneId(),
            {10, 10, 0}));
        assert(world.PlaceActor(
            targetId,
            world.GetDefaultSceneId(),
            {11, 10, 0}));
        assert(engine.QueuePlayerMoveToActor(playerId, targetId));
        assert(world.RemoveActor(targetId));
        RegisterAttackObservation(engine, attack);

        engine.Step();

        assert(attack.count == 0);
        assert(world.GetSceneMembership(playerId)->coordinate ==
               (osrssim::SceneCoordinate{10, 10, 0}));
        assert(world.GetActorAttackTimer(playerId) == -1);
        assert(!world.GetPlayer(playerId)->movementTarget.has_value());
    }

    {
        osrssim::Engine engine;
        osrssim::World& world = engine.GetWorld();
        osrssim::ActorId playerId = world.CreatePlayer(1, 1, osrssim::CombatComposition{}).value();
        osrssim::ActorId npcId = world.CreateNpc(1, 1, osrssim::CombatComposition{}).value();
        std::vector<osrssim::ActorId> attackOrder;

        assert(world.PlaceActor(
            playerId,
            world.GetDefaultSceneId(),
            {10, 10, 0}));
        assert(world.PlaceActor(npcId, world.GetDefaultSceneId(), {11, 10, 0}));
        assert(world.SetActorMovementTarget(playerId, npcId));
        assert(world.SetActorMovementTarget(npcId, playerId));
        engine.GetCombatService().RegisterGenericAttackCallback(
            [&attackOrder](
                osrssim::World&,
                osrssim::ActorId attacker,
                osrssim::ActorId,
                osrssim::Tick,
                const osrssim::WeaponDefinition&)
            {
                attackOrder.push_back(attacker);
                return true;
            });

        engine.Step();

        assert(attackOrder.size() == 2);
        assert(attackOrder[0] == npcId);
        assert(attackOrder[1] == playerId);
        assert(world.GetActorAttackTimer(npcId) == 4);
        assert(world.GetActorAttackTimer(playerId) == 4);
    }

    {
        osrssim::Engine engine;
        osrssim::World& world = engine.GetWorld();
        const osrssim::ActorId firstPlayerId =
            world.CreatePlayer(1, 0, osrssim::CombatComposition{}).value();
        const osrssim::ActorId firstNpcId =
            world.CreateNpc(1, 0, osrssim::CombatComposition{}).value();
        osrssim::ActorId createdPlayerId = 0;
        osrssim::ActorId createdNpcId = 0;

        assert(world.PlaceActor(
            firstPlayerId,
            world.GetDefaultSceneId(),
            {10, 10, 0}));
        assert(world.PlaceActor(
            firstNpcId,
            world.GetDefaultSceneId(),
            {30, 30, 0}));
        assert(world.GetActorCombatQueue(firstPlayerId)->AddEvent(
            1,
            [&world, &createdPlayerId]()
            {
                createdPlayerId =
                    world.CreatePlayer(1, 1, osrssim::CombatComposition{})
                        .value();
                assert(world.PlaceActor(
                    createdPlayerId,
                    world.GetDefaultSceneId(),
                    {20, 20, 0}));
                assert(world.SetPlayerSceneCoordinateMovementTarget(
                    createdPlayerId,
                    {21, 20, 0}));
            }));
        assert(world.GetActorCombatQueue(firstNpcId)->AddEvent(
            1,
            [&world, &createdNpcId]()
            {
                createdNpcId =
                    world.CreateNpc(1, 1, osrssim::CombatComposition{})
                        .value();
                assert(world.PlaceActor(
                    createdNpcId,
                    world.GetDefaultSceneId(),
                    {40, 40, 0}));
                assert(world.SetActorSceneCoordinateMovementTarget(
                    createdNpcId,
                    {41, 40, 0}));
            }));

        engine.Step();

        assert(createdPlayerId != 0);
        assert(createdNpcId != 0);
        assert(world.GetPlayer(createdPlayerId)->playerIndex == 1);
        assert(world.GetNpc(createdNpcId)->npcIndex == 1);
        assert(world.GetSceneMembership(createdPlayerId)->coordinate ==
               (osrssim::SceneCoordinate{21, 20, 0}));
        assert(world.GetSceneMembership(createdNpcId)->coordinate ==
               (osrssim::SceneCoordinate{41, 40, 0}));
    }

    {
        osrssim::Engine engine;
        osrssim::World& world = engine.GetWorld();
        std::vector<osrssim::ActorId> playerIds;
        playerIds.reserve(65536);

        for (int index = 0; index <= 65535; ++index)
        {
            playerIds.push_back(
                world.CreatePlayer(1, 0, osrssim::CombatComposition{})
                    .value());
        }

        for (int index = 0; index <= 65535; ++index)
        {
            if (index != 1)
            {
                assert(world.RemoveActor(playerIds[index]));
            }
        }

        const osrssim::ActorId cursorPlayerId = playerIds[1];
        osrssim::ActorId wrappedPlayerId = 0;

        assert(world.PlaceActor(
            cursorPlayerId,
            world.GetDefaultSceneId(),
            {10, 10, 0}));
        assert(world.GetActorCombatQueue(cursorPlayerId)->AddEvent(
            1,
            [&world, &wrappedPlayerId]()
            {
                wrappedPlayerId =
                    world.CreatePlayer(1, 1, osrssim::CombatComposition{})
                        .value();
                assert(world.GetPlayer(wrappedPlayerId)->playerIndex == 0);
                assert(world.PlaceActor(
                    wrappedPlayerId,
                    world.GetDefaultSceneId(),
                    {20, 20, 0}));
                assert(world.SetPlayerSceneCoordinateMovementTarget(
                    wrappedPlayerId,
                    {21, 20, 0}));
            }));

        engine.Step();

        assert(wrappedPlayerId != 0);
        assert(world.GetSceneMembership(wrappedPlayerId)->coordinate ==
               (osrssim::SceneCoordinate{20, 20, 0}));

        engine.Step();

        assert(world.GetSceneMembership(wrappedPlayerId)->coordinate ==
               (osrssim::SceneCoordinate{21, 20, 0}));
    }

    {
        osrssim::Engine engine;
        osrssim::World& world = engine.GetWorld();
        const osrssim::ActorId firstPlayerId =
            world.CreatePlayer(1, 0, osrssim::CombatComposition{}).value();
        const osrssim::ActorId secondPlayerId =
            world.CreatePlayer(1, 1, osrssim::CombatComposition{}).value();

        assert(world.PlaceActor(
            firstPlayerId,
            world.GetDefaultSceneId(),
            {10, 10, 0}));
        assert(world.PlaceActor(
            secondPlayerId,
            world.GetDefaultSceneId(),
            {20, 20, 0}));
        assert(world.SetPlayerSceneCoordinateMovementTarget(
            secondPlayerId,
            {21, 20, 0}));
        assert(world.GetActorCombatQueue(firstPlayerId)->AddEvent(
            1,
            [&world, secondPlayerId]()
            {
                assert(world.RemoveActor(secondPlayerId));
            }));

        engine.Step();

        assert(world.GetPlayer(secondPlayerId) == nullptr);
        assert(world.GetSceneMembership(secondPlayerId) == nullptr);
    }

    {
        osrssim::Engine engine;
        osrssim::World& world = engine.GetWorld();

        const osrssim::ActorId playerId =
            engine.CreatePlayer(1, 1, osrssim::CombatComposition{}).value();
        const osrssim::ActorId npcId =
            engine.CreateNpc(1, 1, osrssim::CombatComposition{}).value();

        assert(world.GetPlayer(playerId) != nullptr);
        assert(world.GetNpc(npcId) != nullptr);
        assert(world.GetNpc(npcId)->behaviorId == 0);
        assert(!engine.RemoveNpc(playerId));
        assert(!engine.RemovePlayer(npcId));
        assert(engine.RemovePlayer(playerId));
        assert(engine.RemoveNpc(npcId));
        assert(!engine.RemovePlayer(playerId));
        assert(!engine.RemoveNpc(npcId));
    }

    {
        osrssim::Engine engine;
        osrssim::World& world = engine.GetWorld();
        int sharedDestroyedCount = 0;
        int dedicatedDestroyedCount = 0;

        const osrssim::NpcBehaviorId sharedBehaviorId =
            engine.RegisterNpcBehavior(
                std::make_unique<CountingBehavior>(
                    sharedDestroyedCount,
                    true));
        const osrssim::ActorId firstNpcId =
            engine.CreateNpc(
                1,
                1,
                osrssim::CombatComposition{},
                sharedBehaviorId)
                .value();
        const osrssim::ActorId secondNpcId =
            engine.CreateNpc(
                1,
                1,
                osrssim::CombatComposition{},
                sharedBehaviorId)
                .value();

        assert(world.GetNpc(firstNpcId)->behaviorId == sharedBehaviorId);
        assert(world.GetNpc(secondNpcId)->behaviorId == sharedBehaviorId);
        assert(sharedDestroyedCount == 0);

        assert(engine.SetNpcBehavior(
            firstNpcId,
            std::make_unique<CountingBehavior>(dedicatedDestroyedCount)));
        const osrssim::NpcBehaviorId dedicatedBehaviorId =
            world.GetNpc(firstNpcId)->behaviorId;

        assert(dedicatedBehaviorId != 0);
        assert(dedicatedBehaviorId != sharedBehaviorId);
        assert(engine.GetNpcBehavior(dedicatedBehaviorId) != nullptr);
        assert(engine.SetNpcBehavior(firstNpcId, sharedBehaviorId));
        assert(dedicatedDestroyedCount == 1);
        assert(engine.GetNpcBehavior(dedicatedBehaviorId) == nullptr);

        assert(engine.RemoveNpc(secondNpcId));
        assert(sharedDestroyedCount == 0);
    }

    {
        osrssim::Engine engine;
        osrssim::World& world = engine.GetWorld();
        int destroyedCount = 0;

        const osrssim::NpcBehaviorId behaviorId =
            engine.RegisterNpcBehavior(
                std::make_unique<CountingBehavior>(destroyedCount));
        const osrssim::ActorId firstNpcId =
            engine.CreateNpc(
                1,
                1,
                osrssim::CombatComposition{},
                behaviorId)
                .value();

        bool threw = false;

        try
        {
            engine.CreateNpc(1, 1, osrssim::CombatComposition{}, behaviorId);
        }
        catch (const std::logic_error&)
        {
            threw = true;
        }

        assert(threw);
        assert(world.GetNpcs().size() == 1);
        assert(world.GetNpc(firstNpcId) != nullptr);
        assert(world.GetNpc(firstNpcId)->behaviorId == behaviorId);
        assert(destroyedCount == 0);
        assert(engine.RemoveNpc(firstNpcId));
        assert(destroyedCount == 0);
    }

    {
        osrssim::Engine engine;
        osrssim::World& world = engine.GetWorld();
        int destroyedCount = 0;

        const osrssim::ActorId playerId =
            engine.CreatePlayer(1, 1, osrssim::CombatComposition{}).value();
        const osrssim::ActorId npcId =
            engine.CreateNpc(
                1,
                1,
                osrssim::CombatComposition{},
                std::make_unique<CountingBehavior>(destroyedCount))
                .value();
        const osrssim::NpcBehaviorId dedicatedBehaviorId =
            world.GetNpc(npcId)->behaviorId;

        assert(!engine.SetNpcBehavior(playerId, 0));

        bool invalidIdThrew = false;

        try
        {
            engine.SetNpcBehavior(npcId, 999);
        }
        catch (const std::logic_error&)
        {
            invalidIdThrew = true;
        }

        assert(invalidIdThrew);

        bool nullBehaviorThrew = false;

        try
        {
            engine.RegisterNpcBehavior(nullptr);
        }
        catch (const std::invalid_argument&)
        {
            nullBehaviorThrew = true;
        }

        assert(nullBehaviorThrew);
        assert(dedicatedBehaviorId != 0);
        assert(engine.GetNpcBehavior(dedicatedBehaviorId) != nullptr);
        assert(engine.RemoveNpc(npcId));
        assert(destroyedCount == 1);
        assert(engine.GetNpcBehavior(dedicatedBehaviorId) == nullptr);
    }

    return 0;
}
