#pragma once

#include "Scene.h"
#include "Types.h"

namespace osrssim
{

class Engine
{
private:
    Tick m_CurrentTick = 0;
    Scene m_Scene;

public:
    void Step();
    Tick GetCurrentTick() const;
    Scene& GetScene();
    const Scene& GetScene() const;
};

}  // namespace osrssim
