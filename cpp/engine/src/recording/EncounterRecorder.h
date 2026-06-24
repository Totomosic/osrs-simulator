#pragma once

#include "Engine.h"

#include <nlohmann/json.hpp>

#include <string>
#include <unordered_map>

namespace osrssim::recording
{

class EncounterRecorder
{
private:
    std::string m_EncounterName;
    double m_SecondsPerTick = 0.6;
    nlohmann::json m_Recording;
    std::unordered_map<ActorId, nlohmann::json> m_PreviousActors;

    static nlohmann::json CreateEmptyTick(int tick);
    static nlohmann::json CreateActorSnapshot(
        const World& world,
        ActorId actorId);
    static std::unordered_map<ActorId, nlohmann::json> CreatePlacedActors(
        const World& world);
    static nlohmann::json CreateActorChanges(
        const std::unordered_map<ActorId, nlohmann::json>& previousActors,
        const std::unordered_map<ActorId, nlohmann::json>& currentActors);

public:
    EncounterRecorder(std::string encounterName, double secondsPerTick);

    void RecordInitialState(const Engine& engine);
    void RecordCompletedTick(const Engine& engine);
    std::string ExportJson() const;
};

}  // namespace osrssim::recording
