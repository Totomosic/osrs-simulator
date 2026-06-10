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

}  // namespace osrssim
