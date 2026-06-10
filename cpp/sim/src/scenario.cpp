#include "scenario.h"

#include "world.h"

#include <utility>

namespace osrssim::sim
{

Scenario::Scenario(std::string name, Seed seed, Tick ticks, std::uint32_t trials)
    : m_Name(std::move(name)),
      m_Seed(seed),
      m_Ticks(ticks),
      m_Trials(trials)
{
}

const std::string& Scenario::GetName() const
{
    return m_Name;
}

Seed Scenario::GetSeed() const
{
    return m_Seed;
}

Tick Scenario::GetTicks() const
{
    return m_Ticks;
}

std::uint32_t Scenario::GetTrials() const
{
    return m_Trials;
}

ScenarioSummary::ScenarioSummary(
    std::string scenarioName,
    std::uint32_t trials,
    Tick ticksPerTrial,
    Tick totalTicks)
    : m_ScenarioName(std::move(scenarioName)),
      m_Trials(trials),
      m_TicksPerTrial(ticksPerTrial),
      m_TotalTicks(totalTicks)
{
}

const std::string& ScenarioSummary::GetScenarioName() const
{
    return m_ScenarioName;
}

std::uint32_t ScenarioSummary::GetTrials() const
{
    return m_Trials;
}

Tick ScenarioSummary::GetTicksPerTrial() const
{
    return m_TicksPerTrial;
}

Tick ScenarioSummary::GetTotalTicks() const
{
    return m_TotalTicks;
}

void ScenarioSummary::AddTicks(Tick ticks)
{
    m_TotalTicks += ticks;
}

ScenarioSummary RunScenario(const Scenario& scenario)
{
    ScenarioSummary summary(
        scenario.GetName(),
        scenario.GetTrials(),
        scenario.GetTicks(),
        0);

    for (std::uint32_t trial = 0; trial < scenario.GetTrials(); ++trial)
    {
        engine::World world(scenario.GetSeed() + trial);

        for (Tick tick = 0; tick < scenario.GetTicks(); ++tick)
        {
            world.AdvanceTick();
        }

        summary.AddTicks(world.GetTick());
    }

    return summary;
}

}  // namespace osrssim::sim
