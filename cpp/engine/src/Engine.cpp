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

Scene& Engine::GetScene()
{
    return m_Scene;
}

const Scene& Engine::GetScene() const
{
    return m_Scene;
}

}  // namespace osrssim
