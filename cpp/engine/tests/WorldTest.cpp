#include "Tile.h"
#include "World.h"

#include <cassert>

int main()
{
    {
        osrssim::World world;
        osrssim::ActorId playerId = world.CreatePlayer(0, -1);
        osrssim::ActorId npcId = world.CreateNpc(2, 3);

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
        assert(npcActor != nullptr);
        assert(npcActor->id == npcId);
        assert(npcActor->size == 2);
        assert(npcActor->speed == 3);
    }

    {
        osrssim::World world;
        osrssim::ActorId playerId = world.CreatePlayer(1, 2);
        osrssim::ActorId npcId = world.CreateNpc(2, 1);

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

        osrssim::ActorId actorId = world.CreatePlayer(2, 1);

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
        osrssim::ActorId actorId = world.CreatePlayer(1, 1);

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
        osrssim::ActorId actorId = world.CreatePlayer(2, 1);

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
        osrssim::ActorId firstActorId = world.CreatePlayer(1, 1);
        osrssim::ActorId secondActorId = world.CreatePlayer(1, 1);

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
        osrssim::ActorId playerId = world.CreatePlayer(1, 1);
        osrssim::ActorId npcId = world.CreateNpc(1, 1);

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
        osrssim::ActorId actorId = world.CreatePlayer(1, 2);

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
        osrssim::ActorId actorId = world.CreatePlayer(2, 1);

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
        osrssim::ActorId actorId = world.CreatePlayer(2, 1);

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
        osrssim::ActorId actorId = world.CreatePlayer(1, 2);

        assert(scene != nullptr);
        assert(world.PlaceActor(actorId, world.GetDefaultSceneId(), {10, 10, 0}));
        assert(world.MoveActorByDelta(actorId, 2, 1));
        assert(world.GetSceneMembership(actorId)->coordinate ==
               (osrssim::SceneCoordinate{12, 11, 0}));
    }

    {
        osrssim::World world;
        osrssim::Scene* scene = world.TryGetScene(world.GetDefaultSceneId());
        osrssim::ActorId unplacedActorId = world.CreatePlayer(1, 1);
        osrssim::ActorId placedActorId = world.CreatePlayer(1, 1);

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
        osrssim::ActorId stoppedActorId = world.CreatePlayer(1, 0);
        osrssim::ActorId slowActorId = world.CreatePlayer(1, 1);

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
        osrssim::ActorId actorId = world.CreatePlayer(1, 1);

        assert(world.SetActorSpeed(actorId, 3));
        assert(world.GetActorCore(actorId)->speed == 3);

        assert(world.SetActorSpeed(actorId, -4));
        assert(world.GetActorCore(actorId)->speed == 0);

        assert(!world.SetActorSpeed(999, 2));
    }

    {
        osrssim::World world;
        osrssim::ActorId actorId = world.CreatePlayer(1, 1);

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
        osrssim::Scene* scene = world.TryGetScene(world.GetDefaultSceneId());
        osrssim::ActorId actorId = world.CreatePlayer(1, 1);

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
        assert(world.GetPlayer(actorId)->movementTarget.has_value());
    }

    {
        osrssim::World world;
        osrssim::ActorId moverId = world.CreatePlayer(1, 1);
        osrssim::ActorId targetId = world.CreateNpc(1, 1);

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
        osrssim::ActorId moverId = world.CreatePlayer(1, 1);
        osrssim::ActorId targetId = world.CreateNpc(1, 1);

        assert(world.PlaceActor(moverId, world.GetDefaultSceneId(), {10, 10, 0}));
        assert(world.SetActorMovementTarget(moverId, targetId));

        assert(!world.UpdateActorMovement(moverId));
        assert(world.GetSceneMembership(moverId)->coordinate ==
               (osrssim::SceneCoordinate{10, 10, 0}));
        assert(!world.GetPlayer(moverId)->movementTarget.has_value());
    }

    {
        osrssim::World world;
        osrssim::ActorId moverId = world.CreatePlayer(1, 1);
        osrssim::ActorId targetId = world.CreateNpc(1, 1);

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
        osrssim::ActorId moverId = world.CreatePlayer(1, 1);
        osrssim::ActorId targetId = world.CreateNpc(1, 1);

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
        osrssim::ActorId moverId = world.CreateNpc(1, 1);
        osrssim::ActorId targetId = world.CreatePlayer(1, 1);

        assert(world.PlaceActor(moverId, world.GetDefaultSceneId(), {10, 10, 0}));
        assert(world.PlaceActor(targetId, world.GetDefaultSceneId(), {12, 10, 0}));
        assert(world.SetActorMovementTarget(moverId, targetId));
        assert(world.RemoveActorSceneMembership(moverId));

        assert(!world.UpdateActorMovement(moverId));
        assert(!world.GetNpc(moverId)->movementTarget.has_value());
    }

    {
        osrssim::World world;
        osrssim::ActorId actorId = world.CreatePlayer(1, 1);

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
        osrssim::ActorId actorId = world.CreatePlayer(1, 1);

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
        osrssim::ActorId actorId = world.CreatePlayer(2, 2);

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
        osrssim::ActorId movingPlayerId = world.CreatePlayer(1, 1);
        osrssim::ActorId occupyingPlayerId = world.CreatePlayer(1, 1);

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
        osrssim::ActorId npcId = world.CreateNpc(1, 1);
        osrssim::ActorId occupyingPlayerId = world.CreatePlayer(1, 1);

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
        osrssim::ActorId npcId = world.CreateNpc(1, 2);
        osrssim::ActorId occupyingPlayerId = world.CreatePlayer(1, 1);

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
        osrssim::ActorId npcId = world.CreateNpc(1, 1);
        osrssim::ActorId occupyingPlayerId = world.CreatePlayer(1, 1);

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

    return 0;
}
