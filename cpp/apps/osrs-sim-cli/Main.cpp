#include "Engine.h"
#include "Logging.h"

int main(int argc, char**)
{
    osrssim::Log::Init();

    LOG_INFO("osrs-sim-cli starting with {} argument(s)", argc - 1);

    osrssim::Engine engine;
    LOG_INFO("Initial engine tick: {}", engine.GetCurrentTick());

    engine.Step();
    LOG_INFO("Engine tick after one step: {}", engine.GetCurrentTick());

    return 0;
}
