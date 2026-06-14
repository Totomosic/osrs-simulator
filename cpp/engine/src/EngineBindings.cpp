#include "Engine.h"
#include "Scene.h"
#include "Tile.h"
#include "World.h"

#include <emscripten/bind.h>
#include <emscripten/val.h>

#include <sstream>

namespace
{

osrssim::World& GetEngineWorld(osrssim::Engine& engine)
{
    return engine.GetWorld();
}

osrssim::Scene* TryGetWorldScene(osrssim::World& world, osrssim::SceneId sceneId)
{
    return world.TryGetScene(sceneId);
}

bool PlaceSceneGameObject(
    osrssim::Scene& scene,
    osrssim::SceneCoordinate coordinate,
    osrssim::EntityId id,
    osrssim::CardinalDirection direction,
    int sizeX,
    int sizeY,
    const osrssim::CollisionProfile& collisionProfile)
{
    return scene.PlaceGameObject(
        coordinate,
        id,
        direction,
        sizeX,
        sizeY,
        collisionProfile);
}

bool IsGameObjectTile(
    const osrssim::Scene& scene,
    osrssim::SceneCoordinate coordinate)
{
    const osrssim::Tile* tile = scene.TryGetTile(coordinate);
    return tile != nullptr && tile->gameObject.has_value();
}

emscripten::val GetTileFlagLabels(
    const osrssim::Scene& scene,
    osrssim::SceneCoordinate coordinate)
{
    const osrssim::Tile* tile = scene.TryGetTile(coordinate);
    emscripten::val labels = emscripten::val::array();
    int nextIndex = 0;

    if (tile == nullptr)
    {
        return labels;
    }

    auto appendFlag = [&](osrssim::TileFlag flag, const char* label)
    {
        if (!tile->HasFlag(flag))
        {
            return;
        }

        labels.set(nextIndex, label);
        ++nextIndex;
    };

    appendFlag(osrssim::TileFlag::Occupied, "Occupied");
    appendFlag(osrssim::TileFlag::BlockMovementNorthWest, "BlockMovementNorthWest");
    appendFlag(osrssim::TileFlag::BlockMovementNorth, "BlockMovementNorth");
    appendFlag(osrssim::TileFlag::BlockMovementNorthEast, "BlockMovementNorthEast");
    appendFlag(osrssim::TileFlag::BlockMovementEast, "BlockMovementEast");
    appendFlag(osrssim::TileFlag::BlockMovementSouthEast, "BlockMovementSouthEast");
    appendFlag(osrssim::TileFlag::BlockMovementSouth, "BlockMovementSouth");
    appendFlag(osrssim::TileFlag::BlockMovementSouthWest, "BlockMovementSouthWest");
    appendFlag(osrssim::TileFlag::BlockMovementWest, "BlockMovementWest");
    appendFlag(osrssim::TileFlag::BlockMovementFull, "BlockMovementFull");
    appendFlag(osrssim::TileFlag::BlockMovementObject, "BlockMovementObject");
    appendFlag(osrssim::TileFlag::BlockMovementFloor, "BlockMovementFloor");
    appendFlag(
        osrssim::TileFlag::BlockMovementFloorDecoration,
        "BlockMovementFloorDecoration");
    appendFlag(osrssim::TileFlag::BlockLineOfSightNorth, "BlockLineOfSightNorth");
    appendFlag(osrssim::TileFlag::BlockLineOfSightEast, "BlockLineOfSightEast");
    appendFlag(osrssim::TileFlag::BlockLineOfSightSouth, "BlockLineOfSightSouth");
    appendFlag(osrssim::TileFlag::BlockLineOfSightWest, "BlockLineOfSightWest");
    appendFlag(osrssim::TileFlag::BlockLineOfSightFull, "BlockLineOfSightFull");

    return labels;
}

void AppendCoordinateJson(
    std::ostringstream& output,
    osrssim::SceneCoordinate coordinate)
{
    output << "{\"x\":" << coordinate.x << ",\"y\":" << coordinate.y
           << ",\"plane\":" << coordinate.plane << "}";
}

void AppendActorMovementTargetJson(
    std::ostringstream& output,
    const std::optional<osrssim::MovementTarget>& movementTarget)
{
    if (!movementTarget.has_value())
    {
        output << "null";
        return;
    }

    if (movementTarget->kind == osrssim::MovementTargetKind::SceneCoordinate)
    {
        output << "{\"kind\":\"SceneCoordinate\",\"coordinate\":";
        AppendCoordinateJson(output, movementTarget->sceneCoordinate);
        output << "}";
        return;
    }

    output << "{\"kind\":\"Actor\",\"actorId\":" << movementTarget->actorId
           << "}";
}

std::string GetActorSnapshotJson(
    const osrssim::World& world,
    osrssim::ActorId actorId)
{
    const osrssim::ActorCore* actor = world.GetActorCore(actorId);
    const osrssim::SceneMembership* membership =
        world.GetSceneMembership(actorId);

    if (actor == nullptr || membership == nullptr)
    {
        return "";
    }

    const osrssim::Player* player = world.GetPlayer(actorId);
    const osrssim::Npc* npc = world.GetNpc(actorId);
    std::ostringstream output;

    output << "{\"id\":" << actorId << ",\"kind\":\""
           << (player != nullptr ? "Player" : "NPC") << "\",\"coordinate\":";
    AppendCoordinateJson(output, membership->coordinate);
    output << ",\"size\":" << actor->size << ",\"speed\":" << actor->speed
           << ",\"movementTarget\":";

    if (player != nullptr)
    {
        AppendActorMovementTargetJson(output, player->movementTarget);
    }
    else if (npc != nullptr)
    {
        AppendActorMovementTargetJson(output, npc->movementTarget);
    }
    else
    {
        output << "null";
    }

    output << "}";
    return output.str();
}

emscripten::val GetActorSnapshot(
    const osrssim::World& world,
    osrssim::ActorId actorId)
{
    const std::string snapshot = GetActorSnapshotJson(world, actorId);

    if (snapshot.empty())
    {
        return emscripten::val::null();
    }

    return emscripten::val(snapshot);
}

}  // namespace

