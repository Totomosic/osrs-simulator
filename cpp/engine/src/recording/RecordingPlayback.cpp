#include "recording/RecordingPlayback.h"

#include <stdexcept>

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
    RequireObjectField(recording, "ticks", RequiredJsonKind::Array);

    int expectedTick = recording.at("initialState").at("tick").get<int>() + 1;

    for (const nlohmann::json& tick : recording.at("ticks"))
    {
        RequireObjectField(tick, "tick", RequiredJsonKind::Integer);

        if (tick.at("tick").get<int>() != expectedTick)
        {
            throw std::invalid_argument(
                "Encounter Recording Ticks must be contiguous");
        }

        ++expectedTick;
    }
}

RecordingPlayback RecordingPlayback::LoadFromJson(const std::string& jsonText)
{
    RecordingPlayback playback;
    playback.m_Recording = nlohmann::json::parse(jsonText);
    Validate(playback.m_Recording);
    playback.m_CurrentTick =
        playback.m_Recording.at("initialState").at("tick").get<int>();
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
    return true;
}

}  // namespace osrssim::recording
