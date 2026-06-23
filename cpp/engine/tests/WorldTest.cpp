#include "Tile.h"
#include "World.h"

#include <cassert>
#include <unordered_set>
#include <vector>

namespace
{
osrssim::CombatComposition CombatCompositionWithWeapon(
    osrssim::WeaponDefinition weapon)
{
    osrssim::CombatComposition combatComposition;
    combatComposition.weapon = weapon;
    return combatComposition;
}
}  // namespace

int main()
{
    {
        osrssim::World world;
        const osrssim::ActorId firstPlayerId =
            world.CreatePlayer(1, 1, osrssim::CombatComposition{}).value();
        const osrssim::ActorId secondPlayerId =
            world.CreatePlayer(1, 1, osrssim::CombatComposition{}).value();
        const osrssim::ActorId firstNpcId =
            world.CreateNpc(1, 1, osrssim::CombatComposition{}).value();
        const osrssim::ActorId secondNpcId =
            world.CreateNpc(1, 1, osrssim::CombatComposition{}).value();

        assert(firstPlayerId != secondPlayerId);
        assert(firstNpcId != secondNpcId);
        assert(world.GetPlayer(firstPlayerId)->playerIndex == 0);
        assert(world.GetPlayer(secondPlayerId)->playerIndex == 1);
        assert(world.GetNpc(firstNpcId)->npcIndex == 0);
        assert(world.GetNpc(secondNpcId)->npcIndex == 1);
        assert(world.GetNpc(firstNpcId)->behaviorId == 0);
        assert(world.GetNpc(secondNpcId)->behaviorId == 0);
        assert(world.GetPlayer(firstPlayerId)->actor.id == firstPlayerId);
        assert(world.GetNpc(firstNpcId)->actor.id == firstNpcId);
    }

    {
        osrssim::World world;
        std::vector<osrssim::ActorId> playerIds;
        std::vector<osrssim::ActorId> npcIds;
        playerIds.reserve(65536);
        npcIds.reserve(65536);

        for (int index = 0; index <= 65535; ++index)
        {
            playerIds.push_back(
                world.CreatePlayer(1, 1, osrssim::CombatComposition{}).value());
            npcIds.push_back(
                world.CreateNpc(1, 1, osrssim::CombatComposition{}).value());
        }

        assert(!world.CreatePlayer(1, 1, osrssim::CombatComposition{}).has_value());
        assert(!world.CreateNpc(1, 1, osrssim::CombatComposition{}).has_value());

        std::unordered_set<osrssim::PlayerIndex> playerIndices;
        std::unordered_set<osrssim::NpcIndex> npcIndices;

        for (osrssim::ActorId actorId : playerIds)
        {
            playerIndices.insert(world.GetPlayer(actorId)->playerIndex);
        }

        for (osrssim::ActorId actorId : npcIds)
        {
            npcIndices.insert(world.GetNpc(actorId)->npcIndex);
        }

        assert(playerIndices.size() == 65536);
        assert(npcIndices.size() == 65536);
        assert(world.RemoveActor(playerIds[100]));
        assert(world.RemoveActor(npcIds[100]));

        const osrssim::ActorId wrappedPlayerId =
            world.CreatePlayer(1, 1, osrssim::CombatComposition{}).value();
        const osrssim::ActorId wrappedNpcId =
            world.CreateNpc(1, 1, osrssim::CombatComposition{}).value();

        assert(world.GetPlayer(wrappedPlayerId)->playerIndex == 100);
        assert(world.GetNpc(wrappedNpcId)->npcIndex == 100);
        assert(wrappedPlayerId > playerIds.back());
        assert(wrappedNpcId > npcIds.back());
    }

    {
        osrssim::World world;
        const osrssim::ActorId firstPlayerId =
            world.CreatePlayer(1, 1, osrssim::CombatComposition{}).value();
        const osrssim::ActorId secondPlayerId =
            world.CreatePlayer(1, 1, osrssim::CombatComposition{}).value();

        assert(world.RemoveActor(firstPlayerId));

        const osrssim::ActorId thirdPlayerId =
            world.CreatePlayer(1, 1, osrssim::CombatComposition{}).value();

        assert(world.GetPlayer(secondPlayerId)->playerIndex == 1);
        assert(world.GetPlayer(thirdPlayerId)->playerIndex == 2);
    }

    {
        osrssim::World world;
        osrssim::CombatComposition playerComposition =
            CombatCompositionWithWeapon({12, 4, 6});
        playerComposition.stats.attack = 70;
        playerComposition.bonuses.slashAttack = 15;
        playerComposition.magicBaseMaximumHit = 9;
        osrssim::CombatComposition npcComposition =
            CombatCompositionWithWeapon({13, 5, 7});

        osrssim::ActorId playerId =
            world.CreatePlayer(0, -1, playerComposition).value();
        osrssim::ActorId npcId = world.CreateNpc(2, 3, npcComposition).value();

        assert(playerId != npcId);
        assert(world.GetPlayer(playerId) != nullptr);
        assert(world.GetNpc(playerId) == nullptr);
        assert(world.GetNpc(npcId) != nullptr);
        assert(world.GetPlayer(npcId) == nullptr);
        assert(world.GetSceneMembership(playerId) == nullptr);
        assert(world.GetSceneMembership(npcId) == nullptr);

        const osrssim::ActorCore* playerActor = world.GetActorCore(playerId);
        const osrssim::ActorCore* npcActor = world.GetActorCore(npcId);

        assert(playerActor != nullptr);
        assert(playerActor->id == playerId);
        assert(playerActor->size == 1);
        assert(playerActor->speed == 0);
        assert(playerActor->combatComposition.stats.attack == 70);
        assert(playerActor->combatComposition.bonuses.slashAttack == 15);
        assert(playerActor->combatComposition.magicBaseMaximumHit == 9);
        assert(
            playerActor->combatComposition.weapon ==
            (osrssim::WeaponDefinition{12, 4, 6}));
        assert(npcActor != nullptr);
        assert(npcActor->id == npcId);
        assert(npcActor->size == 2);
        assert(npcActor->speed == 3);
        assert(
            npcActor->combatComposition.weapon ==
            (osrssim::WeaponDefinition{13, 5, 7}));
    }

    {
        osrssim::World world;
        osrssim::ActorId playerId = world.CreatePlayer(1, 1, osrssim::CombatComposition{}).value();
        osrssim::ActorId npcId = world.CreateNpc(1, 1, osrssim::CombatComposition{}).value();

        const osrssim::CombatComposition* playerComposition =
            world.GetActorCombatComposition(playerId);
        const osrssim::CombatComposition* npcComposition =
            world.GetActorCombatComposition(npcId);

        assert(playerComposition != nullptr);
        assert(playerComposition->weapon.id == 0);
        assert(playerComposition->weapon.range == 1);
        assert(playerComposition->weapon.speed == 4);
        assert(npcComposition != nullptr);
        assert(npcComposition->weapon == playerComposition->weapon);
        assert(world.GetActorAttackTimer(playerId) == 0);
        assert(world.GetActorAttackTimer(npcId) == 0);

        assert(world.SetActorAttackTimer(playerId, 3));
        assert(world.SetActorCombatComposition(
            playerId,
            CombatCompositionWithWeapon({42, 7, 5})));
        assert(world.GetActorCombatComposition(playerId)->weapon.id == 42);
        assert(world.GetActorCombatComposition(playerId)->weapon.range == 7);
        assert(world.GetActorCombatComposition(playerId)->weapon.speed == 5);
        assert(world.GetActorAttackTimer(playerId) == 3);

        assert(world.SetActorCombatComposition(
            npcId,
            CombatCompositionWithWeapon({99, 12, 2})));
        assert(world.GetActorCombatComposition(npcId)->weapon.id == 99);
        assert(world.SetActorAttackTimer(npcId, -2));
        assert(world.GetActorAttackTimer(npcId) == -2);

        assert(world.GetActorCombatComposition(999) == nullptr);
        assert(!world.SetActorCombatComposition(
            999,
            CombatCompositionWithWeapon({1, 1, 1})));
        assert(!world.SetActorAttackTimer(999, 1));
    }

    {
        osrssim::World world;
        osrssim::ActorId playerId = world.CreatePlayer(1, 2, osrssim::CombatComposition{}).value();
        osrssim::ActorId npcId = world.CreateNpc(2, 1, osrssim::CombatComposition{}).value();

        assert(world.PlaceActor(
            playerId,
            world.GetDefaultSceneId(),
            {10, 10, 0}));
        assert(world.PlaceActor(npcId, world.GetDefaultSceneId(), {20, 20, 1}));

        const auto& players = world.GetPlayers();
        const auto& npcs = world.GetNpcs();
        const auto& sceneMemberships = world.GetSceneMemberships();

        assert(players.size() == 1);
        assert(npcs.size() == 1);
        assert(sceneMemberships.size() == 2);
        assert(players.at(playerId).actor.id == playerId);
        assert(players.at(playerId).actor.size == 1);
        assert(players.at(playerId).actor.speed == 2);
        assert(npcs.at(npcId).actor.id == npcId);
        assert(npcs.at(npcId).actor.size == 2);
        assert(npcs.at(npcId).actor.speed == 1);
        assert(sceneMemberships.at(playerId).sceneId == world.GetDefaultSceneId());
        assert(sceneMemberships.at(playerId).coordinate ==
               (osrssim::SceneCoordinate{10, 10, 0}));
        assert(sceneMemberships.at(npcId).sceneId == world.GetDefaultSceneId());
        assert(sceneMemberships.at(npcId).coordinate ==
               (osrssim::SceneCoordinate{20, 20, 1}));

        assert(world.RemoveActor(playerId));
        assert(world.GetPlayers().empty());
        assert(world.GetSceneMemberships().size() == 1);
        assert(world.GetSceneMemberships().contains(npcId));
    }

    {
        osrssim::World world;
        osrssim::Scene* scene = world.TryGetScene(world.GetDefaultSceneId());

        assert(scene != nullptr);

        osrssim::ActorId actorId = world.CreatePlayer(2, 1, osrssim::CombatComposition{}).value();

        assert(!world.PlaceActor(
            actorId,
            world.GetDefaultSceneId(),
            {103, 103, 0}));

        scene->TryGetTile({11, 10, 0})
            ->AddFlag(osrssim::TileFlag::BlockMovementObject);

        assert(!world.PlaceActor(actorId, world.GetDefaultSceneId(), {10, 10, 0}));
        assert(world.GetSceneMembership(actorId) == nullptr);
        assert(!scene->TryGetTile({10, 10, 0})->IsOccupied());
        assert(!scene->TryGetTile({10, 11, 0})->IsOccupied());
        assert(!scene->TryGetTile({11, 10, 0})->IsOccupied());
        assert(!scene->TryGetTile({11, 11, 0})->IsOccupied());

        scene->TryGetTile({11, 10, 0})
            ->RemoveFlag(osrssim::TileFlag::BlockMovementObject);

        osrssim::CollisionProfile lineOfSightOnly;
        lineOfSightOnly.blocksLineOfSight = true;

        assert(scene->PlaceGameObject(
            {11, 10, 0},
            100,
            osrssim::CardinalDirection::North,
            lineOfSightOnly));
        assert(world.PlaceActor(actorId, world.GetDefaultSceneId(), {10, 10, 0}));

        const osrssim::SceneMembership* membership =
            world.GetSceneMembership(actorId);

        assert(membership != nullptr);
        assert(membership->coordinate == (osrssim::SceneCoordinate{10, 10, 0}));
        assert(scene->TryGetTile({10, 10, 0})->IsOccupied());
        assert(scene->TryGetTile({10, 11, 0})->IsOccupied());
        assert(scene->TryGetTile({11, 10, 0})->IsOccupied());
        assert(scene->TryGetTile({11, 11, 0})->IsOccupied());
    }

    {
        osrssim::World world;
        osrssim::Scene* scene = world.TryGetScene(world.GetDefaultSceneId());
        osrssim::ActorId actorId = world.CreatePlayer(1, 1, osrssim::CombatComposition{}).value();

        assert(scene != nullptr);
        assert(world.PlaceActor(actorId, world.GetDefaultSceneId(), {10, 10, 0}));

        scene->TryGetTile({20, 20, 0})
            ->AddFlag(osrssim::TileFlag::BlockMovementObject);

        assert(!world.PlaceActor(actorId, world.GetDefaultSceneId(), {20, 20, 0}));
        assert(world.GetSceneMembership(actorId)->coordinate ==
               (osrssim::SceneCoordinate{10, 10, 0}));
        assert(scene->TryGetTile({10, 10, 0})->IsOccupied());
        assert(!scene->TryGetTile({20, 20, 0})->IsOccupied());
    }

    {
        osrssim::World world;
        osrssim::Scene* scene = world.TryGetScene(world.GetDefaultSceneId());
        osrssim::ActorId actorId = world.CreatePlayer(2, 1, osrssim::CombatComposition{}).value();

        assert(scene != nullptr);
        assert(!world.RemoveActorSceneMembership(actorId));
        assert(world.PlaceActor(actorId, world.GetDefaultSceneId(), {10, 10, 0}));

        assert(world.RemoveActorSceneMembership(actorId));
        assert(world.GetPlayer(actorId) != nullptr);
        assert(world.GetSceneMembership(actorId) == nullptr);
        assert(!scene->TryGetTile({10, 10, 0})->IsOccupied());
        assert(!scene->TryGetTile({10, 11, 0})->IsOccupied());
        assert(!scene->TryGetTile({11, 10, 0})->IsOccupied());
        assert(!scene->TryGetTile({11, 11, 0})->IsOccupied());
        assert(!world.RemoveActorSceneMembership(actorId));
    }

    {
        osrssim::World world;
        osrssim::Scene* scene = world.TryGetScene(world.GetDefaultSceneId());
        osrssim::ActorId firstActorId = world.CreatePlayer(1, 1, osrssim::CombatComposition{}).value();
        osrssim::ActorId secondActorId = world.CreatePlayer(1, 1, osrssim::CombatComposition{}).value();

        assert(scene != nullptr);
        assert(world.PlaceActor(firstActorId, world.GetDefaultSceneId(), {10, 10, 0}));
        assert(world.PlaceActor(secondActorId, world.GetDefaultSceneId(), {10, 10, 0}));

        assert(world.RemoveActorSceneMembership(firstActorId));
        assert(world.GetSceneMembership(firstActorId) == nullptr);
        assert(world.GetSceneMembership(secondActorId)->coordinate ==
               (osrssim::SceneCoordinate{10, 10, 0}));
        assert(!scene->TryGetTile({10, 10, 0})->IsOccupied());
    }

    {
        osrssim::World world;
        osrssim::Scene* scene = world.TryGetScene(world.GetDefaultSceneId());
        osrssim::ActorId playerId = world.CreatePlayer(1, 1, osrssim::CombatComposition{}).value();
        osrssim::ActorId npcId = world.CreateNpc(1, 1, osrssim::CombatComposition{}).value();

        assert(scene != nullptr);
        assert(world.PlaceActor(playerId, world.GetDefaultSceneId(), {10, 10, 0}));
        assert(world.PlaceActor(npcId, world.GetDefaultSceneId(), {20, 20, 0}));

        assert(world.RemoveActor(playerId));
        assert(world.GetPlayer(playerId) == nullptr);
        assert(world.GetActorCore(playerId) == nullptr);
        assert(world.GetSceneMembership(playerId) == nullptr);
        assert(!scene->TryGetTile({10, 10, 0})->IsOccupied());
        assert(scene->TryGetTile({20, 20, 0})->IsOccupied());

        assert(world.RemoveActor(npcId));
        assert(world.GetNpc(npcId) == nullptr);
        assert(world.GetActorCore(npcId) == nullptr);
        assert(world.GetSceneMembership(npcId) == nullptr);
        assert(!scene->TryGetTile({20, 20, 0})->IsOccupied());
        assert(!world.RemoveActor(playerId));
        assert(!world.RemoveActor(999));
    }

    {
        osrssim::World world;
        osrssim::Scene* scene = world.TryGetScene(world.GetDefaultSceneId());
        osrssim::ActorId actorId = world.CreatePlayer(1, 2, osrssim::CombatComposition{}).value();

        assert(scene != nullptr);
        assert(world.PlaceActor(actorId, world.GetDefaultSceneId(), {10, 10, 0}));

        osrssim::CollisionProfile lineOfSightOnly;
        lineOfSightOnly.blocksLineOfSight = true;

        assert(scene->PlaceGameObject(
            {12, 10, 0},
            200,
            osrssim::CardinalDirection::North,
            lineOfSightOnly));
        assert(world.MoveActorByDelta(actorId, 2, 0));
        assert(world.GetSceneMembership(actorId)->coordinate ==
               (osrssim::SceneCoordinate{12, 10, 0}));

        assert(world.PlaceActor(actorId, world.GetDefaultSceneId(), {10, 10, 0}));

        scene->TryGetTile({11, 10, 0})
            ->AddFlag(osrssim::TileFlag::BlockMovementObject);

        assert(!world.MoveActorByDelta(actorId, 2, 0));

        const osrssim::SceneMembership* membership =
            world.GetSceneMembership(actorId);

        assert(membership != nullptr);
        assert(membership->coordinate == (osrssim::SceneCoordinate{10, 10, 0}));
        assert(scene->TryGetTile({10, 10, 0})->IsOccupied());
        assert(!scene->TryGetTile({12, 10, 0})->IsOccupied());
    }

    {
        osrssim::World world;
        osrssim::Scene* scene = world.TryGetScene(world.GetDefaultSceneId());
        osrssim::ActorId actorId = world.CreatePlayer(1, 2, osrssim::CombatComposition{}).value();

        assert(scene != nullptr);
        assert(world.PlaceActor(actorId, world.GetDefaultSceneId(), {10, 10, 0}));

        scene->TryGetTile({12, 10, 0})
            ->AddFlag(osrssim::TileFlag::BlockMovementObject);

        assert(world.MoveActorByDelta(actorId, 2, 0));
        assert(world.GetSceneMembership(actorId)->coordinate ==
               (osrssim::SceneCoordinate{11, 10, 0}));
        assert(!scene->TryGetTile({10, 10, 0})->IsOccupied());
        assert(scene->TryGetTile({11, 10, 0})->IsOccupied());
        assert(!scene->TryGetTile({12, 10, 0})->IsOccupied());
    }

    {
        osrssim::World world;
        osrssim::Scene* scene = world.TryGetScene(world.GetDefaultSceneId());
        osrssim::ActorId actorId = world.CreatePlayer(1, 1, osrssim::CombatComposition{}).value();

        assert(scene != nullptr);
        assert(world.PlaceActor(actorId, world.GetDefaultSceneId(), {10, 10, 0}));

        scene->TryGetTile({11, 11, 0})
            ->AddFlag(osrssim::TileFlag::BlockMovementObject);

        assert(world.MoveActorByDelta(actorId, 1, 1));
        assert(world.GetSceneMembership(actorId)->coordinate ==
               (osrssim::SceneCoordinate{11, 10, 0}));
    }

    {
        osrssim::World world;
        osrssim::Scene* scene = world.TryGetScene(world.GetDefaultSceneId());
        osrssim::ActorId actorId = world.CreatePlayer(1, 1, osrssim::CombatComposition{}).value();

        assert(scene != nullptr);
        assert(world.PlaceActor(actorId, world.GetDefaultSceneId(), {10, 10, 0}));

        scene->TryGetTile({9, 11, 0})
            ->AddFlag(osrssim::TileFlag::BlockMovementObject);

        assert(world.MoveActorByDelta(actorId, -1, 1));
        assert(world.GetSceneMembership(actorId)->coordinate ==
               (osrssim::SceneCoordinate{9, 10, 0}));
    }

    {
        osrssim::World world;
        osrssim::Scene* scene = world.TryGetScene(world.GetDefaultSceneId());
        osrssim::ActorId actorId = world.CreatePlayer(2, 1, osrssim::CombatComposition{}).value();

        assert(scene != nullptr);
        assert(world.PlaceActor(actorId, world.GetDefaultSceneId(), {10, 10, 0}));

        scene->TryGetTile({12, 11, 0})
            ->AddFlag(osrssim::TileFlag::BlockMovementFloor);

        assert(!world.MoveActorByDelta(actorId, 1, 0));
        assert(world.GetSceneMembership(actorId)->coordinate ==
               (osrssim::SceneCoordinate{10, 10, 0}));
        assert(scene->TryGetTile({10, 10, 0})->IsOccupied());
        assert(scene->TryGetTile({10, 11, 0})->IsOccupied());
        assert(scene->TryGetTile({11, 10, 0})->IsOccupied());
        assert(scene->TryGetTile({11, 11, 0})->IsOccupied());
        assert(!scene->TryGetTile({12, 10, 0})->IsOccupied());
        assert(!scene->TryGetTile({12, 11, 0})->IsOccupied());
    }

    {
        osrssim::World world;
        osrssim::Scene* scene = world.TryGetScene(world.GetDefaultSceneId());
        osrssim::ActorId actorId = world.CreatePlayer(2, 1, osrssim::CombatComposition{}).value();

        assert(scene != nullptr);
        assert(world.PlaceActor(actorId, world.GetDefaultSceneId(), {10, 10, 0}));

        assert(world.MoveActorByDelta(actorId, 1, 1));
        assert(world.GetSceneMembership(actorId)->coordinate ==
               (osrssim::SceneCoordinate{11, 11, 0}));
        assert(!scene->TryGetTile({10, 10, 0})->IsOccupied());
        assert(scene->TryGetTile({11, 11, 0})->IsOccupied());
        assert(scene->TryGetTile({12, 12, 0})->IsOccupied());
    }

    {
        osrssim::World world;
        osrssim::Scene* scene = world.TryGetScene(world.GetDefaultSceneId());
        osrssim::ActorId actorId = world.CreatePlayer(1, 2, osrssim::CombatComposition{}).value();

        assert(scene != nullptr);
        assert(world.PlaceActor(actorId, world.GetDefaultSceneId(), {10, 10, 0}));
        assert(world.MoveActorByDelta(actorId, 2, 1));
        assert(world.GetSceneMembership(actorId)->coordinate ==
               (osrssim::SceneCoordinate{12, 11, 0}));
    }

    {
        osrssim::World world;
        osrssim::Scene* scene = world.TryGetScene(world.GetDefaultSceneId());
        osrssim::ActorId unplacedActorId = world.CreatePlayer(1, 1, osrssim::CombatComposition{}).value();
        osrssim::ActorId placedActorId = world.CreatePlayer(1, 1, osrssim::CombatComposition{}).value();

        assert(scene != nullptr);
        assert(!world.MoveActorByDelta(999, 1, 0));
        assert(!world.MoveActorByDelta(unplacedActorId, 1, 0));
        assert(world.PlaceActor(
            placedActorId,
            world.GetDefaultSceneId(),
            {10, 10, 0}));

        assert(!world.MoveActorByDelta(placedActorId, 0, 0));
        assert(world.GetSceneMembership(placedActorId)->coordinate ==
               (osrssim::SceneCoordinate{10, 10, 0}));
        assert(scene->TryGetTile({10, 10, 0})->IsOccupied());
    }

    {
        osrssim::World world;
        osrssim::Scene* scene = world.TryGetScene(world.GetDefaultSceneId());
        osrssim::ActorId stoppedActorId = world.CreatePlayer(1, 0, osrssim::CombatComposition{}).value();
        osrssim::ActorId slowActorId = world.CreatePlayer(1, 1, osrssim::CombatComposition{}).value();

        assert(scene != nullptr);
        assert(world.PlaceActor(
            stoppedActorId,
            world.GetDefaultSceneId(),
            {10, 10, 0}));
        assert(world.PlaceActor(
            slowActorId,
            world.GetDefaultSceneId(),
            {20, 20, 0}));

        assert(!world.MoveActorByDelta(stoppedActorId, 1, 0));
        assert(!world.MoveActorByDelta(slowActorId, 2, 0));
        assert(world.GetSceneMembership(stoppedActorId)->coordinate ==
               (osrssim::SceneCoordinate{10, 10, 0}));
        assert(world.GetSceneMembership(slowActorId)->coordinate ==
               (osrssim::SceneCoordinate{20, 20, 0}));
        assert(scene->TryGetTile({10, 10, 0})->IsOccupied());
        assert(scene->TryGetTile({20, 20, 0})->IsOccupied());
        assert(!scene->TryGetTile({11, 10, 0})->IsOccupied());
        assert(!scene->TryGetTile({22, 20, 0})->IsOccupied());
    }

    {
        osrssim::World world;
        osrssim::ActorId actorId = world.CreatePlayer(1, 1, osrssim::CombatComposition{}).value();

        assert(world.SetActorSpeed(actorId, 3));
        assert(world.GetActorCore(actorId)->speed == 3);

        assert(world.SetActorSpeed(actorId, -4));
        assert(world.GetActorCore(actorId)->speed == 0);

        assert(!world.SetActorSpeed(999, 2));
    }

    {
        osrssim::World world;
        osrssim::ActorId actorId = world.CreatePlayer(1, 1, osrssim::CombatComposition{}).value();

        assert(world.PlaceActor(actorId, world.GetDefaultSceneId(), {10, 10, 0}));
        assert(world.SetPlayerSceneCoordinateMovementTarget(
            actorId,
            {12, 10, 0}));
        assert(world.RemoveActorSceneMembership(actorId));

        assert(!world.UpdatePlayerMovement(actorId));
        assert(!world.GetPlayer(actorId)->movementTarget.has_value());
    }

    {
        osrssim::World world;
        osrssim::ActorId playerId = world.CreatePlayer(1, 1, osrssim::CombatComposition{}).value();
        osrssim::ActorId npcId = world.CreateNpc(1, 2, osrssim::CombatComposition{}).value();

        assert(world.PlaceActor(playerId, world.GetDefaultSceneId(), {10, 10, 0}));
        assert(world.PlaceActor(npcId, world.GetDefaultSceneId(), {20, 20, 0}));

        assert(world.SetActorSceneCoordinateMovementTarget(
            playerId,
            {11, 10, 0}));
        assert(world.SetActorSceneCoordinateMovementTarget(
            npcId,
            {22, 20, 0}));
        assert(!world.SetActorSceneCoordinateMovementTarget(
            999,
            {12, 10, 0}));
        assert(world.ClearActorMovementTarget(playerId));
        assert(!world.GetPlayer(playerId)->movementTarget.has_value());
        assert(world.SetActorSceneCoordinateMovementTarget(
            playerId,
            {11, 10, 0}));
        assert(!world.ClearActorMovementTarget(999));

        assert(world.UpdateActorMovement(playerId));
        assert(world.UpdateActorMovement(npcId));
        assert(world.GetSceneMembership(playerId)->coordinate ==
               (osrssim::SceneCoordinate{11, 10, 0}));
        assert(world.GetSceneMembership(npcId)->coordinate ==
               (osrssim::SceneCoordinate{22, 20, 0}));
        assert(!world.GetPlayer(playerId)->movementTarget.has_value());
        assert(!world.GetNpc(npcId)->movementTarget.has_value());
    }

    {
        osrssim::World world;
        osrssim::ActorId actorId = world.CreatePlayer(2, 1, osrssim::CombatComposition{}).value();

        assert(world.PlaceActor(actorId, world.GetDefaultSceneId(), {10, 10, 0}));
        assert(world.SetPlayerSceneCoordinateMovementTarget(
            actorId,
            {11, 10, 0}));

        assert(world.UpdatePlayerMovement(actorId));
        assert(world.GetSceneMembership(actorId)->coordinate ==
               (osrssim::SceneCoordinate{11, 10, 0}));
        assert(!world.GetPlayer(actorId)->movementTarget.has_value());
    }

    {
        osrssim::World world;
        osrssim::ActorId actorId = world.CreateNpc(2, 1, osrssim::CombatComposition{}).value();

        assert(world.PlaceActor(actorId, world.GetDefaultSceneId(), {10, 10, 0}));
        assert(world.SetActorSceneCoordinateMovementTarget(
            actorId,
            {11, 10, 0}));

        assert(world.UpdateActorMovement(actorId));
        assert(world.GetSceneMembership(actorId)->coordinate ==
               (osrssim::SceneCoordinate{11, 10, 0}));
        assert(!world.GetNpc(actorId)->movementTarget.has_value());
    }

    {
        osrssim::World world;
        osrssim::Scene* scene = world.TryGetScene(world.GetDefaultSceneId());
        osrssim::ActorId actorId = world.CreatePlayer(2, 1, osrssim::CombatComposition{}).value();
        osrssim::ActorId occupyingActorId = world.CreatePlayer(1, 1, osrssim::CombatComposition{}).value();

        assert(scene != nullptr);
        assert(world.PlaceActor(actorId, world.GetDefaultSceneId(), {10, 10, 0}));
        assert(world.PlaceActor(
            occupyingActorId,
            world.GetDefaultSceneId(),
            {20, 20, 0}));

        scene->TryGetTile({31, 30, 0})
            ->AddFlag(osrssim::TileFlag::BlockMovementObject);

        assert(!world.CanActorUseSceneCoordinateMovementTarget(
            actorId,
            {103, 102, 0}));
        assert(!world.CanActorUseSceneCoordinateMovementTarget(
            actorId,
            {30, 30, 0}));
        assert(world.CanActorUseSceneCoordinateMovementTarget(
            actorId,
            {20, 20, 0}));
    }

    {
        osrssim::World world;
        osrssim::Scene* scene = world.TryGetScene(world.GetDefaultSceneId());
        osrssim::ActorId actorId = world.CreatePlayer(1, 1, osrssim::CombatComposition{}).value();

        assert(scene != nullptr);
        assert(world.PlaceActor(actorId, world.GetDefaultSceneId(), {10, 10, 0}));
        assert(world.SetPlayerSceneCoordinateMovementTarget(
            actorId,
            {12, 10, 0}));

        scene->TryGetTile({11, 10, 0})
            ->AddFlag(osrssim::TileFlag::BlockMovementObject);

        assert(!world.UpdatePlayerMovement(actorId));
        assert(world.GetSceneMembership(actorId)->coordinate ==
               (osrssim::SceneCoordinate{10, 10, 0}));
        assert(!world.GetPlayer(actorId)->movementTarget.has_value());
    }

    {
        osrssim::World world;
        osrssim::Scene* scene = world.TryGetScene(world.GetDefaultSceneId());
        osrssim::ActorId actorId = world.CreatePlayer(1, 1, osrssim::CombatComposition{}).value();

        assert(scene != nullptr);
        assert(world.PlaceActor(actorId, world.GetDefaultSceneId(), {10, 10, 0}));
        assert(world.SetPlayerSceneCoordinateMovementTarget(
            actorId,
            {11, 11, 0}));

        scene->TryGetTile({11, 11, 0})
            ->AddFlag(osrssim::TileFlag::BlockMovementObject);

        assert(world.UpdatePlayerMovement(actorId));
        assert(world.GetSceneMembership(actorId)->coordinate ==
               (osrssim::SceneCoordinate{11, 10, 0}));
        assert(world.GetPlayer(actorId)->movementTarget.has_value());
        assert(!scene->TryGetTile({10, 10, 0})->IsOccupied());
        assert(scene->TryGetTile({11, 10, 0})->IsOccupied());
        assert(!scene->TryGetTile({10, 11, 0})->IsOccupied());
        assert(!scene->TryGetTile({11, 11, 0})->IsOccupied());
    }

    {
        osrssim::World world;
        osrssim::Scene* scene = world.TryGetScene(world.GetDefaultSceneId());
        osrssim::ActorId actorId = world.CreatePlayer(1, 2, osrssim::CombatComposition{}).value();

        assert(scene != nullptr);
        assert(world.PlaceActor(actorId, world.GetDefaultSceneId(), {10, 10, 0}));
        assert(world.SetPlayerSceneCoordinateMovementTarget(
            actorId,
            {12, 10, 0}));

        scene->TryGetTile({12, 10, 0})
            ->AddFlag(osrssim::TileFlag::BlockMovementObject);

        assert(world.UpdatePlayerMovement(actorId));
        assert(world.GetSceneMembership(actorId)->coordinate ==
               (osrssim::SceneCoordinate{11, 10, 0}));
        assert(world.GetPlayer(actorId)->movementTarget.has_value());
        assert(!scene->TryGetTile({10, 10, 0})->IsOccupied());
        assert(scene->TryGetTile({11, 10, 0})->IsOccupied());
        assert(!scene->TryGetTile({12, 10, 0})->IsOccupied());

        assert(!world.UpdatePlayerMovement(actorId));
        assert(world.GetSceneMembership(actorId)->coordinate ==
               (osrssim::SceneCoordinate{11, 10, 0}));
        assert(!world.GetPlayer(actorId)->movementTarget.has_value());
    }

    {
        osrssim::World world;
        osrssim::ActorId moverId = world.CreatePlayer(1, 1, osrssim::CombatComposition{}).value();
        osrssim::ActorId targetId = world.CreateNpc(1, 1, osrssim::CombatComposition{}).value();

        assert(world.PlaceActor(moverId, world.GetDefaultSceneId(), {10, 10, 0}));
        assert(world.PlaceActor(targetId, world.GetDefaultSceneId(), {12, 10, 0}));
        assert(world.SetActorMovementTarget(moverId, targetId));
        assert(world.RemoveActor(targetId));

        assert(!world.UpdateActorMovement(moverId));
        assert(world.GetSceneMembership(moverId)->coordinate ==
               (osrssim::SceneCoordinate{10, 10, 0}));
        assert(!world.GetPlayer(moverId)->movementTarget.has_value());
    }

    {
        osrssim::World world;
        osrssim::ActorId moverId = world.CreatePlayer(1, 1, osrssim::CombatComposition{}).value();
        osrssim::ActorId targetId = world.CreateNpc(1, 1, osrssim::CombatComposition{}).value();

        assert(world.PlaceActor(moverId, world.GetDefaultSceneId(), {10, 10, 0}));
        assert(world.SetActorMovementTarget(moverId, targetId));

        assert(!world.UpdateActorMovement(moverId));
        assert(world.GetSceneMembership(moverId)->coordinate ==
               (osrssim::SceneCoordinate{10, 10, 0}));
        assert(!world.GetPlayer(moverId)->movementTarget.has_value());
    }

    {
        osrssim::World world;
        osrssim::ActorId moverId = world.CreatePlayer(1, 1, osrssim::CombatComposition{}).value();
        osrssim::ActorId targetId = world.CreateNpc(1, 1, osrssim::CombatComposition{}).value();

        assert(world.PlaceActor(moverId, world.GetDefaultSceneId(), {10, 10, 0}));
        assert(world.PlaceActor(targetId, world.GetDefaultSceneId(), {12, 10, 1}));
        assert(world.SetActorMovementTarget(moverId, targetId));

        assert(!world.UpdateActorMovement(moverId));
        assert(world.GetSceneMembership(moverId)->coordinate ==
               (osrssim::SceneCoordinate{10, 10, 0}));
        assert(!world.GetPlayer(moverId)->movementTarget.has_value());
    }

    {
        osrssim::World world;
        osrssim::Scene* scene = world.TryGetScene(world.GetDefaultSceneId());
        osrssim::ActorId moverId = world.CreatePlayer(1, 1, osrssim::CombatComposition{}).value();
        osrssim::ActorId targetId = world.CreateNpc(1, 1, osrssim::CombatComposition{}).value();

        assert(scene != nullptr);
        assert(world.PlaceActor(moverId, world.GetDefaultSceneId(), {10, 10, 0}));
        assert(world.PlaceActor(targetId, world.GetDefaultSceneId(), {12, 10, 0}));
        assert(world.SetActorMovementTarget(moverId, targetId));

        scene->TryGetTile({11, 10, 0})
            ->AddFlag(osrssim::TileFlag::BlockMovementObject);

        assert(!world.UpdateActorMovement(moverId));
        assert(world.GetSceneMembership(moverId)->coordinate ==
               (osrssim::SceneCoordinate{10, 10, 0}));
        assert(world.GetPlayer(moverId)->movementTarget.has_value());
    }

    {
        osrssim::World world;
        osrssim::ActorId moverId = world.CreateNpc(1, 1, osrssim::CombatComposition{}).value();
        osrssim::ActorId targetId = world.CreatePlayer(1, 1, osrssim::CombatComposition{}).value();

        assert(world.PlaceActor(moverId, world.GetDefaultSceneId(), {10, 10, 0}));
        assert(world.PlaceActor(targetId, world.GetDefaultSceneId(), {11, 11, 0}));
        assert(world.SetActorMovementTarget(moverId, targetId));

        assert(world.UpdateActorMovement(moverId));
        assert(world.GetSceneMembership(moverId)->coordinate ==
               (osrssim::SceneCoordinate{11, 10, 0}));
        assert(world.GetNpc(moverId)->movementTarget.has_value());
    }

    {
        osrssim::World world;
        osrssim::Scene* scene = world.TryGetScene(world.GetDefaultSceneId());
        osrssim::ActorId moverId = world.CreateNpc(1, 1, osrssim::CombatComposition{}).value();
        osrssim::ActorId targetId = world.CreatePlayer(1, 1, osrssim::CombatComposition{}).value();

        assert(scene != nullptr);
        assert(world.PlaceActor(moverId, world.GetDefaultSceneId(), {10, 10, 0}));
        assert(world.PlaceActor(targetId, world.GetDefaultSceneId(), {9, 11, 0}));
        assert(world.SetActorMovementTarget(moverId, targetId));

        scene->TryGetTile({9, 10, 0})
            ->AddFlag(osrssim::TileFlag::BlockMovementObject);
        scene->TryGetTile({10, 11, 0})
            ->AddFlag(osrssim::TileFlag::BlockMovementObject);

        assert(!world.UpdateActorMovement(moverId));
        assert(world.GetSceneMembership(moverId)->coordinate ==
               (osrssim::SceneCoordinate{10, 10, 0}));
        assert(world.GetNpc(moverId)->movementTarget.has_value());
    }

    {
        osrssim::World world;
        osrssim::Scene* scene = world.TryGetScene(world.GetDefaultSceneId());
        osrssim::ActorId moverId = world.CreateNpc(1, 2, osrssim::CombatComposition{}).value();
        osrssim::ActorId targetId = world.CreatePlayer(1, 1, osrssim::CombatComposition{}).value();
        osrssim::ActorId occupyingPlayerId = world.CreatePlayer(1, 1, osrssim::CombatComposition{}).value();

        assert(scene != nullptr);
        assert(world.PlaceActor(moverId, world.GetDefaultSceneId(), {10, 10, 0}));
        assert(world.PlaceActor(targetId, world.GetDefaultSceneId(), {13, 10, 0}));
        assert(world.PlaceActor(
            occupyingPlayerId,
            world.GetDefaultSceneId(),
            {12, 10, 0}));
        assert(world.SetActorMovementTarget(moverId, targetId));

        assert(world.UpdateActorMovement(moverId));
        assert(world.GetSceneMembership(moverId)->coordinate ==
               (osrssim::SceneCoordinate{11, 10, 0}));
        assert(world.GetSceneMembership(targetId)->coordinate ==
               (osrssim::SceneCoordinate{13, 10, 0}));
        assert(world.GetSceneMembership(occupyingPlayerId)->coordinate ==
               (osrssim::SceneCoordinate{12, 10, 0}));
        assert(world.GetNpc(moverId)->movementTarget.has_value());
        assert(!scene->TryGetTile({10, 10, 0})->IsOccupied());
        assert(scene->TryGetTile({11, 10, 0})->IsOccupied());
        assert(scene->TryGetTile({12, 10, 0})->IsOccupied());
        assert(scene->TryGetTile({13, 10, 0})->IsOccupied());
    }

    {
        osrssim::World world;
        osrssim::Scene* scene = world.TryGetScene(world.GetDefaultSceneId());
        osrssim::ActorId moverId = world.CreatePlayer(1, 2, osrssim::CombatComposition{}).value();
        osrssim::ActorId targetId = world.CreateNpc(1, 1, osrssim::CombatComposition{}).value();
        osrssim::ActorId occupyingPlayerId = world.CreatePlayer(1, 1, osrssim::CombatComposition{}).value();

        assert(scene != nullptr);
        assert(world.PlaceActor(moverId, world.GetDefaultSceneId(), {10, 10, 0}));
        assert(world.PlaceActor(targetId, world.GetDefaultSceneId(), {13, 10, 0}));
        assert(world.PlaceActor(
            occupyingPlayerId,
            world.GetDefaultSceneId(),
            {12, 10, 0}));
        assert(world.SetActorMovementTarget(moverId, targetId));

        assert(world.UpdateActorMovement(moverId));
        assert(world.GetSceneMembership(moverId)->coordinate ==
               (osrssim::SceneCoordinate{12, 10, 0}));
        assert(world.GetSceneMembership(targetId)->coordinate ==
               (osrssim::SceneCoordinate{13, 10, 0}));
        assert(world.GetSceneMembership(occupyingPlayerId)->coordinate ==
               (osrssim::SceneCoordinate{12, 10, 0}));
        assert(world.GetPlayer(moverId)->movementTarget.has_value());
        assert(!scene->TryGetTile({10, 10, 0})->IsOccupied());
        assert(scene->TryGetTile({12, 10, 0})->IsOccupied());
        assert(scene->TryGetTile({13, 10, 0})->IsOccupied());
    }

    {
        osrssim::World world;
        osrssim::ActorId moverId = world.CreateNpc(1, 1, osrssim::CombatComposition{}).value();
        osrssim::ActorId targetId = world.CreatePlayer(1, 1, osrssim::CombatComposition{}).value();

        assert(world.PlaceActor(moverId, world.GetDefaultSceneId(), {10, 10, 0}));
        assert(world.PlaceActor(targetId, world.GetDefaultSceneId(), {12, 10, 0}));
        assert(world.SetActorMovementTarget(moverId, targetId));

        assert(world.UpdateActorMovement(moverId));
        assert(world.GetSceneMembership(moverId)->coordinate ==
               (osrssim::SceneCoordinate{11, 10, 0}));
        assert(world.GetNpc(moverId)->movementTarget.has_value());

        assert(world.MoveActorByDelta(targetId, 1, 0));
        assert(world.UpdateActorMovement(moverId));
        assert(world.GetSceneMembership(moverId)->coordinate ==
               (osrssim::SceneCoordinate{12, 10, 0}));
        assert(world.GetSceneMembership(targetId)->coordinate ==
               (osrssim::SceneCoordinate{13, 10, 0}));
        assert(world.GetNpc(moverId)->movementTarget.has_value());
    }

    {
        osrssim::World world;
        osrssim::ActorId moverId = world.CreateNpc(1, 1, osrssim::CombatComposition{}).value();
        osrssim::ActorId targetId = world.CreatePlayer(2, 1, osrssim::CombatComposition{}).value();

        assert(world.PlaceActor(moverId, world.GetDefaultSceneId(), {10, 10, 0}));
        assert(world.PlaceActor(targetId, world.GetDefaultSceneId(), {7, 9, 0}));
        assert(world.SetActorMovementTarget(moverId, targetId));

        assert(world.UpdateActorMovement(moverId));
        assert(world.GetSceneMembership(moverId)->coordinate ==
               (osrssim::SceneCoordinate{9, 9, 0}));
        assert(world.GetSceneMembership(targetId)->coordinate ==
               (osrssim::SceneCoordinate{7, 9, 0}));
        assert(world.GetNpc(moverId)->movementTarget.has_value());
    }

    {
        osrssim::World world;
        osrssim::ActorId moverId = world.CreateNpc(1, 1, osrssim::CombatComposition{}).value();
        osrssim::ActorId targetId = world.CreatePlayer(1, 1, osrssim::CombatComposition{}).value();

        assert(world.PlaceActor(moverId, world.GetDefaultSceneId(), {10, 10, 0}));
        assert(world.PlaceActor(targetId, world.GetDefaultSceneId(), {11, 10, 0}));
        assert(world.SetActorMovementTarget(moverId, targetId));

        assert(!world.UpdateActorMovement(moverId));
        assert(world.GetSceneMembership(moverId)->coordinate ==
               (osrssim::SceneCoordinate{10, 10, 0}));
        assert(world.GetNpc(moverId)->movementTarget.has_value());
    }

    {
        osrssim::World world;
        osrssim::ActorId moverId = world.CreatePlayer(1, 3, osrssim::CombatComposition{}).value();
        osrssim::ActorId targetId = world.CreateNpc(1, 1, osrssim::CombatComposition{}).value();

        assert(world.PlaceActor(moverId, world.GetDefaultSceneId(), {10, 10, 0}));
        assert(world.PlaceActor(targetId, world.GetDefaultSceneId(), {13, 10, 0}));
        assert(world.SetActorMovementTarget(moverId, targetId));

        assert(world.UpdateActorMovement(moverId));
        assert(world.GetSceneMembership(moverId)->coordinate ==
               (osrssim::SceneCoordinate{12, 10, 0}));
        assert(world.GetSceneMembership(targetId)->coordinate ==
               (osrssim::SceneCoordinate{13, 10, 0}));
        assert(world.GetPlayer(moverId)->movementTarget.has_value());
    }

    {
        osrssim::World world;
        osrssim::ActorId moverId = world.CreateNpc(2, 2, osrssim::CombatComposition{}).value();
        osrssim::ActorId targetId = world.CreatePlayer(2, 1, osrssim::CombatComposition{}).value();

        assert(world.PlaceActor(moverId, world.GetDefaultSceneId(), {10, 10, 0}));
        assert(world.PlaceActor(targetId, world.GetDefaultSceneId(), {11, 10, 0}));
        assert(world.SetActorMovementTarget(moverId, targetId));

        assert(world.UpdateActorMovement(moverId));
        assert(world.GetSceneMembership(moverId)->coordinate ==
               (osrssim::SceneCoordinate{10, 9, 0}));
        assert(world.GetSceneMembership(targetId)->coordinate ==
               (osrssim::SceneCoordinate{11, 10, 0}));
        assert(world.GetNpc(moverId)->movementTarget.has_value());
    }

    {
        osrssim::World world;
        osrssim::ActorId moverId = world.CreateNpc(1, 1, osrssim::CombatComposition{}).value();
        osrssim::ActorId targetId = world.CreatePlayer(1, 1, osrssim::CombatComposition{}).value();

        assert(world.PlaceActor(moverId, world.GetDefaultSceneId(), {10, 10, 0}));
        assert(world.PlaceActor(targetId, world.GetDefaultSceneId(), {12, 10, 0}));
        assert(world.SetActorMovementTarget(moverId, targetId));
        assert(world.RemoveActorSceneMembership(moverId));

        assert(!world.UpdateActorMovement(moverId));
        assert(!world.GetNpc(moverId)->movementTarget.has_value());
    }

    {
        osrssim::World world;
        osrssim::ActorId actorId = world.CreatePlayer(1, 1, osrssim::CombatComposition{}).value();

        assert(world.PlaceActor(actorId, world.GetDefaultSceneId(), {10, 10, 0}));
        assert(world.SetPlayerSceneCoordinateMovementTarget(
            actorId,
            {12, 10, 0}));
        assert(world.PlaceActor(actorId, world.GetDefaultSceneId(), {10, 10, 1}));

        assert(!world.UpdatePlayerMovement(actorId));
        assert(world.GetSceneMembership(actorId)->coordinate ==
               (osrssim::SceneCoordinate{10, 10, 1}));
        assert(!world.GetPlayer(actorId)->movementTarget.has_value());
    }

    {
        osrssim::World world;
        osrssim::ActorId actorId = world.CreatePlayer(1, 1, osrssim::CombatComposition{}).value();

        assert(world.PlaceActor(actorId, world.GetDefaultSceneId(), {10, 10, 0}));
        assert(world.SetPlayerSceneCoordinateMovementTarget(
            actorId,
            {osrssim::Scene::Width, 10, 0}));

        assert(!world.UpdatePlayerMovement(actorId));
        assert(world.GetSceneMembership(actorId)->coordinate ==
               (osrssim::SceneCoordinate{10, 10, 0}));
        assert(!world.GetPlayer(actorId)->movementTarget.has_value());
    }

    {
        osrssim::World world;
        osrssim::Scene* scene = world.TryGetScene(world.GetDefaultSceneId());
        osrssim::ActorId actorId = world.CreatePlayer(2, 2, osrssim::CombatComposition{}).value();

        assert(scene != nullptr);
        assert(world.PlaceActor(actorId, world.GetDefaultSceneId(), {10, 10, 0}));

        scene->TryGetTile({11, 11, 0})
            ->AddFlag(osrssim::TileFlag::BlockMovementFull);

        assert(!world.MoveActorByDelta(actorId, 1, 1));
        assert(world.GetSceneMembership(actorId)->coordinate ==
               (osrssim::SceneCoordinate{10, 10, 0}));
    }

    {
        osrssim::World world;
        osrssim::Scene* scene = world.TryGetScene(world.GetDefaultSceneId());
        osrssim::ActorId actorId = world.CreatePlayer(2, 1, osrssim::CombatComposition{}).value();

        assert(scene != nullptr);
        assert(world.PlaceActor(actorId, world.GetDefaultSceneId(), {10, 10, 0}));

        scene->TryGetTile({12, 11, 0})
            ->AddFlag(osrssim::TileFlag::BlockMovementNorth);

        assert(world.MoveActorByDelta(actorId, 1, 1));
        assert(world.GetSceneMembership(actorId)->coordinate ==
               (osrssim::SceneCoordinate{11, 10, 0}));
    }

    {
        osrssim::World world;
        osrssim::Scene* scene = world.TryGetScene(world.GetDefaultSceneId());
        osrssim::ActorId movingPlayerId = world.CreatePlayer(1, 1, osrssim::CombatComposition{}).value();
        osrssim::ActorId occupyingPlayerId = world.CreatePlayer(1, 1, osrssim::CombatComposition{}).value();

        assert(scene != nullptr);
        assert(world.PlaceActor(
            movingPlayerId,
            world.GetDefaultSceneId(),
            {10, 10, 0}));
        assert(world.PlaceActor(
            occupyingPlayerId,
            world.GetDefaultSceneId(),
            {11, 10, 0}));

        assert(world.MoveActorByDelta(movingPlayerId, 1, 0));
        assert(world.GetSceneMembership(movingPlayerId)->coordinate ==
               (osrssim::SceneCoordinate{11, 10, 0}));
        assert(world.GetSceneMembership(occupyingPlayerId)->coordinate ==
               (osrssim::SceneCoordinate{11, 10, 0}));
        assert(!scene->TryGetTile({10, 10, 0})->IsOccupied());
        assert(scene->TryGetTile({11, 10, 0})->IsOccupied());
    }

    {
        osrssim::World world;
        osrssim::Scene* scene = world.TryGetScene(world.GetDefaultSceneId());
        osrssim::ActorId npcId = world.CreateNpc(1, 1, osrssim::CombatComposition{}).value();
        osrssim::ActorId occupyingPlayerId = world.CreatePlayer(1, 1, osrssim::CombatComposition{}).value();

        assert(scene != nullptr);
        assert(world.PlaceActor(npcId, world.GetDefaultSceneId(), {10, 10, 0}));
        assert(world.PlaceActor(
            occupyingPlayerId,
            world.GetDefaultSceneId(),
            {11, 10, 0}));

        assert(!world.MoveActorByDelta(npcId, 1, 0));
        assert(world.GetSceneMembership(npcId)->coordinate ==
               (osrssim::SceneCoordinate{10, 10, 0}));
        assert(world.GetSceneMembership(occupyingPlayerId)->coordinate ==
               (osrssim::SceneCoordinate{11, 10, 0}));
        assert(scene->TryGetTile({10, 10, 0})->IsOccupied());
        assert(scene->TryGetTile({11, 10, 0})->IsOccupied());
    }

    {
        osrssim::World world;
        osrssim::Scene* scene = world.TryGetScene(world.GetDefaultSceneId());
        osrssim::ActorId npcId = world.CreateNpc(1, 2, osrssim::CombatComposition{}).value();
        osrssim::ActorId occupyingPlayerId = world.CreatePlayer(1, 1, osrssim::CombatComposition{}).value();

        assert(scene != nullptr);
        assert(world.PlaceActor(npcId, world.GetDefaultSceneId(), {10, 10, 0}));
        assert(world.PlaceActor(
            occupyingPlayerId,
            world.GetDefaultSceneId(),
            {11, 10, 0}));

        assert(world.MoveActorByDelta(npcId, 2, 0));
        assert(world.GetSceneMembership(npcId)->coordinate ==
               (osrssim::SceneCoordinate{12, 10, 0}));
        assert(!scene->TryGetTile({10, 10, 0})->IsOccupied());
        assert(scene->TryGetTile({11, 10, 0})->IsOccupied());
        assert(scene->TryGetTile({12, 10, 0})->IsOccupied());
    }

    {
        osrssim::World world;
        osrssim::Scene* scene = world.TryGetScene(world.GetDefaultSceneId());
        osrssim::ActorId npcId = world.CreateNpc(1, 1, osrssim::CombatComposition{}).value();
        osrssim::ActorId occupyingPlayerId = world.CreatePlayer(1, 1, osrssim::CombatComposition{}).value();

        assert(scene != nullptr);
        assert(world.PlaceActor(npcId, world.GetDefaultSceneId(), {10, 10, 0}));
        assert(world.PlaceActor(
            occupyingPlayerId,
            world.GetDefaultSceneId(),
            {10, 10, 0}));

        assert(world.MoveActorByDelta(npcId, 1, 0));
        assert(world.GetSceneMembership(npcId)->coordinate ==
               (osrssim::SceneCoordinate{11, 10, 0}));
        assert(world.GetSceneMembership(occupyingPlayerId)->coordinate ==
               (osrssim::SceneCoordinate{10, 10, 0}));
        assert(!scene->TryGetTile({10, 10, 0})->IsOccupied());
        assert(scene->TryGetTile({11, 10, 0})->IsOccupied());
    }

    {
        osrssim::World world;
        osrssim::Scene* scene = world.TryGetScene(world.GetDefaultSceneId());
        osrssim::ActorId npcId = world.CreateNpc(1, 1, osrssim::CombatComposition{}).value();
        osrssim::ActorId occupyingPlayerId = world.CreatePlayer(1, 1, osrssim::CombatComposition{}).value();

        assert(scene != nullptr);
        assert(world.PlaceActor(npcId, world.GetDefaultSceneId(), {10, 10, 0}));
        assert(world.PlaceActor(
            occupyingPlayerId,
            world.GetDefaultSceneId(),
            {11, 10, 0}));
        assert(world.SetActorSceneCoordinateMovementTarget(npcId, {11, 10, 0}));

        assert(!world.UpdateActorMovement(npcId));
        assert(world.GetSceneMembership(npcId)->coordinate ==
               (osrssim::SceneCoordinate{10, 10, 0}));
        assert(world.GetNpc(npcId)->movementTarget.has_value());
        assert(scene->TryGetTile({10, 10, 0})->IsOccupied());
        assert(scene->TryGetTile({11, 10, 0})->IsOccupied());
    }

    {
        osrssim::World world;
        osrssim::Scene* scene = world.TryGetScene(world.GetDefaultSceneId());
        osrssim::ActorId npcId = world.CreateNpc(1, 1, osrssim::CombatComposition{}).value();

        assert(scene != nullptr);
        assert(world.PlaceActor(npcId, world.GetDefaultSceneId(), {10, 10, 0}));
        assert(world.SetActorSceneCoordinateMovementTarget(npcId, {11, 10, 0}));

        scene->TryGetTile({11, 10, 0})
            ->AddFlag(osrssim::TileFlag::BlockMovementObject);

        assert(!world.UpdateActorMovement(npcId));
        assert(world.GetSceneMembership(npcId)->coordinate ==
               (osrssim::SceneCoordinate{10, 10, 0}));
        assert(!world.GetNpc(npcId)->movementTarget.has_value());
        assert(scene->TryGetTile({10, 10, 0})->IsOccupied());
        assert(!scene->TryGetTile({11, 10, 0})->IsOccupied());
    }

    {
        osrssim::World world;
        osrssim::Scene* scene = world.TryGetScene(world.GetDefaultSceneId());
        osrssim::ActorId npcId = world.CreateNpc(2, 1, osrssim::CombatComposition{}).value();
        osrssim::ActorId occupyingPlayerId = world.CreatePlayer(1, 1, osrssim::CombatComposition{}).value();

        assert(scene != nullptr);
        assert(world.PlaceActor(npcId, world.GetDefaultSceneId(), {10, 10, 0}));
        assert(world.PlaceActor(
            occupyingPlayerId,
            world.GetDefaultSceneId(),
            {12, 10, 0}));

        assert(world.MoveActorByDelta(npcId, 1, 1));
        assert(world.GetSceneMembership(npcId)->coordinate ==
               (osrssim::SceneCoordinate{11, 11, 0}));
        assert(world.GetSceneMembership(occupyingPlayerId)->coordinate ==
               (osrssim::SceneCoordinate{12, 10, 0}));
    }

    {
        osrssim::World world;
        osrssim::Scene* scene = world.TryGetScene(world.GetDefaultSceneId());
        osrssim::ActorId npcId = world.CreateNpc(3, 1, osrssim::CombatComposition{}).value();
        osrssim::ActorId playerId = world.CreatePlayer(1, 1, osrssim::CombatComposition{}).value();

        assert(scene != nullptr);

        osrssim::CollisionProfile objectCollision;
        objectCollision.blocksMovement = true;

        assert(scene->PlaceGameObject(
            {0, 1, 0},
            1000,
            osrssim::CardinalDirection::North,
            3,
            3,
            objectCollision));
        assert(world.PlaceActor(npcId, world.GetDefaultSceneId(), {2, 4, 0}));
        assert(world.PlaceActor(playerId, world.GetDefaultSceneId(), {3, 0, 0}));
        assert(world.SetActorMovementTarget(npcId, playerId));

        assert(world.UpdateActorMovement(npcId));
        assert(world.GetSceneMembership(npcId)->coordinate ==
               (osrssim::SceneCoordinate{3, 3, 0}));
    }

    {
        osrssim::World world;
        osrssim::Scene* scene = world.TryGetScene(world.GetDefaultSceneId());
        osrssim::ActorId npcId = world.CreateNpc(2, 2, osrssim::CombatComposition{}).value();

        assert(scene != nullptr);
        assert(world.PlaceActor(npcId, world.GetDefaultSceneId(), {10, 10, 0}));

        scene->TryGetTile({10, 9, 0})
            ->AddFlag(osrssim::TileFlag::BlockMovementObject);
        scene->TryGetTile({11, 8, 0})
            ->AddFlag(osrssim::TileFlag::BlockMovementObject);

        assert(world.MoveActorByDelta(npcId, 2, -2));
        assert(world.GetSceneMembership(npcId)->coordinate ==
               (osrssim::SceneCoordinate{12, 8, 0}));
    }

    {
        osrssim::World world;
        osrssim::Scene* scene = world.TryGetScene(world.GetDefaultSceneId());
        osrssim::ActorId npcId = world.CreateNpc(3, 1, osrssim::CombatComposition{}).value();

        assert(scene != nullptr);

        osrssim::CollisionProfile objectCollision;
        objectCollision.blocksMovement = true;

        assert(scene->PlaceGameObject(
            {0, 1, 0},
            1001,
            osrssim::CardinalDirection::North,
            3,
            3,
            objectCollision));
        scene->TryGetTile({2, 4, 0})
            ->AddFlag(osrssim::TileFlag::BlockMovementSouthEast);

        assert(world.PlaceActor(npcId, world.GetDefaultSceneId(), {2, 4, 0}));

        assert(world.MoveActorByDelta(npcId, 1, -1));
        assert(world.GetSceneMembership(npcId)->coordinate ==
               (osrssim::SceneCoordinate{3, 4, 0}));
    }

    {
        osrssim::World world;
        osrssim::Scene* scene = world.TryGetScene(world.GetDefaultSceneId());
        osrssim::ActorId npcId = world.CreateNpc(3, 1, osrssim::CombatComposition{}).value();

        assert(scene != nullptr);
        assert(world.PlaceActor(npcId, world.GetDefaultSceneId(), {2, 4, 0}));

        scene->TryGetTile({3, 3, 0})
            ->AddFlag(osrssim::TileFlag::BlockMovementObject);

        assert(world.MoveActorByDelta(npcId, 1, -1));
        assert(world.GetSceneMembership(npcId)->coordinate ==
               (osrssim::SceneCoordinate{3, 4, 0}));
    }

    {
        osrssim::World world;
        osrssim::Scene* scene = world.TryGetScene(world.GetDefaultSceneId());
        osrssim::ActorId npcId = world.CreateNpc(1, 1, osrssim::CombatComposition{}).value();
        osrssim::ActorId playerId = world.CreatePlayer(1, 1, osrssim::CombatComposition{}).value();

        assert(scene != nullptr);
        assert(world.PlaceActor(npcId, world.GetDefaultSceneId(), {10, 10, 0}));
        assert(world.PlaceActor(playerId, world.GetDefaultSceneId(), {11, 11, 0}));
        assert(world.SetActorMovementTarget(npcId, playerId));

        scene->TryGetTile({11, 10, 0})
            ->AddFlag(osrssim::TileFlag::BlockMovementObject);

        assert(!world.UpdateActorMovement(npcId));
        assert(world.GetSceneMembership(npcId)->coordinate ==
               (osrssim::SceneCoordinate{10, 10, 0}));
        assert(world.GetNpc(npcId)->movementTarget.has_value());
    }

    {
        osrssim::World world;
        osrssim::ActorId npcId = world.CreateNpc(4, 1, osrssim::CombatComposition{}).value();
        osrssim::ActorId playerId = world.CreatePlayer(1, 1, osrssim::CombatComposition{}).value();

        assert(world.PlaceActor(npcId, world.GetDefaultSceneId(), {10, 10, 0}));
        assert(world.PlaceActor(playerId, world.GetDefaultSceneId(), {12, 11, 0}));
        assert(world.SetActorMovementTarget(npcId, playerId));

        assert(world.UpdateActorMovement(npcId));
        assert(world.GetSceneMembership(npcId)->coordinate ==
               (osrssim::SceneCoordinate{11, 10, 0}));
        assert(world.GetSceneMembership(playerId)->coordinate ==
               (osrssim::SceneCoordinate{12, 11, 0}));
        assert(world.GetNpc(npcId)->movementTarget.has_value());
    }

    {
        osrssim::World world;
        osrssim::ActorId eventOwnerId =
            world.CreatePlayer(1, 1, osrssim::CombatComposition{}).value();
        osrssim::ActorId targetId =
            world.CreateNpc(2, 1, osrssim::CombatComposition{}).value();
        int firstEventCount = 0;
        int secondEventCount = 0;

        assert(world.PlaceActor(
            eventOwnerId,
            world.GetDefaultSceneId(),
            {10, 10, 0}));
        assert(world.PlaceActor(
            targetId,
            world.GetDefaultSceneId(),
            {20, 20, 0}));
        assert(world.QueueActorCombatEvent(
            eventOwnerId,
            2,
            [&firstEventCount]()
            {
                ++firstEventCount;
            },
            osrssim::ProjectileMetadata{
                101,
                {10.5, 10.5, 0},
                targetId,
                world.GetActorFootprintCenter(targetId)}));
        assert(world.QueueActorCombatEvent(
            eventOwnerId,
            3,
            [&secondEventCount]()
            {
                ++secondEventCount;
            },
            osrssim::ProjectileMetadata{
                202,
                {11.5, 10.5, 0},
                targetId,
                world.GetActorFootprintCenter(targetId)}));

        assert(world.MoveActorByDelta(targetId, 1, 0));

        {
            const std::vector<osrssim::ProjectileSnapshot> projectiles =
                world.GetProjectileSnapshots();
            assert(projectiles.size() == 2);
            assert(projectiles[0].projectileId == 101);
            assert(projectiles[0].targetActorId == targetId);
            assert(projectiles[0].lastKnownTargetCenter ==
                   (osrssim::ScenePosition{22.0, 21.0, 0}));
            assert(projectiles[0].elapsedTicks == 0);
            assert(projectiles[0].totalTicks == 2);
            assert(projectiles[1].projectileId == 202);
            assert(projectiles[1].targetActorId == targetId);
            assert(projectiles[1].lastKnownTargetCenter ==
                   (osrssim::ScenePosition{22.0, 21.0, 0}));
            assert(projectiles[1].elapsedTicks == 0);
            assert(projectiles[1].totalTicks == 3);
        }

        assert(world.RemoveActor(targetId));

        world.GetActorCombatQueue(eventOwnerId)->Process();

        {
            const std::vector<osrssim::ProjectileSnapshot> projectiles =
                world.GetProjectileSnapshots();
            assert(projectiles.size() == 2);
            assert(projectiles[0].lastKnownTargetCenter ==
                   (osrssim::ScenePosition{22.0, 21.0, 0}));
            assert(projectiles[0].elapsedTicks == 1);
            assert(projectiles[1].lastKnownTargetCenter ==
                   (osrssim::ScenePosition{22.0, 21.0, 0}));
            assert(projectiles[1].elapsedTicks == 1);
        }

        world.GetActorCombatQueue(eventOwnerId)->Process();

        {
            const std::vector<osrssim::ProjectileSnapshot> projectiles =
                world.GetProjectileSnapshots();
            assert(firstEventCount == 1);
            assert(secondEventCount == 0);
            assert(projectiles.size() == 1);
            assert(projectiles[0].projectileId == 202);
            assert(projectiles[0].lastKnownTargetCenter ==
                   (osrssim::ScenePosition{22.0, 21.0, 0}));
            assert(projectiles[0].elapsedTicks == 2);
        }

        world.GetActorCombatQueue(eventOwnerId)->Process();

        assert(firstEventCount == 1);
        assert(secondEventCount == 1);
        assert(world.GetProjectileSnapshots().empty());
    }

    return 0;
}
