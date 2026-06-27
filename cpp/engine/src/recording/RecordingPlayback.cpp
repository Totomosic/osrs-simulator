#include "recording/RecordingPlayback.h"

#include "Scene.h"

#include <algorithm>
#include <stdexcept>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
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

    if (direction == "North" || direction == "north")
    {
        return CardinalDirection::North;
    }

    if (direction == "East" || direction == "east")
    {
        return CardinalDirection::East;
    }

    if (direction == "South" || direction == "south")
    {
        return CardinalDirection::South;
    }

    if (direction == "West" || direction == "west")
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

bool IsKnownAttackType(const std::string& value)
{
    return value == "Stab" || value == "Slash" || value == "Crush" ||
           value == "Magic" || value == "RangedLight" ||
           value == "RangedStandard" || value == "RangedHeavy" ||
           value == "stab" || value == "slash" || value == "crush" ||
           value == "magic" || value == "ranged_light" ||
           value == "ranged_standard" || value == "ranged_heavy";
}

bool IsKnownEquipmentSlot(const std::string& value)
{
    return value == "Head" || value == "Cape" || value == "Amulet" ||
           value == "Weapon" || value == "Body" || value == "Shield" ||
           value == "Legs" || value == "Hands" || value == "Feet" ||
           value == "Ring" || value == "Ammo";
}

bool IsKnownActorKind(const std::string& value)
{
    return value == "Player" || value == "Npc" ||
           value == "player" || value == "npc";
}

bool IsGameObjectKind(const std::string& value)
{
    return value == "GameObject" || value == "game_object";
}

bool IsWallObjectKind(const std::string& value)
{
    return value == "WallObject" || value == "wall_object";
}

int SceneEntityKindSortRank(const nlohmann::json& sceneEntity)
{
    return IsGameObjectKind(sceneEntity.at("kind").get<std::string>()) ? 0 : 1;
}

