#pragma once

#include "recording/EncounterRecording.h"

#include <nlohmann/json.hpp>

#include <string>

namespace osrssim::recording
{

class RecordingPlayback
{
private:
    EncounterRecording m_Recording;
    nlohmann::json m_CurrentSnapshot = nlohmann::json::object();
    nlohmann::json m_Actors = nlohmann::json::object();
    nlohmann::json m_SceneEntities = nlohmann::json::object();
    nlohmann::json m_Attacks = nlohmann::json::array();
    nlohmann::json m_DamageApplications = nlohmann::json::array();
    nlohmann::json m_Projectiles = nlohmann::json::array();
    int m_CurrentTick = 0;

    RecordingPlayback(EncounterRecording recording);
    void RebuildToCurrentTick();

public:
    static RecordingPlayback LoadFromJson(const std::string& jsonText);

    std::string GetEncounterName() const;
    double GetSecondsPerTick() const;
    int GetInitialTick() const;
    int GetCurrentTick() const;
    int GetLastTick() const;
    std::string GetCurrentSnapshotJson() const;
    bool Advance();
    void Reset();
    bool IsComplete() const;
};

}  // namespace osrssim::recording
