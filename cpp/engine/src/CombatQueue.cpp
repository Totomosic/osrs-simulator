#include "CombatQueue.h"

#include <utility>

namespace osrssim
{

bool CombatQueue::AddEvent(int ticksRemaining, std::function<void()> callback)
{
    if (!callback)
    {
        return false;
    }

    m_Events.push_back({ticksRemaining, ticksRemaining, std::move(callback)});

    return true;
}

bool CombatQueue::AddEvent(
    int ticksRemaining,
    std::function<void()> callback,
    ProjectileMetadata projectile)
{
    if (!callback || projectile.projectileId <= 0 || ticksRemaining <= 0)
    {
        return false;
    }

    projectile.totalTicks = ticksRemaining;
    m_Events.push_back(
        {ticksRemaining, ticksRemaining, std::move(callback), projectile});

    return true;
}

void CombatQueue::Process()
{
    std::vector<CombatEvent> eventsToProcess;
    eventsToProcess.swap(m_Events);

    std::vector<CombatEvent> retainedEvents;
    retainedEvents.reserve(eventsToProcess.size() + m_Events.size());

    for (CombatEvent& event : eventsToProcess)
    {
        --event.ticksRemaining;

        if (event.ticksRemaining <= 0)
        {
            event.callback();
        }
        else
        {
            retainedEvents.push_back(std::move(event));
        }
    }

    retainedEvents.insert(
        retainedEvents.end(),
        std::make_move_iterator(m_Events.begin()),
        std::make_move_iterator(m_Events.end()));

    m_Events = std::move(retainedEvents);
}

std::size_t CombatQueue::GetEventCount() const
{
    return m_Events.size();
}

void CombatQueue::UpdateProjectileTargetCenter(
    ActorId targetActorId,
    ScenePosition targetCenter)
{
    for (CombatEvent& event : m_Events)
    {
        if (event.projectile.has_value() &&
            event.projectile->targetActorId == targetActorId)
        {
            event.projectile->lastKnownTargetCenter = targetCenter;
        }
    }
}

std::vector<ProjectileSnapshot> CombatQueue::GetProjectileSnapshots() const
{
    std::vector<ProjectileSnapshot> snapshots;

    for (const CombatEvent& event : m_Events)
    {
        if (!event.projectile.has_value())
        {
            continue;
        }

        ProjectileSnapshot snapshot;
        snapshot.projectileId = event.projectile->projectileId;
        snapshot.source = event.projectile->source;
        snapshot.targetActorId = event.projectile->targetActorId;
        snapshot.lastKnownTargetCenter =
            event.projectile->lastKnownTargetCenter;
        snapshot.elapsedTicks = event.totalTicks - event.ticksRemaining;
        snapshot.totalTicks = event.totalTicks;
        snapshots.push_back(snapshot);
    }

    return snapshots;
}

}  // namespace osrssim
