#pragma once

#include "recording/EncounterRecording.h"

#include <nlohmann/json.hpp>

#include <optional>
#include <string>

namespace osrssim::recording
{

class RecordingPlayback
{
private:
    nlohmann::json m_Recording;
    std::optional<EncounterRecording> m_Version2Recording;
    nlohmann::json m_CurrentSnapshot = nlohmann::json::object();
    nlohmann::json m_Actors = nlohmann::json::object();
    nlohmann::json m_SceneEntities = nlohmann::json::object();
    nlohmann::json m_Attacks = nlohmann::json::array();
    nlohmann::json m_DamageApplications = nlohmann::json::array();
    nlohmann::json m_Projectiles = nlohmann::json::array();
    int m_CurrentTick = 0;

    static void Validate(const nlohmann::json& recording);
    void RebuildVersion2Snapshot();
    void RebuildToCurrentTick();

public:
    static RecordingPlayback LoadFromJson(const std::string& jsonText);

    std::string GetEncounterName() const;
    double GetSecondsPerTick() const;
    int GetInitialTick() const;
    int GetCurrentTick() const;
    int GetLastTick() const;
    std::string GetActorsJson() const;
    std::string GetSceneEntitiesJson() const;
    std::string GetAttacksJson() const;
    std::string GetDamageApplicationsJson() const;
    std::string GetProjectilesJson() const;
    std::string GetCurrentSnapshotJson() const;
    bool Advance();
    void Reset();
    bool IsComplete() const;
    bool PreviousTick();
    bool NextTick();
    bool GoToTick(int tick);
};

}  // namespace osrssim::recording
