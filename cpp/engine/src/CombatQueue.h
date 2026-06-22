#pragma once

#include <cstddef>
#include <functional>
#include <vector>

namespace osrssim
{

class CombatQueue
{
private:
    struct CombatEvent
    {
        int ticksRemaining = 0;
        std::function<void()> callback;
    };

    std::vector<CombatEvent> m_Events;

public:
    bool AddEvent(int ticksRemaining, std::function<void()> callback);
    void Process();
    std::size_t GetEventCount() const;
};

}  // namespace osrssim
