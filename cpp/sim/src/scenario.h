#pragma once

#include "types.h"

#include <cstdint>
#include <string>

namespace osrssim::sim
{

class Scenario
{
    std::string m_Name;
    Seed m_Seed = 0;
    Tick m_Ticks = 0;
    std::uint32_t m_Trials = 1;

public:
    Scenario(std::string name, Seed seed, Tick ticks, std::uint32_t trials);

    [[nodiscard]] const std::string& GetName() const;
    [[nodiscard]] Seed GetSeed() const;
    [[nodiscard]] Tick GetTicks() const;
    [[nodiscard]] std::uint32_t GetTrials() const;
};

class ScenarioSummary
{
    std::string m_ScenarioName;
    std::uint32_t m_Trials = 0;
    Tick m_TicksPerTrial = 0;
    Tick m_TotalTicks = 0;

public:
    ScenarioSummary(std::string scenarioName, std::uint32_t trials, Tick ticksPerTrial, Tick totalTicks);

    [[nodiscard]] const std::string& GetScenarioName() const;
    [[nodiscard]] std::uint32_t GetTrials() const;
    [[nodiscard]] Tick GetTicksPerTrial() const;
    [[nodiscard]] Tick GetTotalTicks() const;

    void AddTicks(Tick ticks);
};

ScenarioSummary RunScenario(const Scenario& scenario);

}  // namespace osrssim::sim
