#include "recording/EncounterRecorder.h"

#include <stdexcept>
#include <utility>

namespace osrssim::recording
{

nlohmann::json EncounterRecorder::CreateEmptyTick(int tick)
{
    return nlohmann::json{
        {"tick", tick},
        {"actors", nlohmann::json::array()},
        {"attacks", nlohmann::json::array()},
        {"damageApplications", nlohmann::json::array()},
        {"sceneChanges", nlohmann::json::array()},
        {"projectiles", nlohmann::json::array()}};
}

EncounterRecorder::EncounterRecorder(
    std::string encounterName,
    double secondsPerTick)
    : m_EncounterName(std::move(encounterName)),
      m_SecondsPerTick(secondsPerTick)
{
    if (m_EncounterName.empty())
    {
        throw std::invalid_argument("Encounter Recording name cannot be empty");
    }

    if (m_SecondsPerTick <= 0.0)
    {
        throw std::invalid_argument(
            "Encounter Recording seconds per Tick must be positive");
    }
}

void EncounterRecorder::RecordInitialState(const Engine& engine)
{
    m_Recording = nlohmann::json{
        {"version", 1},
        {"metadata",
         {{"encounterName", m_EncounterName},
          {"secondsPerTick", m_SecondsPerTick}}},
        {"initialState",
         {{"tick", static_cast<int>(engine.GetCurrentTick())},
          {"actors", nlohmann::json::array()},
          {"sceneEntities", nlohmann::json::array()},
          {"projectiles", nlohmann::json::array()}}},
        {"ticks", nlohmann::json::array()}};
}

void EncounterRecorder::RecordCompletedTick(const Engine& engine)
{
    if (m_Recording.is_null())
    {
        throw std::logic_error(
            "Encounter Recording must record initial state before Ticks");
    }

    m_Recording["ticks"].push_back(
        CreateEmptyTick(static_cast<int>(engine.GetCurrentTick())));
}

std::string EncounterRecorder::ExportJson() const
{
    if (m_Recording.is_null())
    {
        throw std::logic_error(
            "Encounter Recording cannot export before initial state");
    }

    return m_Recording.dump();
}

}  // namespace osrssim::recording
