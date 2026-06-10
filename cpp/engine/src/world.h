#pragma once

#include "types.h"

namespace osrssim::engine
{

class World
{
    Seed m_Seed;
    Tick m_Tick = 0;

public:
    explicit World(Seed seed);

    [[nodiscard]] Seed GetSeed() const;
    [[nodiscard]] Tick GetTick() const;

    void AdvanceTick();
};

}  // namespace osrssim::engine
