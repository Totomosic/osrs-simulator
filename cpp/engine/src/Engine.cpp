#include "Engine.h"

namespace osrssim
{

void Engine::Step()
{
    ++m_CurrentTick;
}

Tick Engine::GetCurrentTick() const
{
    return m_CurrentTick;
}

World& Engine::GetWorld()
{
    return m_World;
}

const World& Engine::GetWorld() const
{
    return m_World;
}

}  // namespace osrssim
