#include "world.h"

#include <cassert>

int main()
{
    osrssim::engine::World world(123);

    assert(world.GetSeed() == 123);
    assert(world.GetTick() == 0);

    world.AdvanceTick();
    assert(world.GetTick() == 1);

    return 0;
}
