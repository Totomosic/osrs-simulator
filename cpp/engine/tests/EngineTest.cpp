#include "Engine.h"
#include "DevelopmentPlayerChaseScenario.h"

#include <cassert>
#include <string>

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
        assert(!world.GetNpc(secondNpcId)->movementTarget.has_value());
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
        osrssim::DevelopmentPlayerChaseScenario scenario;

        assert(scenario.GetTick() == 0);
        assert(scenario.GetPlayerX() == 8);
        assert(scenario.GetPlayerY() == 11);
        assert(scenario.GetNpcX() == 18);
        assert(scenario.GetNpcY() == 10);
        assert(scenario.GetNpcSize() == 4);
        assert(scenario.HasNpcMovementTarget());
        assert(scenario.GetNpcMovementTargetActorId() == scenario.GetPlayerId());
        assert(scenario.GetNpcMovementTargetLabel() == "Player #1");
        assert(scenario.IsGameObjectTile(12, 10, 0));
        assert(scenario.IsGameObjectTile(14, 12, 0));

        const std::string initialSnapshot = scenario.GetSnapshotJson();
        assert(initialSnapshot.find("\"name\":\"Player Chase\"") !=
               std::string::npos);
        assert(initialSnapshot.find("\"size\":4") != std::string::npos);
        assert(initialSnapshot.find("\"BlockMovementObject\"") !=
               std::string::npos);

        assert(scenario.ClickSceneCoordinate(10, 11, 0));
        assert(!scenario.WasLastClickBlocked());
        assert(scenario.GetTick() == 0);
        assert(scenario.GetPlayerX() == 8);
        assert(scenario.GetPlayerY() == 11);
        assert(scenario.HasPlayerMovementTarget());
        assert(scenario.GetPlayerMovementTargetX() == 10);
        assert(scenario.GetPlayerMovementTargetY() == 11);
        assert(scenario.GetPlayerMovementTargetPlane() == 0);

        assert(scenario.ClickSceneCoordinate(7, 11, 0));
        assert(!scenario.WasLastClickBlocked());
        assert(scenario.GetPlayerMovementTargetX() == 7);
        assert(scenario.GetPlayerMovementTargetY() == 11);

        assert(!scenario.ClickSceneCoordinate(12, 10, 0));
        assert(scenario.WasLastClickBlocked());
        assert(scenario.GetPlayerMovementTargetX() == 7);
        assert(scenario.GetPlayerMovementTargetY() == 11);

        scenario.Step();
        scenario.Step();

        assert(scenario.GetTick() == 2);
        assert(scenario.GetNpcX() == 16);
        assert(scenario.GetNpcY() == 10);
        assert(scenario.HasNpcMovementTarget());
    }

    return 0;
}
