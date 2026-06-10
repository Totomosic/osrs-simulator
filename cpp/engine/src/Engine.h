#pragma once

#include "Types.h"

namespace osrssim
{

class Engine
{
private:
    Tick m_CurrentTick = 0;

public:
    void Step();
    Tick GetCurrentTick() const;
};

}  // namespace osrssim
