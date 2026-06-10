#pragma once

#include <cstdint>

namespace osrssim
{

using Tick = std::uint64_t;
using EntityId = std::uint64_t;
using Seed = std::uint64_t;

struct SceneCoordinate
{
    int x = 0;
    int y = 0;
    int plane = 0;

    friend constexpr bool operator==(const SceneCoordinate&, const SceneCoordinate&) = default;
};

}  // namespace osrssim
