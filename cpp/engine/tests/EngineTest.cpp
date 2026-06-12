#include "Engine.h"

#include <cassert>

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
        assert(!engine.QueuePlayerMoveToSceneCoordinate(playerId, {10, 11, 0}));

        engine.Step();

        assert(world.GetSceneMembership(playerId)->coordinate ==
               (osrssim::SceneCoordinate{11, 10, 0}));
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

    return 0;
}
