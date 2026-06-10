#include "scenario.h"

#include <cassert>

int main()
{
    const osrssim::sim::Scenario scenario("smoke", 42, 5, 3);

    const osrssim::sim::ScenarioSummary summary = osrssim::sim::RunScenario(scenario);

    assert(summary.GetScenarioName() == "smoke");
    assert(summary.GetTrials() == 3);
    assert(summary.GetTicksPerTrial() == 5);
    assert(summary.GetTotalTicks() == 15);

    return 0;
}
