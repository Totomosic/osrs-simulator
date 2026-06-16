#pragma once

#include <cstdint>

namespace osrssim
{

using Tick = std::uint64_t;
using EntityId = std::uint64_t;
using ActorId = std::uint64_t;
using SceneId = std::uint64_t;
using Seed = std::uint64_t;
using WeaponId = std::uint64_t;

struct SceneCoordinate
{
    int x = 0;
    int y = 0;
    int plane = 0;

    friend constexpr bool operator==(const SceneCoordinate&, const SceneCoordinate&) = default;
};

struct WeaponDefinition
{
    WeaponId id = 0;
    int range = 1;
    int speed = 4;

    friend constexpr bool operator==(const WeaponDefinition&, const WeaponDefinition&) = default;
};

}  // namespace osrssim
