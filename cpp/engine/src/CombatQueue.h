#pragma once

#include "Types.h"

#include <cstddef>
#include <functional>
#include <optional>
#include <vector>

namespace osrssim
{

class CombatQueue
{
private:
    struct CombatEvent
    {
        int ticksRemaining = 0;
        int totalTicks = 0;
        std::function<void()> callback;
        std::optional<ProjectileMetadata> projectile;
        std::optional<Tick> createdTick;
    };

    std::vector<CombatEvent> m_Events;

public:
    bool AddEvent(
        int ticksRemaining,
        std::function<void()> callback,
        std::optional<Tick> createdTick = std::nullopt);
    bool AddEvent(
        int ticksRemaining,
        std::function<void()> callback,
        ProjectileMetadata projectile,
        std::optional<Tick> createdTick = std::nullopt);
    void Process(std::optional<Tick> currentTick = std::nullopt);
    std::size_t GetEventCount() const;
    void UpdateProjectileTargetCenter(
        ActorId targetActorId,
        ScenePosition targetCenter);
    std::vector<ProjectileSnapshot> GetProjectileSnapshots() const;
};

}  // namespace osrssim
