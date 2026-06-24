#include "recording/RecordingPlayback.h"

#include <algorithm>
#include <stdexcept>
#include <vector>

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
            std::string("Recording field is missing or invalid: ") +
            fieldName);
    }
}

std::string ActorKey(const nlohmann::json& actor)
{
    return std::to_string(actor.at("id").get<unsigned long long>());
}

std::string ActorKey(int actorId)
{
    return std::to_string(actorId);
}

void ApplyActorChange(nlohmann::json& actors, const nlohmann::json& change)
{
    const std::string key = ActorKey(change);

    if (change.contains("present") && change.at("present").is_boolean() &&
        !change.at("present").get<bool>())
    {
        actors.erase(key);
        return;
    }

    nlohmann::json actor =
        actors.contains(key) ? actors.at(key) : nlohmann::json::object();

    for (auto iterator = change.begin(); iterator != change.end(); ++iterator)
    {
        const std::string& field = iterator.key();

        if (field == "debug")
        {
            if (!actor.contains("debug") || !actor.at("debug").is_object())
            {
                actor["debug"] = nlohmann::json::object();
            }

            for (auto debugIterator = iterator.value().begin();
                 debugIterator != iterator.value().end();
                 ++debugIterator)
            {
                actor["debug"][debugIterator.key()] = debugIterator.value();
            }
            continue;
        }

        if (field == "currentHitpoints")
        {
            actor["combatComposition"]["stats"]["hitpoints"] =
                iterator.value();
            continue;
        }

        actor[field] = iterator.value();
    }

    actors[key] = actor;
}

nlohmann::json ActorsObjectToSortedArray(const nlohmann::json& actors)
{
    std::vector<int> actorIds;
    actorIds.reserve(actors.size());

    for (auto iterator = actors.begin(); iterator != actors.end(); ++iterator)
    {
        actorIds.push_back(std::stoi(iterator.key()));
    }

    std::sort(actorIds.begin(), actorIds.end());

    nlohmann::json actorArray = nlohmann::json::array();

    for (int actorId : actorIds)
    {
        actorArray.push_back(actors.at(ActorKey(actorId)));
    }

    return actorArray;
}
}  // namespace

void RecordingPlayback::Validate(const nlohmann::json& recording)
{
    if (!recording.is_object())
    {
        throw std::invalid_argument("Recording root must be an object");
    }

    RequireObjectField(recording, "version", RequiredJsonKind::Integer);

    if (recording.at("version").get<int>() != 1)
    {
        throw std::invalid_argument("Unsupported Encounter Recording version");
    }

    RequireObjectField(recording, "metadata", RequiredJsonKind::Object);
    RequireObjectField(
        recording.at("metadata"),
        "encounterName",
        RequiredJsonKind::String);
    RequireObjectField(
        recording.at("metadata"),
        "secondsPerTick",
        RequiredJsonKind::Number);
    RequireObjectField(recording, "initialState", RequiredJsonKind::Object);
    RequireObjectField(
        recording.at("initialState"),
        "tick",
        RequiredJsonKind::Integer);
    RequireObjectField(
        recording.at("initialState"),
        "actors",
        RequiredJsonKind::Array);
    RequireObjectField(recording, "ticks", RequiredJsonKind::Array);

    int expectedTick = recording.at("initialState").at("tick").get<int>() + 1;

    for (const nlohmann::json& tick : recording.at("ticks"))
    {
        RequireObjectField(tick, "tick", RequiredJsonKind::Integer);
        RequireObjectField(tick, "actors", RequiredJsonKind::Array);

        if (tick.at("tick").get<int>() != expectedTick)
        {
            throw std::invalid_argument(
                "Encounter Recording Ticks must be contiguous");
        }

        ++expectedTick;
    }
}

void RecordingPlayback::RebuildToCurrentTick()
{
    m_Actors = nlohmann::json::object();

    for (const nlohmann::json& actor : m_Recording.at("initialState").at("actors"))
    {
        ApplyActorChange(m_Actors, actor);
    }

    for (const nlohmann::json& tick : m_Recording.at("ticks"))
    {
        if (tick.at("tick").get<int>() > m_CurrentTick)
        {
            break;
        }

        for (const nlohmann::json& actorChange : tick.at("actors"))
        {
            ApplyActorChange(m_Actors, actorChange);
        }
    }
}

RecordingPlayback RecordingPlayback::LoadFromJson(const std::string& jsonText)
{
    RecordingPlayback playback;
    playback.m_Recording = nlohmann::json::parse(jsonText);
    Validate(playback.m_Recording);
    playback.m_CurrentTick =
        playback.m_Recording.at("initialState").at("tick").get<int>();
    playback.RebuildToCurrentTick();
    return playback;
}

std::string RecordingPlayback::GetEncounterName() const
{
    return m_Recording.at("metadata").at("encounterName").get<std::string>();
}

double RecordingPlayback::GetSecondsPerTick() const
{
    return m_Recording.at("metadata").at("secondsPerTick").get<double>();
}

int RecordingPlayback::GetInitialTick() const
{
    return m_Recording.at("initialState").at("tick").get<int>();
}

int RecordingPlayback::GetCurrentTick() const
{
    return m_CurrentTick;
}

int RecordingPlayback::GetLastTick() const
{
    if (m_Recording.at("ticks").empty())
    {
        return GetInitialTick();
    }

    return m_Recording.at("ticks").back().at("tick").get<int>();
}

std::string RecordingPlayback::GetActorsJson() const
{
    return ActorsObjectToSortedArray(m_Actors).dump();
}

bool RecordingPlayback::PreviousTick()
{
    return GoToTick(m_CurrentTick - 1);
}

bool RecordingPlayback::NextTick()
{
    return GoToTick(m_CurrentTick + 1);
}

bool RecordingPlayback::GoToTick(int tick)
{
    if (tick < GetInitialTick() || tick > GetLastTick())
    {
        return false;
    }

    m_CurrentTick = tick;
    RebuildToCurrentTick();
    return true;
}

}  // namespace osrssim::recording
