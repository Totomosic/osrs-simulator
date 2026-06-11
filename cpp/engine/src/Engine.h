#pragma once

#include "Types.h"
#include "World.h"

namespace osrssim
{

class Engine
{
private:
    Tick m_CurrentTick = 0;
    World m_World;

public:
    void Step();
    Tick GetCurrentTick() const;
    World& GetWorld();
    const World& GetWorld() const;
    Scene& GetScene();
    const Scene& GetScene() const;
};

}  // namespace osrssim
