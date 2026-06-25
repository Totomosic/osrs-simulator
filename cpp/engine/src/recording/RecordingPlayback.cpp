#include "recording/RecordingPlayback.h"

#include "Scene.h"

#include <algorithm>
#include <stdexcept>
#include <tuple>
#include <unordered_map>
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
    Number,
    Boolean
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
        case RequiredJsonKind::Boolean:
            return value.is_boolean();
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

CardinalDirection ParseCardinalDirection(const nlohmann::json& value)
{
    const std::string direction = value.get<std::string>();

    if (direction == "North")
    {
        return CardinalDirection::North;
    }

    if (direction == "East")
    {
        return CardinalDirection::East;
    }

    if (direction == "South")
    {
        return CardinalDirection::South;
    }

    if (direction == "West")
    {
        return CardinalDirection::West;
    }

    throw std::invalid_argument("Unknown Scene Entity direction");
}

CollisionProfile ParseCollisionProfile(const nlohmann::json& value)
{
    RequireObjectField(value, "blocksMovement", RequiredJsonKind::Boolean);
    RequireObjectField(value, "blocksLineOfSight", RequiredJsonKind::Boolean);

    CollisionProfile collisionProfile;
    collisionProfile.blocksMovement = value.at("blocksMovement").get<bool>();
    collisionProfile.blocksLineOfSight =
        value.at("blocksLineOfSight").get<bool>();
    return collisionProfile;
}

CardinalDirection ParseWallDirectionEntryDirection(const nlohmann::json& value)
{
    RequireObjectField(value, "direction", RequiredJsonKind::String);
    return ParseCardinalDirection(value.at("direction"));
}

CollisionProfile ParseWallDirectionEntryCollision(const nlohmann::json& value)
{
    RequireObjectField(value, "collision", RequiredJsonKind::Object);
    return ParseCollisionProfile(value.at("collision"));
}

SceneCoordinate ParseSceneCoordinate(const nlohmann::json& value)
{
    RequireObjectField(value, "x", RequiredJsonKind::Integer);
    RequireObjectField(value, "y", RequiredJsonKind::Integer);
    RequireObjectField(value, "plane", RequiredJsonKind::Integer);

    return SceneCoordinate{
        value.at("x").get<int>(),
        value.at("y").get<int>(),
        value.at("plane").get<int>()};
}

std::string SceneEntityKey(const nlohmann::json& sceneEntity)
{
    const nlohmann::json& coordinate = sceneEntity.at("coordinate");

    return sceneEntity.at("kind").get<std::string>() + "|" +
           std::to_string(sceneEntity.at("sceneId").get<int>()) + "|" +
           std::to_string(coordinate.at("plane").get<int>()) + "|" +
           std::to_string(coordinate.at("x").get<int>()) + "|" +
           std::to_string(coordinate.at("y").get<int>()) + "|" +
           std::to_string(sceneEntity.at("id").get<int>());
}

