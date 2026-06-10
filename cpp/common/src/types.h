#pragma once

#include <cstdint>

namespace osrssim
{

using Tick = std::uint64_t;
using EntityId = std::uint64_t;
using Seed = std::uint64_t;

struct Tile
{
    int m_X = 0;
    int m_Y = 0;
    int m_Plane = 0;

    friend constexpr bool operator==(const Tile&, const Tile&) = default;
};

}  // namespace osrssim
