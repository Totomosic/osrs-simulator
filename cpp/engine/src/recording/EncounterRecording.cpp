#include "recording/EncounterRecording.h"

#include <stdexcept>
#include <utility>

namespace osrssim::recording
{
namespace
{
enum class RequiredJsonKind
{
    Object,
    Array,
    String,
    Integer,
    Number
};

bool HasRequiredKind(const nlohmann::json& value, RequiredJsonKind kind)
{
    switch (kind)
    {
        case RequiredJsonKind::Object:
            return value.is_object();
        case RequiredJsonKind::Array:
            return value.is_array();
        case RequiredJsonKind::String:
            return value.is_string();
        case RequiredJsonKind::Integer:
            return value.is_number_integer() || value.is_number_unsigned();
        case RequiredJsonKind::Number:
            return value.is_number();
    }

    return false;
}

void RequireObjectField(
    const nlohmann::json& object,
    const char* fieldName,
    RequiredJsonKind kind)
{
    if (!object.contains(fieldName) ||
        !HasRequiredKind(object.at(fieldName), kind))
    {
        throw std::invalid_argument(
            std::string("Encounter Recording v2 field is missing or invalid: ") +
            fieldName);
    }
}
}  // namespace

EncounterRecording::EncounterRecording(
    EncounterRecordingMetadata metadata,
    int initialTick,
    nlohmann::json initialFacts,
    std::vector<CompletedRecordingTick> completedTicks)
    : m_Metadata(std::move(metadata)),
      m_InitialTick(initialTick),
      m_InitialFacts(std::move(initialFacts)),
      m_CompletedTicks(std::move(completedTicks))
{
}

nlohmann::json EncounterRecording::CreateEmptyInitialFacts()
{
    return {
        {"actorFacts", nlohmann::json::array()},
        {"sceneEntityFacts", nlohmann::json::array()},
        {"visibleProjectiles", nlohmann::json::array()}};
}

nlohmann::json EncounterRecording::CreateEmptyCompletedTickFacts()
{
    return {
        {"actorFacts", nlohmann::json::array()},
        {"sceneEntityFacts", nlohmann::json::array()},
        {"attacks", nlohmann::json::array()},
        {"damageApplications", nlohmann::json::array()},
        {"visibleProjectiles", nlohmann::json::array()}};
}

void EncounterRecording::ValidateInitialFacts(const nlohmann::json& facts)
{
    RequireObjectField(facts, "actorFacts", RequiredJsonKind::Array);
    RequireObjectField(facts, "sceneEntityFacts", RequiredJsonKind::Array);
    RequireObjectField(facts, "visibleProjectiles", RequiredJsonKind::Array);
}

void EncounterRecording::ValidateCompletedTickFacts(const nlohmann::json& facts)
{
    RequireObjectField(facts, "actorFacts", RequiredJsonKind::Array);
    RequireObjectField(facts, "sceneEntityFacts", RequiredJsonKind::Array);
    RequireObjectField(facts, "attacks", RequiredJsonKind::Array);
    RequireObjectField(facts, "damageApplications", RequiredJsonKind::Array);
    RequireObjectField(facts, "visibleProjectiles", RequiredJsonKind::Array);
}

EncounterRecording EncounterRecording::Create(
    EncounterRecordingMetadata metadata,
    int initialTick,
    std::vector<CompletedRecordingTick> completedTicks)
{
    int expectedTick = initialTick + 1;

    for (const CompletedRecordingTick& completedTick : completedTicks)
    {
        if (completedTick.tick != expectedTick)
        {
            throw std::invalid_argument(
                "Encounter Recording completed ticks must be contiguous");
        }

        ValidateCompletedTickFacts(completedTick.facts);
        ++expectedTick;
    }

    return EncounterRecording(
        std::move(metadata),
        initialTick,
        CreateEmptyInitialFacts(),
        std::move(completedTicks));
}

CompletedRecordingTick EncounterRecording::CreateEmptyCompletedTick(int tick)
{
    return {tick, CreateEmptyCompletedTickFacts()};
}

EncounterRecording EncounterRecording::LoadFromJson(
    const std::string& jsonText)
{
    return LoadFromJson(nlohmann::json::parse(jsonText));
}

EncounterRecording EncounterRecording::LoadFromJson(
    const nlohmann::json& document)
{
    if (!document.is_object())
    {
        throw std::invalid_argument("Encounter Recording root must be an object");
    }

    RequireObjectField(document, "version", RequiredJsonKind::Integer);

    if (document.at("version").get<int>() != 2)
    {
        throw std::invalid_argument("Unsupported Encounter Recording version");
    }

    RequireObjectField(document, "metadata", RequiredJsonKind::Object);
    RequireObjectField(
        document.at("metadata"),
        "encounterName",
        RequiredJsonKind::String);
    RequireObjectField(
        document.at("metadata"),
        "secondsPerTick",
        RequiredJsonKind::Number);
    RequireObjectField(document, "initialTick", RequiredJsonKind::Integer);
    RequireObjectField(document, "initialFacts", RequiredJsonKind::Object);
    RequireObjectField(document, "completedTicks", RequiredJsonKind::Array);

    ValidateInitialFacts(document.at("initialFacts"));

    std::vector<CompletedRecordingTick> completedTicks;
    int expectedTick = document.at("initialTick").get<int>() + 1;

    for (const nlohmann::json& tick : document.at("completedTicks"))
    {
        RequireObjectField(tick, "tick", RequiredJsonKind::Integer);
        ValidateCompletedTickFacts(tick);

        if (tick.at("tick").get<int>() != expectedTick)
        {
            throw std::invalid_argument(
                "Encounter Recording completed ticks must be contiguous");
        }

        completedTicks.push_back(
            {tick.at("tick").get<int>(),
             {{"actorFacts", tick.at("actorFacts")},
              {"sceneEntityFacts", tick.at("sceneEntityFacts")},
              {"attacks", tick.at("attacks")},
              {"damageApplications", tick.at("damageApplications")},
              {"visibleProjectiles", tick.at("visibleProjectiles")}}});
        ++expectedTick;
    }

    return EncounterRecording(
        {document.at("metadata").at("encounterName").get<std::string>(),
         document.at("metadata").at("secondsPerTick").get<double>()},
        document.at("initialTick").get<int>(),
        document.at("initialFacts"),
        std::move(completedTicks));
}

std::string EncounterRecording::ExportJson() const
{
    return ToJson().dump();
}

nlohmann::json EncounterRecording::ToJson() const
{
    nlohmann::json completedTicks = nlohmann::json::array();

    for (const CompletedRecordingTick& completedTick : m_CompletedTicks)
    {
        completedTicks.push_back(
            {{"tick", completedTick.tick},
             {"actorFacts", completedTick.facts.at("actorFacts")},
             {"sceneEntityFacts", completedTick.facts.at("sceneEntityFacts")},
             {"attacks", completedTick.facts.at("attacks")},
             {"damageApplications",
              completedTick.facts.at("damageApplications")},
             {"visibleProjectiles",
              completedTick.facts.at("visibleProjectiles")}});
    }

    return {
        {"version", 2},
        {"metadata",
         {{"encounterName", m_Metadata.encounterName},
          {"secondsPerTick", m_Metadata.secondsPerTick}}},
        {"initialTick", m_InitialTick},
        {"initialFacts", m_InitialFacts},
        {"completedTicks", completedTicks}};
}

const std::string& EncounterRecording::GetEncounterName() const
{
    return m_Metadata.encounterName;
}

double EncounterRecording::GetSecondsPerTick() const
{
    return m_Metadata.secondsPerTick;
}

int EncounterRecording::GetInitialTick() const
{
    return m_InitialTick;
}

int EncounterRecording::GetLastTick() const
{
    if (m_CompletedTicks.empty())
    {
        return m_InitialTick;
    }

    return m_CompletedTicks.back().tick;
}

const nlohmann::json& EncounterRecording::GetInitialFacts() const
{
    return m_InitialFacts;
}

const std::vector<CompletedRecordingTick>&
EncounterRecording::GetCompletedTicks() const
{
    return m_CompletedTicks;
}

}  // namespace osrssim::recording
