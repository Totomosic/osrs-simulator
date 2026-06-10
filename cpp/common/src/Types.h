#pragma once

#include <cstdint>

namespace osrssim
{

using Tick = std::uint64_t;
using EntityId = std::uint64_t;
using Seed = std::uint64_t;

struct Tile
{
    int x = 0;
    int y = 0;
    int plane = 0;

    friend constexpr bool operator==(const Tile&, const Tile&) = default;
};

}  // namespace osrssim
