#pragma once

#include <nlohmann/json.hpp>

#include <string>
#include <vector>

namespace osrssim::recording
{

struct EncounterRecordingMetadata
{
    std::string encounterName;
    double secondsPerTick = 0.6;
};

struct CompletedRecordingTick
{
    int tick = 0;
    nlohmann::json facts = nlohmann::json::object();
};

class EncounterRecording
{
private:
    EncounterRecordingMetadata m_Metadata;
    int m_InitialTick = 0;
    nlohmann::json m_InitialFacts = nlohmann::json::object();
    std::vector<CompletedRecordingTick> m_CompletedTicks;

    EncounterRecording(
        EncounterRecordingMetadata metadata,
        int initialTick,
        nlohmann::json initialFacts,
        std::vector<CompletedRecordingTick> completedTicks);

    static nlohmann::json CreateEmptyInitialFacts();
    static nlohmann::json CreateEmptyCompletedTickFacts();
    static void ValidateInitialFacts(const nlohmann::json& facts);
    static void ValidateCompletedTickFacts(
        const nlohmann::json& facts,
        bool allowTickField = false);

public:
    static CompletedRecordingTick CreateEmptyCompletedTick(int tick);
    static EncounterRecording Create(
        EncounterRecordingMetadata metadata,
        int initialTick,
        std::vector<CompletedRecordingTick> completedTicks);
    static EncounterRecording LoadFromJson(const std::string& jsonText);
    static EncounterRecording LoadFromJson(const nlohmann::json& document);

    std::string ExportJson() const;
    nlohmann::json ToJson() const;

    const std::string& GetEncounterName() const;
    double GetSecondsPerTick() const;
    int GetInitialTick() const;
    int GetLastTick() const;
    const nlohmann::json& GetInitialFacts() const;
    const std::vector<CompletedRecordingTick>& GetCompletedTicks() const;
};

}  // namespace osrssim::recording
