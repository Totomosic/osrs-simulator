#include "Engine.h"
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
}