EMSCRIPTEN_BINDINGS(osrssim_engine)
{
    emscripten::value_object<osrssim::SceneCoordinate>("SceneCoordinate")
        .field("x", &osrssim::SceneCoordinate::x)
        .field("y", &osrssim::SceneCoordinate::y)
        .field("plane", &osrssim::SceneCoordinate::plane);

    emscripten::value_object<osrssim::CollisionProfile>("CollisionProfile")
        .field("blocksMovement", &osrssim::CollisionProfile::blocksMovement)
        .field(
            "blocksLineOfSight",
            &osrssim::CollisionProfile::blocksLineOfSight);

    emscripten::enum_<osrssim::CardinalDirection>("CardinalDirection")
        .value("North", osrssim::CardinalDirection::North)
        .value("East", osrssim::CardinalDirection::East)
        .value("South", osrssim::CardinalDirection::South)
        .value("West", osrssim::CardinalDirection::West);

    emscripten::class_<osrssim::Scene>("Scene")
        .function("PlaceGameObject", &PlaceSceneGameObject)
        .function("RemoveGameObject", &osrssim::Scene::RemoveGameObject)
        .function("IsGameObjectTile", &IsGameObjectTile)
        .function("GetTileFlagLabels", &GetTileFlagLabels);

    emscripten::class_<osrssim::World>("World")
        .constructor<>()
        .function("GetDefaultSceneId", &osrssim::World::GetDefaultSceneId)
        .function(
            "TryGetScene",
            &TryGetWorldScene,
            emscripten::return_value_policy::reference(),
            emscripten::allow_raw_pointers())
        .function("CreatePlayer", &osrssim::World::CreatePlayer)
        .function("CreateNpc", &osrssim::World::CreateNpc)
        .function("PlaceActor", &osrssim::World::PlaceActor)
        .function("RemoveActor", &osrssim::World::RemoveActor)
        .function(
            "SetActorMovementTarget",
            &osrssim::World::SetActorMovementTarget)
        .function(
            "SetPlayerSceneCoordinateMovementTarget",
            &osrssim::World::SetPlayerSceneCoordinateMovementTarget)
        .function(
            "CanPlayerUseSceneCoordinateMovementTarget",
            &osrssim::World::CanPlayerUseSceneCoordinateMovementTarget)
        .function("GetActorSnapshot", &GetActorSnapshot);

    emscripten::class_<osrssim::Engine>("Engine")
        .constructor<>()
        .function("Step", &osrssim::Engine::Step)
        .function("GetCurrentTick", &osrssim::Engine::GetCurrentTick)
        .function(
            "GetWorld",
            &GetEngineWorld,
            emscripten::return_value_policy::reference());

}
