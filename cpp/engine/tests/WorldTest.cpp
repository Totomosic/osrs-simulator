#include "Tile.h"
#include "World.h"

#include <cassert>

int main()
{
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
