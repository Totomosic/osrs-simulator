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

Scene& Engine::GetScene()
{
    return *m_World.TryGetScene(m_World.GetDefaultSceneId());
}

const Scene& Engine::GetScene() const
{
    return *m_World.TryGetScene(m_World.GetDefaultSceneId());
}

}  // namespace osrssim