void RequireKnownStringValue(
    const nlohmann::json& object,
    const char* fieldName,
    bool (*isKnown)(const std::string&),
    const char* errorMessage)
{
    RequireObjectField(object, fieldName, RequiredJsonKind::String);

    if (!isKnown(object.at(fieldName).get<std::string>()))
    {
        throw std::invalid_argument(errorMessage);
    }
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

void ValidateSceneCoordinateShape(const nlohmann::json& value)
{
    const Scene scene;
    const SceneCoordinate coordinate = ParseSceneCoordinate(value);

    if (!scene.Contains(coordinate))
    {
        throw std::invalid_argument("Recorded Scene Coordinate is invalid");
    }
}

void ValidateScenePositionShape(const nlohmann::json& value)
{
    RequireObjectField(value, "x", RequiredJsonKind::Number);
    RequireObjectField(value, "y", RequiredJsonKind::Number);
    RequireObjectField(value, "plane", RequiredJsonKind::Integer);
}

void ValidateSceneMembershipShape(const nlohmann::json& value)
{
    RequireObjectField(value, "sceneId", RequiredJsonKind::Integer);
    RequireObjectField(value, "coordinate", RequiredJsonKind::Object);
    ValidateSceneCoordinateShape(value.at("coordinate"));
}

void ValidateActorFootprintShape(SceneCoordinate coordinate, int actorSize)
{
    const Scene scene;

    for (int offsetX = 0; offsetX < actorSize; ++offsetX)
    {
        for (int offsetY = 0; offsetY < actorSize; ++offsetY)
        {
            if (!scene.Contains(
                    {coordinate.x + offsetX,
                     coordinate.y + offsetY,
                     coordinate.plane}))
            {
                throw std::invalid_argument(
                    "Recorded Actor placement cannot be applied");
            }
        }
    }
}

void ValidateCombatStatsShape(const nlohmann::json& value)
{
    RequireObjectField(value, "attack", RequiredJsonKind::Integer);
    RequireObjectField(value, "strength", RequiredJsonKind::Integer);
    RequireObjectField(value, "defence", RequiredJsonKind::Integer);
    RequireObjectField(value, "ranged", RequiredJsonKind::Integer);
    RequireObjectField(value, "magic", RequiredJsonKind::Integer);
    RequireObjectField(value, "hitpoints", RequiredJsonKind::Integer);
}

void ValidateWeaponShape(const nlohmann::json& value)
{
    RequireObjectField(value, "id", RequiredJsonKind::Integer);
    RequireObjectField(value, "range", RequiredJsonKind::Integer);
    RequireObjectField(value, "speed", RequiredJsonKind::Integer);
    RequireObjectField(value, "projectileId", RequiredJsonKind::Integer);
}

void ValidateEquipmentProvenanceShape(const nlohmann::json& value)
{
    if (!value.is_array())
    {
        throw std::invalid_argument(
            "Recording equipment provenance must be an array");
    }

    for (const nlohmann::json& piece : value)
    {
        RequireKnownStringValue(
            piece,
            "slot",
            IsKnownEquipmentSlot,
            "Unknown Equipment Slot");
        RequireObjectField(piece, "pieceId", RequiredJsonKind::Integer);
    }
}

void ValidateCombatCompositionShape(const nlohmann::json& value)
{
    RequireObjectField(value, "stats", RequiredJsonKind::Object);
    ValidateCombatStatsShape(value.at("stats"));
    RequireObjectField(value, "baseStats", RequiredJsonKind::Object);
    ValidateCombatStatsShape(value.at("baseStats"));
    RequireObjectField(value, "bonuses", RequiredJsonKind::Object);
    RequireKnownStringValue(
        value,
        "attackType",
        IsKnownAttackType,
        "Unknown Attack Type");
    RequireObjectField(
        value,
        "magicBaseMaximumHit",
        RequiredJsonKind::Integer);
    RequireObjectField(value, "weapon", RequiredJsonKind::Object);
    ValidateWeaponShape(value.at("weapon"));

    if (value.contains("equipmentProvenance"))
    {
        ValidateEquipmentProvenanceShape(value.at("equipmentProvenance"));
    }
}

void ValidateMovementTargetShape(const nlohmann::json& value)
{
    if (value.is_null())
    {
        return;
    }

    RequireKnownStringValue(
        value,
        "kind",
        [](const std::string& kind)
        {
            return kind == "SceneCoordinate" || kind == "Actor" ||
                   kind == "scene_coordinate" || kind == "actor";
        },
        "Unknown Movement Target kind");

    const std::string kind = value.at("kind").get<std::string>();

    if (kind == "SceneCoordinate" || kind == "scene_coordinate")
    {
        RequireObjectField(value, "coordinate", RequiredJsonKind::Object);
        ValidateSceneCoordinateShape(value.at("coordinate"));
        return;
    }

    RequireObjectField(value, "actorId", RequiredJsonKind::Integer);
}

void ValidateActorDebugShape(const nlohmann::json& value)
{
    if (value.contains("movementTarget"))
    {
        ValidateMovementTargetShape(value.at("movementTarget"));
    }

    if (value.contains("attackTimer"))
    {
        RequireObjectField(value, "attackTimer", RequiredJsonKind::Integer);
    }
}

void ValidateActorShape(const nlohmann::json& actor)
{
    RequireObjectField(actor, "id", RequiredJsonKind::Integer);

    if (actor.contains("present"))
    {
        RequireObjectField(actor, "present", RequiredJsonKind::Boolean);

        if (!actor.at("present").get<bool>())
        {
            return;
        }
    }

    if (actor.contains("kind"))
    {
        RequireKnownStringValue(
            actor,
            "kind",
            IsKnownActorKind,
            "Unknown Actor kind");
    }

    const bool fullActor = actor.contains("present") &&
                           actor.at("present").get<bool>();

    if (fullActor)
    {
        RequireKnownStringValue(
            actor,
            "kind",
            IsKnownActorKind,
            "Unknown Actor kind");

        if (actor.at("kind").get<std::string>() == "Player")
        {
            RequireObjectField(actor, "playerIndex", RequiredJsonKind::Integer);
        }
        else
        {
            RequireObjectField(actor, "npcIndex", RequiredJsonKind::Integer);
        }

        RequireObjectField(actor, "size", RequiredJsonKind::Integer);
        RequireObjectField(actor, "speed", RequiredJsonKind::Integer);
        RequireObjectField(
            actor,
            "sceneMembership",
            RequiredJsonKind::Object);
        RequireObjectField(
            actor,
            "combatComposition",
            RequiredJsonKind::Object);
        RequireObjectField(actor, "debug", RequiredJsonKind::Object);
    }

    if (actor.contains("sceneMembership"))
    {
        ValidateSceneMembershipShape(actor.at("sceneMembership"));

        if (actor.contains("size") && actor.at("size").is_number_integer())
        {
            ValidateActorFootprintShape(
                ParseSceneCoordinate(
                    actor.at("sceneMembership").at("coordinate")),
                actor.at("size").get<int>());
        }
    }

    if (actor.contains("currentHitpoints"))
    {
        RequireObjectField(actor, "currentHitpoints", RequiredJsonKind::Integer);
    }

    if (actor.contains("combatComposition"))
    {
        ValidateCombatCompositionShape(actor.at("combatComposition"));
    }

    if (actor.contains("debug"))
    {
        RequireObjectField(actor, "debug", RequiredJsonKind::Object);
        ValidateActorDebugShape(actor.at("debug"));
    }
}

void ValidateRecordedActorFactShape(const nlohmann::json& actor)
{
    RequireObjectField(actor, "id", RequiredJsonKind::Integer);
    RequireObjectField(actor, "present", RequiredJsonKind::Boolean);

    if (!actor.at("present").get<bool>())
    {
        return;
    }

    if (actor.contains("kind"))
    {
        RequireKnownStringValue(
            actor,
            "kind",
            IsKnownActorKind,
            "Unknown Actor kind");

        if (actor.at("kind").get<std::string>() == "player" ||
            actor.at("kind").get<std::string>() == "Player")
        {
            RequireObjectField(actor, "playerIndex", RequiredJsonKind::Integer);
        }
        else
        {
            RequireObjectField(actor, "npcIndex", RequiredJsonKind::Integer);
        }

        RequireObjectField(actor, "size", RequiredJsonKind::Integer);
        RequireObjectField(actor, "speed", RequiredJsonKind::Integer);
        RequireObjectField(
            actor,
            "sceneMembership",
            RequiredJsonKind::Object);
        RequireObjectField(
            actor,
            "combatComposition",
            RequiredJsonKind::Object);
        RequireObjectField(
            actor,
            "currentHitpoints",
            RequiredJsonKind::Integer);
    }

    if (actor.contains("sceneMembership"))
    {
        ValidateSceneMembershipShape(actor.at("sceneMembership"));

        if (actor.contains("size") && actor.at("size").is_number_integer())
        {
            ValidateActorFootprintShape(
                ParseSceneCoordinate(
                    actor.at("sceneMembership").at("coordinate")),
                actor.at("size").get<int>());
        }
    }

    if (actor.contains("currentHitpoints"))
    {
        RequireObjectField(actor, "currentHitpoints", RequiredJsonKind::Integer);
    }

    if (actor.contains("combatComposition"))
    {
        ValidateCombatCompositionShape(actor.at("combatComposition"));
    }

    if (actor.contains("movementTarget"))
    {
        ValidateMovementTargetShape(actor.at("movementTarget"));
    }

    if (actor.contains("attackTimer"))
    {
        RequireObjectField(actor, "attackTimer", RequiredJsonKind::Integer);
    }
}

void ValidateProjectileShape(const nlohmann::json& projectile)
{
    RequireObjectField(projectile, "projectileId", RequiredJsonKind::Integer);
    RequireObjectField(projectile, "source", RequiredJsonKind::Object);
    ValidateScenePositionShape(projectile.at("source"));
    RequireObjectField(projectile, "targetActorId", RequiredJsonKind::Integer);
    RequireObjectField(
        projectile,
        "lastKnownTargetCenter",
        RequiredJsonKind::Object);
    ValidateScenePositionShape(projectile.at("lastKnownTargetCenter"));
    RequireObjectField(projectile, "totalTicks", RequiredJsonKind::Integer);

    if (projectile.contains("elapsedTicks"))
    {
        RequireObjectField(
            projectile,
            "elapsedTicks",
            RequiredJsonKind::Integer);
    }
}

void ValidateQueuedDamageEventShape(const nlohmann::json& damageEvent)
{
    RequireObjectField(damageEvent, "id", RequiredJsonKind::Integer);
    RequireObjectField(damageEvent, "attackId", RequiredJsonKind::Integer);
    RequireObjectField(damageEvent, "targetId", RequiredJsonKind::Integer);
    RequireObjectField(damageEvent, "damage", RequiredJsonKind::Integer);
    RequireObjectField(damageEvent, "delayTicks", RequiredJsonKind::Integer);
}

void ValidateAttackShape(const nlohmann::json& attack)
{
    RequireObjectField(attack, "id", RequiredJsonKind::Integer);
    RequireObjectField(attack, "tick", RequiredJsonKind::Integer);
    RequireObjectField(attack, "attackerId", RequiredJsonKind::Integer);
    RequireObjectField(attack, "targetId", RequiredJsonKind::Integer);
    RequireObjectField(attack, "callback", RequiredJsonKind::String);
    RequireObjectField(
        attack,
        "queuedDamageEvents",
        RequiredJsonKind::Array);

    for (const nlohmann::json& damageEvent :
         attack.at("queuedDamageEvents"))
    {
        ValidateQueuedDamageEventShape(damageEvent);
    }

    if (attack.contains("projectile"))
    {
        RequireObjectField(attack, "projectile", RequiredJsonKind::Object);
        ValidateProjectileShape(attack.at("projectile"));
    }
}

void ValidateDamageApplicationShape(const nlohmann::json& damageApplication)
{
    RequireObjectField(
        damageApplication,
        "damageEventId",
        RequiredJsonKind::Integer);
    RequireObjectField(damageApplication, "attackId", RequiredJsonKind::Integer);
    RequireObjectField(damageApplication, "tick", RequiredJsonKind::Integer);
    RequireObjectField(damageApplication, "targetId", RequiredJsonKind::Integer);
    RequireObjectField(
        damageApplication,
        "queuedDamage",
        RequiredJsonKind::Integer);
    RequireObjectField(
        damageApplication,
        "appliedDamage",
        RequiredJsonKind::Integer);
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
            const int leftKind = SceneEntityKindSortRank(left);
            const int rightKind = SceneEntityKindSortRank(right);

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
            actor["currentHitpoints"] = iterator.value();
            continue;
        }

        actor[field] = iterator.value();
    }

    actors[key] = actor;
}

