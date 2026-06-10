#include "world.h"

namespace osrssim::engine
{

World::World(Seed seed)
    : m_Seed(seed)
{
}

Seed World::GetSeed() const
{
    return m_Seed;
}

Tick World::GetTick() const
{
    return m_Tick;
}

void World::AdvanceTick()
{
    ++m_Tick;
}

}  // namespace osrssim::engine
