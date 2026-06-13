#include "Engine.h"
#include "DevelopmentPlayerChaseScenario.h"
#include "World.h"

#include <emscripten/bind.h>

namespace
{

osrssim::World& GetEngineWorld(osrssim::Engine& engine)
{
    return engine.GetWorld();
}

}  // namespace

EMSCRIPTEN_BINDINGS(osrssim_engine)
{
    emscripten::class_<osrssim::World>("World")
        .constructor<>()
        .function("GetDefaultSceneId", &osrssim::World::GetDefaultSceneId);

    emscripten::class_<osrssim::Engine>("Engine")
        .constructor<>()
        .function("Step", &osrssim::Engine::Step)
        .function("GetCurrentTick", &osrssim::Engine::GetCurrentTick)
        .function(
            "GetWorld",
            &GetEngineWorld,
            emscripten::return_value_policy::reference());

    emscripten::class_<osrssim::DevelopmentPlayerChaseScenario>(
        "DevelopmentPlayerChaseScenario")
        .constructor<>()
        .function("Step", &osrssim::DevelopmentPlayerChaseScenario::Step)
        .function(
            "ClickSceneCoordinate",
            &osrssim::DevelopmentPlayerChaseScenario::ClickSceneCoordinate)
        .function(
            "SetRunning",
            &osrssim::DevelopmentPlayerChaseScenario::SetRunning)
        .function("IsRunning", &osrssim::DevelopmentPlayerChaseScenario::IsRunning)
        .function(
            "WasLastClickBlocked",
            &osrssim::DevelopmentPlayerChaseScenario::WasLastClickBlocked)
        .function(
            "GetSnapshotJson",
            &osrssim::DevelopmentPlayerChaseScenario::GetSnapshotJson)
        .function("GetTick", &osrssim::DevelopmentPlayerChaseScenario::GetTick)
        .function(
            "GetSceneWidth",
            &osrssim::DevelopmentPlayerChaseScenario::GetSceneWidth)
        .function(
            "GetSceneHeight",
            &osrssim::DevelopmentPlayerChaseScenario::GetSceneHeight)
        .function(
            "GetScenePlaneCount",
            &osrssim::DevelopmentPlayerChaseScenario::GetScenePlaneCount)
        .function(
            "GetPlayerId",
            &osrssim::DevelopmentPlayerChaseScenario::GetPlayerId)
        .function("GetNpcId", &osrssim::DevelopmentPlayerChaseScenario::GetNpcId)
        .function(
            "GetPlayerX",
            &osrssim::DevelopmentPlayerChaseScenario::GetPlayerX)
        .function(
            "GetPlayerY",
            &osrssim::DevelopmentPlayerChaseScenario::GetPlayerY)
        .function(
            "GetPlayerPlane",
            &osrssim::DevelopmentPlayerChaseScenario::GetPlayerPlane)
        .function(
            "HasPlayerMovementTarget",
            &osrssim::DevelopmentPlayerChaseScenario::HasPlayerMovementTarget)
        .function(
            "GetPlayerMovementTargetX",
            &osrssim::DevelopmentPlayerChaseScenario::GetPlayerMovementTargetX)
        .function(
            "GetPlayerMovementTargetY",
            &osrssim::DevelopmentPlayerChaseScenario::GetPlayerMovementTargetY)
        .function(
            "GetPlayerMovementTargetPlane",
            &osrssim::DevelopmentPlayerChaseScenario::GetPlayerMovementTargetPlane)
        .function("GetNpcX", &osrssim::DevelopmentPlayerChaseScenario::GetNpcX)
        .function("GetNpcY", &osrssim::DevelopmentPlayerChaseScenario::GetNpcY)
        .function(
            "GetNpcPlane",
            &osrssim::DevelopmentPlayerChaseScenario::GetNpcPlane)
        .function(
            "GetNpcSize",
            &osrssim::DevelopmentPlayerChaseScenario::GetNpcSize)
        .function(
            "HasNpcMovementTarget",
            &osrssim::DevelopmentPlayerChaseScenario::HasNpcMovementTarget)
        .function(
            "GetNpcMovementTargetActorId",
            &osrssim::DevelopmentPlayerChaseScenario::GetNpcMovementTargetActorId)
        .function(
            "GetNpcMovementTargetLabel",
            &osrssim::DevelopmentPlayerChaseScenario::GetNpcMovementTargetLabel)
        .function(
            "IsGameObjectTile",
            &osrssim::DevelopmentPlayerChaseScenario::IsGameObjectTile)
        .function(
            "IsPlayerTile",
            &osrssim::DevelopmentPlayerChaseScenario::IsPlayerTile)
        .function("IsNpcTile", &osrssim::DevelopmentPlayerChaseScenario::IsNpcTile);
}