bool IsFullRecordedActorFact(const nlohmann::json& fact)
{
    return fact.contains("present") &&
           fact.at("present").is_boolean() &&
           fact.at("present").get<bool>() &&
           fact.contains("kind");
}

void ApplyRecordedActorFact(nlohmann::json& actors, const nlohmann::json& fact)
{
    ApplyActorChange(actors, fact);
}

void ValidateNoDuplicateActorFacts(const nlohmann::json& actorFacts)
{
    std::vector<int> actorIds;

    for (const nlohmann::json& actorFact : actorFacts)
    {
        ValidateRecordedActorFactShape(actorFact);
        actorIds.push_back(actorFact.at("id").get<int>());
    }

    std::sort(actorIds.begin(), actorIds.end());

    for (std::size_t index = 1; index < actorIds.size(); ++index)
    {
        if (actorIds[index - 1] == actorIds[index])
        {
            throw std::invalid_argument(
                "Projection Validity failed: duplicate actor facts in tick");
        }
    }
}

void ValidateVersion2ActorProjection(const EncounterRecording& recording)
{
    nlohmann::json actors = nlohmann::json::object();

    ValidateNoDuplicateActorFacts(
        recording.GetInitialFacts().at("actorFacts"));

    for (const nlohmann::json& actorFact :
         recording.GetInitialFacts().at("actorFacts"))
    {
        if (!IsFullRecordedActorFact(actorFact))
        {
            throw std::invalid_argument(
                "Projection Validity failed: initial actor facts must be full");
        }

        ApplyRecordedActorFact(actors, actorFact);
    }

    for (const CompletedRecordingTick& completedTick :
         recording.GetCompletedTicks())
    {
        ValidateNoDuplicateActorFacts(
            completedTick.facts.at("actorFacts"));

        for (const nlohmann::json& actorFact :
             completedTick.facts.at("actorFacts"))
        {
            const std::string key = ActorKey(actorFact);
            const bool actorIsPresent = actors.contains(key);
            const bool factIsPresence =
                actorFact.at("present").get<bool>();

            if (!factIsPresence)
            {
                if (!actorIsPresent)
                {
                    throw std::invalid_argument(
                        "Projection Validity failed: actor absence before presence");
                }

                ApplyRecordedActorFact(actors, actorFact);
                continue;
            }

            if (!actorIsPresent && !IsFullRecordedActorFact(actorFact))
            {
                throw std::invalid_argument(
                    "Projection Validity failed: actor reappearance must be full");
            }

            ApplyRecordedActorFact(actors, actorFact);
        }
    }
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

bool IsFullRecordedSceneEntityFact(const nlohmann::json& fact)
{
    return fact.contains("present") &&
           fact.at("present").is_boolean() &&
           fact.at("present").get<bool>() &&
           fact.contains("direction") &&
           fact.contains("collision");
}

void ValidateRecordedSceneEntityFactShape(const nlohmann::json& fact)
{
    RequireObjectField(fact, "kind", RequiredJsonKind::String);
    RequireObjectField(fact, "sceneId", RequiredJsonKind::Integer);
    RequireObjectField(fact, "id", RequiredJsonKind::Integer);
    RequireObjectField(fact, "coordinate", RequiredJsonKind::Object);
    ValidateSceneCoordinateShape(fact.at("coordinate"));
    RequireObjectField(fact, "present", RequiredJsonKind::Boolean);

    const std::string kind = fact.at("kind").get<std::string>();

    if (!IsGameObjectKind(kind) && !IsWallObjectKind(kind))
    {
        throw std::invalid_argument("Unknown Recorded Scene Entity kind");
    }

    if (!fact.at("present").get<bool>())
    {
        return;
    }

    RequireObjectField(fact, "direction", RequiredJsonKind::String);
    ParseCardinalDirection(fact.at("direction"));
    RequireObjectField(fact, "collision", RequiredJsonKind::Object);
    ParseCollisionProfile(fact.at("collision"));

    if (IsGameObjectKind(kind))
    {
        RequireObjectField(fact, "sizeX", RequiredJsonKind::Integer);
        RequireObjectField(fact, "sizeY", RequiredJsonKind::Integer);
        return;
    }

    if (fact.contains("directions"))
    {
        RequireObjectField(fact, "directions", RequiredJsonKind::Array);

        if (fact.at("directions").size() != 2)
        {
            throw std::invalid_argument(
                "Recorded Wall Object directions must contain two entries");
        }

        for (const nlohmann::json& direction : fact.at("directions"))
        {
            ParseWallDirectionEntryDirection(direction);
            ParseWallDirectionEntryCollision(direction);
        }
    }
}

void ApplyRecordedSceneEntityFact(
    nlohmann::json& sceneEntities,
    const nlohmann::json& fact)
{
    ValidateRecordedSceneEntityFactShape(fact);

    const std::string key = SceneEntityKey(fact);

    if (!fact.at("present").get<bool>())
    {
        if (!sceneEntities.contains(key))
        {
            throw std::invalid_argument(
                "Projection Validity failed: scene entity absence before placement");
        }

        sceneEntities.erase(key);
        return;
    }

    sceneEntities[key] = fact;
}

void ValidateNoDuplicateSceneEntityFacts(const nlohmann::json& sceneEntityFacts)
{
    std::vector<std::string> keys;
    keys.reserve(sceneEntityFacts.size());

    for (const nlohmann::json& sceneEntityFact : sceneEntityFacts)
    {
        ValidateRecordedSceneEntityFactShape(sceneEntityFact);
        keys.push_back(SceneEntityKey(sceneEntityFact));
    }

    std::sort(keys.begin(), keys.end());

    for (std::size_t index = 1; index < keys.size(); ++index)
    {
        if (keys[index - 1] == keys[index])
        {
            throw std::invalid_argument(
                "Projection Validity failed: duplicate scene entity facts in tick");
        }
    }
}

void ValidateVersion2SceneEntityProjection(const EncounterRecording& recording)
{
    nlohmann::json sceneEntities = nlohmann::json::object();

    ValidateNoDuplicateSceneEntityFacts(
        recording.GetInitialFacts().at("sceneEntityFacts"));

    for (const nlohmann::json& sceneEntityFact :
         recording.GetInitialFacts().at("sceneEntityFacts"))
    {
        if (!IsFullRecordedSceneEntityFact(sceneEntityFact))
        {
            throw std::invalid_argument(
                "Projection Validity failed: initial scene entity facts must be full placements");
        }

        ApplyRecordedSceneEntityFact(sceneEntities, sceneEntityFact);
    }

    for (const CompletedRecordingTick& completedTick :
         recording.GetCompletedTicks())
    {
        ValidateNoDuplicateSceneEntityFacts(
            completedTick.facts.at("sceneEntityFacts"));

        for (const nlohmann::json& sceneEntityFact :
             completedTick.facts.at("sceneEntityFacts"))
        {
            if (sceneEntityFact.at("present").get<bool>() &&
                !IsFullRecordedSceneEntityFact(sceneEntityFact))
            {
                throw std::invalid_argument(
                    "Projection Validity failed: scene entity placement facts must be full");
            }

            ApplyRecordedSceneEntityFact(sceneEntities, sceneEntityFact);
        }
    }
}

void ValidateVersion2CombatProjection(const EncounterRecording& recording)
{
    std::unordered_set<unsigned long long> queuedDamageEventIds;

    if (recording.GetInitialFacts().contains("attacks") ||
        recording.GetInitialFacts().contains("damageApplications"))
    {
        throw std::invalid_argument(
            "Recording Validity failed: initial facts cannot contain combat facts");
    }

    for (const CompletedRecordingTick& completedTick :
         recording.GetCompletedTicks())
    {
        for (const nlohmann::json& attack :
             completedTick.facts.at("attacks"))
        {
            ValidateAttackShape(attack);

            if (attack.at("tick").get<int>() != completedTick.tick)
            {
                throw std::invalid_argument(
                    "Recording Validity failed: attack tick does not match completed tick");
            }

            const unsigned long long attackId =
                attack.at("id").get<unsigned long long>();

            for (const nlohmann::json& damageEvent :
                 attack.at("queuedDamageEvents"))
            {
                if (damageEvent.at("attackId").get<unsigned long long>() !=
                    attackId)
                {
                    throw std::invalid_argument(
                        "Recording Validity failed: queued damage attack id mismatch");
                }

                const unsigned long long damageEventId =
                    damageEvent.at("id").get<unsigned long long>();

                if (!queuedDamageEventIds.insert(damageEventId).second)
                {
                    throw std::invalid_argument(
                        "Projection Validity failed: duplicate queued damage event");
                }
            }
        }

        for (const nlohmann::json& damageApplication :
             completedTick.facts.at("damageApplications"))
        {
            ValidateDamageApplicationShape(damageApplication);

            if (damageApplication.at("tick").get<int>() != completedTick.tick)
            {
                throw std::invalid_argument(
                    "Recording Validity failed: damage application tick does not match completed tick");
            }

            const unsigned long long damageEventId =
                damageApplication.at("damageEventId").get<unsigned long long>();

            if (!queuedDamageEventIds.contains(damageEventId))
            {
                throw std::invalid_argument(
                    "Projection Validity failed: damage application references unknown queued damage event");
            }
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

void RecordingPlayback::RebuildToCurrentTick()
{
    m_Actors = nlohmann::json::object();
    m_SceneEntities = nlohmann::json::object();
    m_Attacks = nlohmann::json::array();
    m_DamageApplications = nlohmann::json::array();
    m_Projectiles = nlohmann::json::array();

    for (const nlohmann::json& actorFact :
         m_Recording.GetInitialFacts().at("actorFacts"))
    {
        ApplyRecordedActorFact(m_Actors, actorFact);
    }

    for (const nlohmann::json& sceneEntityFact :
         m_Recording.GetInitialFacts().at("sceneEntityFacts"))
    {
        ApplyRecordedSceneEntityFact(m_SceneEntities, sceneEntityFact);
    }

    if (m_CurrentTick == m_Recording.GetInitialTick())
    {
        m_Projectiles =
            m_Recording.GetInitialFacts().at("visibleProjectiles");
    }
    else
    {
        for (const CompletedRecordingTick& completedTick :
             m_Recording.GetCompletedTicks())
        {
            if (completedTick.tick > m_CurrentTick)
            {
                break;
            }

            for (const nlohmann::json& actorFact :
                 completedTick.facts.at("actorFacts"))
            {
                ApplyRecordedActorFact(m_Actors, actorFact);
            }

            for (const nlohmann::json& sceneEntityFact :
                 completedTick.facts.at("sceneEntityFacts"))
            {
                ApplyRecordedSceneEntityFact(m_SceneEntities, sceneEntityFact);
            }

            if (completedTick.tick == m_CurrentTick)
            {
                m_Attacks = completedTick.facts.at("attacks");
                m_DamageApplications =
                    completedTick.facts.at("damageApplications");
                m_Projectiles =
                    completedTick.facts.at("visibleProjectiles");
                break;
            }
        }
    }

    m_CurrentSnapshot = {
        {"tick", m_CurrentTick},
        {"actors", ActorsObjectToSortedArray(m_Actors)},
        {"sceneEntities", SceneEntitiesObjectToSortedArray(m_SceneEntities)},
        {"attacks", m_Attacks},
        {"damageApplications", m_DamageApplications},
        {"visibleProjectiles", m_Projectiles}};
}

RecordingPlayback::RecordingPlayback(EncounterRecording recording)
    : m_Recording(std::move(recording)),
      m_CurrentTick(m_Recording.GetInitialTick())
{
    RebuildToCurrentTick();
}

RecordingPlayback RecordingPlayback::LoadFromJson(const std::string& jsonText)
{
    EncounterRecording recording = EncounterRecording::LoadFromJson(jsonText);
    ValidateVersion2ActorProjection(recording);
    ValidateVersion2SceneEntityProjection(recording);
    ValidateVersion2CombatProjection(recording);
    return RecordingPlayback(std::move(recording));
}

std::string RecordingPlayback::GetEncounterName() const
{
    return m_Recording.GetEncounterName();
}

double RecordingPlayback::GetSecondsPerTick() const
{
    return m_Recording.GetSecondsPerTick();
}

int RecordingPlayback::GetInitialTick() const
{
    return m_Recording.GetInitialTick();
}

int RecordingPlayback::GetCurrentTick() const
{
    return m_CurrentTick;
}

int RecordingPlayback::GetLastTick() const
{
    return m_Recording.GetLastTick();
}

std::string RecordingPlayback::GetCurrentSnapshotJson() const
{
    return m_CurrentSnapshot.dump();
}

bool RecordingPlayback::Advance()
{
    if (m_CurrentTick >= GetLastTick())
    {
        return false;
    }

    ++m_CurrentTick;
    RebuildToCurrentTick();
    return true;
}

void RecordingPlayback::Reset()
{
    m_CurrentTick = GetInitialTick();
    RebuildToCurrentTick();
}

bool RecordingPlayback::IsComplete() const
{
    return m_CurrentTick == GetLastTick();
}

}  // namespace osrssim::recording
