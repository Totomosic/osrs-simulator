#pragma once

#include <nlohmann/json.hpp>

#include <string>

namespace osrssim::recording
{

class RecordingPlayback
{
private:
    nlohmann::json m_Recording;
    nlohmann::json m_Actors = nlohmann::json::object();
    nlohmann::json m_SceneEntities = nlohmann::json::object();
    nlohmann::json m_Attacks = nlohmann::json::array();
    nlohmann::json m_DamageApplications = nlohmann::json::array();
    nlohmann::json m_Projectiles = nlohmann::json::array();
    int m_CurrentTick = 0;

    static void Validate(const nlohmann::json& recording);
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
    bool PreviousTick();
    bool NextTick();
    bool GoToTick(int tick);
};

}  // namespace osrssim::recording