std::vector<std::string> GetSortedSceneEntityKeys(
    const nlohmann::json& sceneEntities)
{
    std::vector<std::string> keys;
    keys.reserve(sceneEntities.size());

    for (auto iterator = sceneEntities.begin();
         iterator != sceneEntities.end();
         ++iterator)
    {
        keys.push_back(iterator.key());
    }

    std::sort(
        keys.begin(),
        keys.end(),
        [&sceneEntities](const std::string& lhs, const std::string& rhs)
        {
            const nlohmann::json& left = sceneEntities.at(lhs);
            const nlohmann::json& right = sceneEntities.at(rhs);
            const nlohmann::json& leftCoordinate = left.at("coordinate");
            const nlohmann::json& rightCoordinate = right.at("coordinate");
            const int leftKind =
                left.at("kind").get<std::string>() == "GameObject" ? 0 : 1;
            const int rightKind =
                right.at("kind").get<std::string>() == "GameObject" ? 0 : 1;

            return std::tuple{
                       left.at("sceneId").get<int>(),
                       leftCoordinate.at("plane").get<int>(),
                       leftCoordinate.at("x").get<int>(),
                       leftCoordinate.at("y").get<int>(),
                       leftKind,
                       left.at("id").get<int>()} <
                   std::tuple{
                       right.at("sceneId").get<int>(),
                       rightCoordinate.at("plane").get<int>(),
                       rightCoordinate.at("x").get<int>(),
                       rightCoordinate.at("y").get<int>(),
                       rightKind,
                       right.at("id").get<int>()};
        });

    return keys;
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

void ValidateSceneEntityShape(const nlohmann::json& sceneEntity)
{
    RequireObjectField(sceneEntity, "kind", RequiredJsonKind::String);
    RequireObjectField(sceneEntity, "sceneId", RequiredJsonKind::Integer);
    RequireObjectField(sceneEntity, "id", RequiredJsonKind::Integer);
    RequireObjectField(sceneEntity, "coordinate", RequiredJsonKind::Object);
    ParseSceneCoordinate(sceneEntity.at("coordinate"));
    RequireObjectField(sceneEntity, "direction", RequiredJsonKind::String);
    ParseCardinalDirection(sceneEntity.at("direction"));
    RequireObjectField(sceneEntity, "collision", RequiredJsonKind::Object);
    ParseCollisionProfile(sceneEntity.at("collision"));

    const std::string kind = sceneEntity.at("kind").get<std::string>();

    if (kind == "GameObject")
    {
        RequireObjectField(sceneEntity, "sizeX", RequiredJsonKind::Integer);
        RequireObjectField(sceneEntity, "sizeY", RequiredJsonKind::Integer);
        return;
    }

    if (kind != "WallObject")
    {
        throw std::invalid_argument("Unknown Scene Entity kind");
    }

    if (sceneEntity.contains("directions"))
    {
        RequireObjectField(sceneEntity, "directions", RequiredJsonKind::Array);

        if (sceneEntity.at("directions").size() != 2)
        {
            throw std::invalid_argument(
                "Recorded Wall Object directions must contain two entries");
        }

        for (const nlohmann::json& direction : sceneEntity.at("directions"))
        {
            ParseWallDirectionEntryDirection(direction);
            ParseWallDirectionEntryCollision(direction);
        }
    }
}

Scene& GetPlaybackScene(
    std::unordered_map<SceneId, Scene>& scenes,
    SceneId sceneId)
{
    auto [iterator, inserted] = scenes.try_emplace(sceneId);
    return iterator->second;
}

void ApplySceneEntityPlacementToScene(
    std::unordered_map<SceneId, Scene>& scenes,
    const nlohmann::json& sceneEntity)
{
    ValidateSceneEntityShape(sceneEntity);

    Scene& scene = GetPlaybackScene(
        scenes,
        sceneEntity.at("sceneId").get<SceneId>());
    const std::string kind = sceneEntity.at("kind").get<std::string>();
    const SceneCoordinate coordinate =
        ParseSceneCoordinate(sceneEntity.at("coordinate"));
    const EntityId id = sceneEntity.at("id").get<EntityId>();
    const CardinalDirection direction =
        ParseCardinalDirection(sceneEntity.at("direction"));
    const CollisionProfile collisionProfile =
        ParseCollisionProfile(sceneEntity.at("collision"));

    if (kind == "GameObject")
    {
        const bool placed = scene.PlaceGameObject(
            coordinate,
            id,
            direction,
            sceneEntity.at("sizeX").get<int>(),
            sceneEntity.at("sizeY").get<int>(),
            collisionProfile);

        if (!placed)
        {
            throw std::invalid_argument(
                "Recorded Game Object placement cannot be applied");
        }

        return;
    }

    bool placed = false;

    if (sceneEntity.contains("directions"))
    {
        const nlohmann::json& directions = sceneEntity.at("directions");
        placed = scene.PlaceWallObject(
            coordinate,
            id,
            ParseWallDirectionEntryDirection(directions.at(0)),
            ParseWallDirectionEntryCollision(directions.at(0)),
            ParseWallDirectionEntryDirection(directions.at(1)),
            ParseWallDirectionEntryCollision(directions.at(1)));
    }
    else
    {
        placed = scene.PlaceWallObject(coordinate, id, direction, collisionProfile);
    }

    if (!placed)
    {
        throw std::invalid_argument(
            "Recorded Wall Object placement cannot be applied");
    }
}

void ApplySceneEntityRemovalToScene(
    std::unordered_map<SceneId, Scene>& scenes,
    const nlohmann::json& sceneEntity)
{
    ValidateSceneEntityShape(sceneEntity);

    Scene& scene = GetPlaybackScene(
        scenes,
        sceneEntity.at("sceneId").get<SceneId>());
    const SceneCoordinate coordinate =
        ParseSceneCoordinate(sceneEntity.at("coordinate"));
    const std::string kind = sceneEntity.at("kind").get<std::string>();
    bool removed = false;

    if (kind == "GameObject")
    {
        removed = scene.RemoveGameObject(coordinate);
    }
    else
    {
        removed = scene.RemoveWallObject(coordinate);
    }

    if (!removed)
    {
        throw std::invalid_argument(
            "Recorded Scene Entity removal cannot be applied");
    }
}

void ApplySceneEntityChange(
    nlohmann::json& sceneEntities,
    std::unordered_map<SceneId, Scene>& scenes,
    const nlohmann::json& change)
{
    const std::string key = SceneEntityKey(change);

    if (change.contains("present") && change.at("present").is_boolean() &&
        !change.at("present").get<bool>())
    {
        ApplySceneEntityRemovalToScene(scenes, change);
        sceneEntities.erase(key);
        return;
    }

    ApplySceneEntityPlacementToScene(scenes, change);
    nlohmann::json sceneEntity = change;
    sceneEntity.erase("present");
    sceneEntities[key] = sceneEntity;
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

nlohmann::json SceneEntitiesObjectToSortedArray(
    const nlohmann::json& sceneEntities)
{
    nlohmann::json sceneEntityArray = nlohmann::json::array();

    for (const std::string& key : GetSortedSceneEntityKeys(sceneEntities))
    {
        sceneEntityArray.push_back(sceneEntities.at(key));
    }

    return sceneEntityArray;
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
    RequireObjectField(
        recording.at("initialState"),
        "sceneEntities",
        RequiredJsonKind::Array);
    RequireObjectField(recording, "ticks", RequiredJsonKind::Array);

    for (const nlohmann::json& sceneEntity :
         recording.at("initialState").at("sceneEntities"))
    {
        ValidateSceneEntityShape(sceneEntity);
    }

    int expectedTick = recording.at("initialState").at("tick").get<int>() + 1;

    for (const nlohmann::json& tick : recording.at("ticks"))
    {
        RequireObjectField(tick, "tick", RequiredJsonKind::Integer);
        RequireObjectField(tick, "actors", RequiredJsonKind::Array);
        RequireObjectField(tick, "sceneChanges", RequiredJsonKind::Array);

        for (const nlohmann::json& sceneChange : tick.at("sceneChanges"))
        {
            ValidateSceneEntityShape(sceneChange);
            RequireObjectField(
                sceneChange,
                "present",
                RequiredJsonKind::Boolean);
        }

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
    m_SceneEntities = nlohmann::json::object();
    std::unordered_map<SceneId, Scene> scenes;

    for (const nlohmann::json& actor : m_Recording.at("initialState").at("actors"))
    {
        ApplyActorChange(m_Actors, actor);
    }

    for (const nlohmann::json& sceneEntity :
         m_Recording.at("initialState").at("sceneEntities"))
    {
        ApplySceneEntityChange(m_SceneEntities, scenes, sceneEntity);
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

        for (const nlohmann::json& sceneChange : tick.at("sceneChanges"))
        {
            ApplySceneEntityChange(m_SceneEntities, scenes, sceneChange);
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

std::string RecordingPlayback::GetSceneEntitiesJson() const
{
    return SceneEntitiesObjectToSortedArray(m_SceneEntities).dump();
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
