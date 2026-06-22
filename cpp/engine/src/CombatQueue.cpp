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

    m_Events.push_back({ticksRemaining, std::move(callback)});

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

}  // namespace osrssim
