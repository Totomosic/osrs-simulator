#pragma once

#include "Engine.h"

#include <nlohmann/json.hpp>

#include <string>
#include <unordered_map>

namespace osrssim::recording
{

class EncounterRecorder : public CombatService::Observer
{
private:
    std::string m_EncounterName;
    double m_SecondsPerTick = 0.6;
    nlohmann::json m_Recording;
    std::unordered_map<ActorId, nlohmann::json> m_PreviousActors;
    std::unordered_map<std::string, nlohmann::json> m_PreviousSceneEntities;
    nlohmann::json m_PendingAttacks = nlohmann::json::array();
    nlohmann::json m_PendingDamageApplications = nlohmann::json::array();

    static nlohmann::json CreateEmptyTick(int tick);
    static nlohmann::json CreateActorSnapshot(
        const World& world,
        ActorId actorId);
    static std::unordered_map<ActorId, nlohmann::json> CreatePlacedActors(
        const World& world);
    static nlohmann::json CreateActorChanges(
        const std::unordered_map<ActorId, nlohmann::json>& previousActors,
        const std::unordered_map<ActorId, nlohmann::json>& currentActors);
    static std::unordered_map<std::string, nlohmann::json> CreateSceneEntities(
        const World& world);
    static nlohmann::json CreateSceneEntityChanges(
        const std::unordered_map<std::string, nlohmann::json>& previousSceneEntities,
        const std::unordered_map<std::string, nlohmann::json>& currentSceneEntities);
    static nlohmann::json CreateProjectileJson(
        const ProjectileMetadata& projectile);
    static nlohmann::json CreateProjectileSnapshotJson(
        const ProjectileSnapshot& projectile);
    static nlohmann::json CreateProjectileSnapshots(const World& world);

public:
    EncounterRecorder(std::string encounterName, double secondsPerTick);

    void RecordInitialState(const Engine& engine);
    void RecordCompletedTick(const Engine& engine);
    std::string ExportJson() const;
    std::string ExportVersion2Json() const;
    void OnAttackQueued(
        const CombatService::AttackObservation& attack) override;
    void OnDamageApplied(
        const CombatService::DamageApplicationObservation& damageApplication)
        override;
};

}  // namespace osrssim::recording
