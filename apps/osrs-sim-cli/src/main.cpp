#include "scenario.h"

#include <iostream>

int main()
{
    const osrssim::sim::Scenario scenario("smoke", 1, 10, 2);

    const osrssim::sim::ScenarioSummary summary = osrssim::sim::RunScenario(scenario);

    std::cout << "scenario=" << summary.GetScenarioName()
              << " trials=" << summary.GetTrials()
              << " ticks_per_trial=" << summary.GetTicksPerTrial()
              << " total_ticks=" << summary.GetTotalTicks() << '\n';

    return 0;
}
