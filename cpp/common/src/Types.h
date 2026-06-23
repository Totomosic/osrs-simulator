#pragma once

#include <cstdint>

namespace osrssim
{

using Tick = std::uint64_t;
using EntityId = std::uint64_t;
using ActorId = std::uint64_t;
using PlayerIndex = std::uint16_t;
using NpcIndex = std::uint16_t;
using SceneId = std::uint64_t;
using Seed = std::uint64_t;
using WeaponId = std::uint64_t;
using CombatCompositionId = std::uint64_t;
using NpcId = std::uint64_t;
using NpcBehaviorId = std::uint64_t;

struct SceneCoordinate
{
    int x = 0;
    int y = 0;
    int plane = 0;

    friend constexpr bool operator==(const SceneCoordinate&, const SceneCoordinate&) = default;
};

struct ScenePosition
{
    double x = 0.0;
    double y = 0.0;
    int plane = 0;

    friend constexpr bool operator==(const ScenePosition&, const ScenePosition&) = default;
};

struct WeaponDefinition
{
    WeaponId id = 0;
    int range = 1;
    int speed = 4;
    int projectileId = 0;

    friend constexpr bool operator==(const WeaponDefinition&, const WeaponDefinition&) = default;
};

struct ProjectileMetadata
{
    int projectileId = 0;
    ScenePosition source;
    ActorId targetActorId = 0;
    ScenePosition lastKnownTargetCenter;
    int totalTicks = 0;

    friend constexpr bool operator==(const ProjectileMetadata&, const ProjectileMetadata&) = default;
};

struct ProjectileSnapshot
{
    int projectileId = 0;
    ScenePosition source;
    ActorId targetActorId = 0;
    ScenePosition lastKnownTargetCenter;
    int elapsedTicks = 0;
    int totalTicks = 0;

    friend constexpr bool operator==(const ProjectileSnapshot&, const ProjectileSnapshot&) = default;
};

}  // namespace osrssim
