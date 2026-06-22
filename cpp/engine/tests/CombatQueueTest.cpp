#include "CombatQueue.h"

#include <cassert>
#include <vector>

int main()
{
    {
        osrssim::CombatQueue queue;
        std::vector<int> fired;

        assert(queue.AddEvent(
            3,
            [&fired]()
            {
                fired.push_back(1);
            }));
        assert(queue.AddEvent(
            1,
            [&fired]()
            {
                fired.push_back(2);
            }));
        assert(queue.AddEvent(
            1,
            [&fired]()
            {
                fired.push_back(3);
            }));

        queue.Process();

        assert((fired == std::vector<int>{2, 3}));
        assert(queue.GetEventCount() == 1);

        queue.Process();

        assert((fired == std::vector<int>{2, 3}));
        assert(queue.GetEventCount() == 1);

        queue.Process();

        assert((fired == std::vector<int>{2, 3, 1}));
        assert(queue.GetEventCount() == 0);
    }

    {
        osrssim::CombatQueue queue;
        std::vector<int> fired;

        assert(queue.AddEvent(
            1,
            [&queue, &fired]()
            {
                fired.push_back(1);
                assert(queue.AddEvent(
                    1,
                    [&fired]()
                    {
                        fired.push_back(2);
                    }));
            }));

        queue.Process();

        assert((fired == std::vector<int>{1}));
        assert(queue.GetEventCount() == 1);

        queue.Process();

        assert((fired == std::vector<int>{1, 2}));
        assert(queue.GetEventCount() == 0);
    }

    {
        osrssim::CombatQueue queue;

        assert(!queue.AddEvent(1, nullptr));
    }

    return 0;
}
