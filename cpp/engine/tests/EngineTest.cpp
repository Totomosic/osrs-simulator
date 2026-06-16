#include "Engine.h"

#include <cassert>

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
        });
}
}  // namespace

int main()
{
    {
        osrssim::Engine engine;
        osrssim::World& world = engine.GetWorld();
        osrssim::ActorId playerId = world.CreatePlayer(1, 1);

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
        osrssim::ActorId playerId = world.CreatePlayer(1, 1);
        osrssim::ActorId npcId = world.CreateNpc(1, 1);
        osrssim::ActorId unplacedNpcId = world.CreateNpc(1, 1);

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
        osrssim::ActorId attackerId = world.CreatePlayer(1, 1);
        osrssim::ActorId targetId = world.CreateNpc(1, 1);
        int genericAttackCount = 0;
        osrssim::ActorId callbackAttackerId = 0;
        osrssim::ActorId callbackTargetId = 0;
        osrssim::Tick callbackTick = 0;
        osrssim::WeaponDefinition callbackWeapon;
        int callbackAttackTimer = 0;

        assert(world.SetActorWeaponDefinition(attackerId, {42, 7, 5}));
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
        assert(callbackAttackTimer == 5);
        assert(world.GetActorAttackTimer(attackerId) == 5);
    }

    {
        osrssim::Engine engine;
        osrssim::World& world = engine.GetWorld();
        osrssim::ActorId attackerId = world.CreatePlayer(1, 1);
        osrssim::ActorId targetId = world.CreateNpc(1, 1);
        int genericAttackCount = 0;
        int weaponAttackCount = 0;

        assert(world.SetActorWeaponDefinition(attackerId, {42, 7, 5}));
        engine.GetCombatService().RegisterGenericAttackCallback(
            [&genericAttackCount](
                osrssim::World&,
                osrssim::ActorId,
                osrssim::ActorId,
                osrssim::Tick,
                const osrssim::WeaponDefinition&)
            {
                ++genericAttackCount;
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
        osrssim::ActorId attackerId = world.CreatePlayer(1, 1);
        osrssim::ActorId targetId = world.CreateNpc(1, 1);

        assert(world.SetActorWeaponDefinition(attackerId, {7, 3, 2}));

        assert(engine.GetCombatService().DispatchAttack(
            world,
            attackerId,
            targetId,
            1));

        assert(world.GetActorAttackTimer(attackerId) == 2);
        assert(!engine.GetCombatService().DispatchAttack(
            world,
            attackerId,
            999,
            1));
    }

    {
        osrssim::Engine engine;
        osrssim::World& world = engine.GetWorld();
        osrssim::ActorId playerId = world.CreatePlayer(1, 1);

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
        osrssim::ActorId playerId = world.CreatePlayer(1, 1);

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
        osrssim::ActorId npcId = world.CreateNpc(1, 1);

        assert(world.PlaceActor(npcId, world.GetDefaultSceneId(), {10, 10, 0}));
        assert(!engine.QueuePlayerMoveToSceneCoordinate(npcId, {11, 10, 0}));

        engine.Step();

        assert(world.GetSceneMembership(npcId)->coordinate ==
               (osrssim::SceneCoordinate{10, 10, 0}));
    }

    {
        osrssim::Engine engine;
        osrssim::World& world = engine.GetWorld();
        osrssim::ActorId playerId = world.CreatePlayer(1, 1);

        assert(engine.QueuePlayerMoveToSceneCoordinate(playerId, {11, 10, 0}));

        engine.Step();

        assert(world.GetSceneMembership(playerId) == nullptr);
        assert(!world.GetPlayer(playerId)->movementTarget.has_value());
    }

    {
        osrssim::Engine engine;
        osrssim::World& world = engine.GetWorld();
        osrssim::ActorId npcId = world.CreateNpc(1, 1);
        osrssim::ActorId playerId = world.CreatePlayer(1, 1);

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
        osrssim::ActorId firstNpcId = world.CreateNpc(1, 1);
        osrssim::ActorId secondNpcId = world.CreateNpc(1, 1);

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
        osrssim::ActorId playerId = world.CreatePlayer(1, 0);

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
        osrssim::ActorId firstPlayerId = world.CreatePlayer(1, 1);
        osrssim::ActorId secondPlayerId = world.CreatePlayer(1, 1);

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
        osrssim::ActorId playerId = world.CreatePlayer(1, 2);
        osrssim::ActorId targetId = world.CreateNpc(2, 1);

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
        osrssim::ActorId npcId = world.CreateNpc(1, 1);
        osrssim::ActorId targetId = world.CreatePlayer(1, 1);

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
        osrssim::ActorId playerId = world.CreatePlayer(1, 2);
        osrssim::ActorId npcId = world.CreateNpc(4, 1);
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
        osrssim::ActorId playerId = world.CreatePlayer(1, 2);
        osrssim::ActorId targetId = world.CreateNpc(1, 1);
        AttackObservation attack;

        assert(world.PlaceActor(
            playerId,
            world.GetDefaultSceneId(),
            {10, 10, 0}));
        assert(world.PlaceActor(
            targetId,
            world.GetDefaultSceneId(),
            {13, 10, 0}));
        assert(world.SetActorWeaponDefinition(playerId, {42, 3, 5}));
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
        osrssim::ActorId playerId = world.CreatePlayer(1, 2);
        osrssim::ActorId targetId = world.CreateNpc(1, 1);
        AttackObservation attack;

        assert(world.PlaceActor(
            playerId,
            world.GetDefaultSceneId(),
            {10, 10, 0}));
        assert(world.PlaceActor(
            targetId,
            world.GetDefaultSceneId(),
            {13, 10, 0}));
        assert(world.SetActorWeaponDefinition(playerId, {42, 3, 5}));
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
        osrssim::ActorId npcId = world.CreateNpc(1, 2);
        osrssim::ActorId targetId = world.CreatePlayer(1, 1);
        AttackObservation attack;

        assert(world.PlaceActor(npcId, world.GetDefaultSceneId(), {10, 10, 0}));
        assert(world.PlaceActor(
            targetId,
            world.GetDefaultSceneId(),
            {10, 13, 0}));
        assert(world.SetActorWeaponDefinition(npcId, {42, 3, 5}));
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
        osrssim::ActorId playerId = world.CreatePlayer(1, 2);
        osrssim::ActorId targetId = world.CreateNpc(1, 1);
        AttackObservation attack;

        assert(world.PlaceActor(
            playerId,
            world.GetDefaultSceneId(),
            {10, 10, 0}));
        assert(world.PlaceActor(
            targetId,
            world.GetDefaultSceneId(),
            {12, 10, 0}));
        assert(world.SetActorWeaponDefinition(playerId, {42, 5, 5}));
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
        osrssim::ActorId playerId = world.CreatePlayer(1, 2);
        osrssim::ActorId targetId = world.CreateNpc(1, 1);
        AttackObservation attack;

        assert(world.PlaceActor(
            playerId,
            world.GetDefaultSceneId(),
            {10, 10, 0}));
        assert(world.PlaceActor(
            targetId,
            world.GetDefaultSceneId(),
            {13, 10, 0}));
        assert(world.SetActorWeaponDefinition(playerId, {42, 3, 5}));
        assert(world.SetActorMovementTarget(playerId, targetId));
        RegisterAttackObservation(engine, attack);

        assert(world.UpdatePlayerMovement(playerId));

        assert(attack.count == 0);
        assert(world.GetSceneMembership(playerId)->coordinate ==
               (osrssim::SceneCoordinate{12, 10, 0}));
    }

    return 0;
}
