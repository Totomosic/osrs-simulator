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
        osrssim::ActorId actorId = world.CreatePlayer(2, 2);

        assert(scene != nullptr);
        assert(world.PlaceActor(actorId, world.GetDefaultSceneId(), {10, 10, 0}));

        scene->TryGetTile({11, 11, 0})
            ->AddFlag(osrssim::TileFlag::BlockMovementFull);

        assert(!world.MoveActorByDelta(actorId, 1, 1));
        assert(world.GetSceneMembership(actorId)->coordinate ==
               (osrssim::SceneCoordinate{10, 10, 0}));
    }

    return 0;
}
