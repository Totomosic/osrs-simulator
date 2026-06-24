#pragma once

#include <nlohmann/json.hpp>

#include <string>

namespace osrssim::recording
{

class RecordingPlayback
{
private:
    nlohmann::json m_Recording;
    int m_CurrentTick = 0;

    static void Validate(const nlohmann::json& recording);

public:
    static RecordingPlayback LoadFromJson(const std::string& jsonText);

    std::string GetEncounterName() const;
    double GetSecondsPerTick() const;
    int GetInitialTick() const;
    int GetCurrentTick() const;
    int GetLastTick() const;
    bool PreviousTick();
    bool NextTick();
    bool GoToTick(int tick);
};

}  // namespace osrssim::recording
