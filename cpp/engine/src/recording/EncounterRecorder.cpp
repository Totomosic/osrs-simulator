#include "recording/EncounterRecorder.h"

#include "EquipmentDatabase.h"

#include <algorithm>
#include <stdexcept>
#include <tuple>
#include <utility>
#include <vector>

namespace osrssim::recording
{
namespace
{
nlohmann::json CreateCoordinateJson(SceneCoordinate coordinate)
{
    return nlohmann::json{
        {"x", coordinate.x},
        {"y", coordinate.y},
        {"plane", coordinate.plane}};
}

nlohmann::json CreateStatsJson(const CombatStats& stats)
{
    return nlohmann::json{
        {"attack", stats.attack},
        {"strength", stats.strength},
        {"defence", stats.defence},
        {"ranged", stats.ranged},
        {"magic", stats.magic},
        {"hitpoints", stats.hitpoints}};
}

nlohmann::json CreateBonusesJson(const EquipmentBonuses& bonuses)
{
    return nlohmann::json{
        {"stabAttack", bonuses.stabAttack},
        {"slashAttack", bonuses.slashAttack},
        {"crushAttack", bonuses.crushAttack},
        {"magicAttack", bonuses.magicAttack},
        {"rangedAttack", bonuses.rangedAttack},
        {"stabDefence", bonuses.stabDefence},
        {"slashDefence", bonuses.slashDefence},
        {"crushDefence", bonuses.crushDefence},
        {"magicDefence", bonuses.magicDefence},
        {"rangedDefenceLight", bonuses.rangedDefenceLight},
        {"rangedDefenceStandard", bonuses.rangedDefenceStandard},
        {"rangedDefenceHeavy", bonuses.rangedDefenceHeavy},
        {"meleeStrength", bonuses.meleeStrength},
        {"rangedStrength", bonuses.rangedStrength},
        {"magicDamagePercent", bonuses.magicDamagePercent}};
}

const char* EquipmentSlotName(EquipmentSlot slot)
{
    switch (slot)
    {
        case EquipmentSlot::Head:
            return "Head";
        case EquipmentSlot::Cape:
            return "Cape";
        case EquipmentSlot::Amulet:
            return "Amulet";
        case EquipmentSlot::Weapon:
            return "Weapon";
        case EquipmentSlot::Body:
            return "Body";
        case EquipmentSlot::Shield:
            return "Shield";
        case EquipmentSlot::Legs:
            return "Legs";
        case EquipmentSlot::Hands:
            return "Hands";
        case EquipmentSlot::Feet:
            return "Feet";
        case EquipmentSlot::Ring:
            return "Ring";
        case EquipmentSlot::Ammo:
            return "Ammo";
    }

    return "Unknown";
}

const char* AttackTypeName(AttackType attackType)
{
    switch (attackType)
    {
        case AttackType::Stab:
            return "Stab";
        case AttackType::Slash:
            return "Slash";
        case AttackType::Crush:
            return "Crush";
        case AttackType::Magic:
            return "Magic";
        case AttackType::RangedLight:
            return "RangedLight";
        case AttackType::RangedStandard:
            return "RangedStandard";
        case AttackType::RangedHeavy:
            return "RangedHeavy";
    }

    return "Unknown";
}

std::string ToSnakeCaseEnum(std::string value)
{
    if (value == "Player")
    {
        return "player";
    }

    if (value == "Npc")
    {
        return "npc";
    }

    if (value == "Stab")
    {
        return "stab";
    }

    if (value == "Slash")
    {
        return "slash";
    }

    if (value == "Crush")
    {
        return "crush";
    }

    if (value == "Magic")
    {
        return "magic";
    }

    if (value == "RangedLight")
    {
        return "ranged_light";
    }

    if (value == "RangedStandard")
    {
        return "ranged_standard";
    }

    if (value == "RangedHeavy")
    {
        return "ranged_heavy";
    }

    if (value == "SceneCoordinate")
    {
        return "scene_coordinate";
    }

    if (value == "Actor")
    {
        return "actor";
    }

    if (value == "GameObject")
    {
        return "game_object";
    }

    if (value == "WallObject")
    {
        return "wall_object";
    }

    if (value == "North")
    {
        return "north";
    }

    if (value == "East")
    {
        return "east";
    }

    if (value == "South")
    {
        return "south";
    }

    if (value == "West")
    {
        return "west";
    }

    return value;
}

nlohmann::json CreateVersion2MovementTargetJson(
    const nlohmann::json& movementTarget)
{
    if (movementTarget.is_null())
    {
        return nullptr;
    }

    nlohmann::json target = movementTarget;
    target["kind"] = ToSnakeCaseEnum(target.at("kind").get<std::string>());
    return target;
}

nlohmann::json CreateVersion2CombatCompositionJson(
    nlohmann::json composition)
{
    composition["attackType"] =
        ToSnakeCaseEnum(composition.at("attackType").get<std::string>());
    return composition;
}

nlohmann::json CreateVersion2ActorFact(const nlohmann::json& actor)
{
    nlohmann::json fact = {
        {"id", actor.at("id")},
        {"present", actor.value("present", true)}};

    if (!fact.at("present").get<bool>())
    {
        return fact;
    }

    if (actor.contains("kind"))
    {
        fact["kind"] = ToSnakeCaseEnum(actor.at("kind").get<std::string>());
    }

    if (actor.contains("playerIndex"))
    {
        fact["playerIndex"] = actor.at("playerIndex");
    }

    if (actor.contains("npcIndex"))
    {
        fact["npcIndex"] = actor.at("npcIndex");
    }

    if (actor.contains("sceneMembership"))
    {
        fact["sceneMembership"] = actor.at("sceneMembership");
    }

    if (actor.contains("size"))
    {
        fact["size"] = actor.at("size");
    }

    if (actor.contains("speed"))
    {
        fact["speed"] = actor.at("speed");
    }

    if (actor.contains("combatComposition"))
    {
        fact["combatComposition"] =
            CreateVersion2CombatCompositionJson(actor.at("combatComposition"));
        fact["currentHitpoints"] =
            actor.at("combatComposition").at("stats").at("hitpoints");
    }

    if (actor.contains("currentHitpoints"))
    {
        fact["currentHitpoints"] = actor.at("currentHitpoints");
    }

    if (actor.contains("debug"))
    {
        if (actor.at("debug").contains("movementTarget"))
        {
            fact["movementTarget"] = CreateVersion2MovementTargetJson(
                actor.at("debug").at("movementTarget"));
        }

        if (actor.at("debug").contains("attackTimer"))
        {
            fact["attackTimer"] = actor.at("debug").at("attackTimer");
        }
    }

    return fact;
}

nlohmann::json CreateVersion2ActorFacts(const nlohmann::json& actors)
{
    nlohmann::json actorFacts = nlohmann::json::array();

    for (const nlohmann::json& actor : actors)
    {
        actorFacts.push_back(CreateVersion2ActorFact(actor));
    }

    return actorFacts;
}

nlohmann::json CreateVersion2WallDirectionFacts(
    const nlohmann::json& directions)
{
    nlohmann::json directionFacts = nlohmann::json::array();

    for (const nlohmann::json& direction : directions)
    {
        directionFacts.push_back(
            {{"direction",
              ToSnakeCaseEnum(direction.at("direction").get<std::string>())},
             {"collision", direction.at("collision")}});
    }

    return directionFacts;
}

nlohmann::json CreateVersion2SceneEntityFact(
    const nlohmann::json& sceneEntity)
{
    nlohmann::json fact = {
        {"kind", ToSnakeCaseEnum(sceneEntity.at("kind").get<std::string>())},
        {"sceneId", sceneEntity.at("sceneId")},
        {"id", sceneEntity.at("id")},
        {"coordinate", sceneEntity.at("coordinate")},
        {"present", sceneEntity.value("present", true)}};

    if (!fact.at("present").get<bool>())
    {
        return fact;
    }

    fact["direction"] =
        ToSnakeCaseEnum(sceneEntity.at("direction").get<std::string>());
    fact["collision"] = sceneEntity.at("collision");

    if (sceneEntity.at("kind").get<std::string>() == "GameObject")
    {
        fact["sizeX"] = sceneEntity.at("sizeX");
        fact["sizeY"] = sceneEntity.at("sizeY");
    }
    else if (sceneEntity.contains("directions"))
    {
        fact["directions"] =
            CreateVersion2WallDirectionFacts(sceneEntity.at("directions"));
    }

    return fact;
}

nlohmann::json CreateVersion2SceneEntityFacts(
    const nlohmann::json& sceneEntities)
{
    nlohmann::json sceneEntityFacts = nlohmann::json::array();

    for (const nlohmann::json& sceneEntity : sceneEntities)
    {
        sceneEntityFacts.push_back(CreateVersion2SceneEntityFact(sceneEntity));
    }

    return sceneEntityFacts;
}

const char* CardinalDirectionName(CardinalDirection direction)
{
    switch (direction)
    {
        case CardinalDirection::North:
            return "North";
        case CardinalDirection::East:
            return "East";
        case CardinalDirection::South:
            return "South";
        case CardinalDirection::West:
            return "West";
    }

    return "Unknown";
}

nlohmann::json CreateCollisionProfileJson(
    const CollisionProfile& collisionProfile)
{
    return nlohmann::json{
        {"blocksMovement", collisionProfile.blocksMovement},
        {"blocksLineOfSight", collisionProfile.blocksLineOfSight}};
}

nlohmann::json CreateWallDirectionsJson(
    const SceneEntityPlacement& placement)
{
    nlohmann::json directions = nlohmann::json::array();

    for (int index = 0; index < placement.directionCount; ++index)
    {
        directions.push_back(
            {{"direction",
              CardinalDirectionName(
                  placement.directions[static_cast<std::size_t>(index)])},
             {"collision",
              CreateCollisionProfileJson(
                  placement
                      .collisionProfiles[static_cast<std::size_t>(index)])}});
    }

    return directions;
}

nlohmann::json CreateWeaponJson(const WeaponDefinition& weapon)
{
    return nlohmann::json{
        {"id", weapon.id},
        {"range", weapon.range},
        {"speed", weapon.speed},
        {"projectileId", weapon.projectileId}};
}

nlohmann::json CreateScenePositionJson(ScenePosition position)
{
    return nlohmann::json{
        {"x", position.x},
        {"y", position.y},
        {"plane", position.plane}};
}

nlohmann::json CreateEquipmentProvenanceJson(
    const std::vector<EquipmentPieceProvenance>& provenance)
{
    nlohmann::json provenanceJson = nlohmann::json::array();

    for (const EquipmentPieceProvenance& piece : provenance)
    {
        provenanceJson.push_back(
            {{"slot", EquipmentSlotName(piece.slot)},
             {"pieceId", piece.pieceId}});
    }

    return provenanceJson;
}

nlohmann::json CreateCombatCompositionJson(
    const CombatComposition& composition)
{
    nlohmann::json compositionJson = {
        {"stats", CreateStatsJson(composition.stats)},
        {"baseStats", CreateStatsJson(composition.baseStats)},
        {"bonuses", CreateBonusesJson(composition.bonuses)},
        {"attackType", AttackTypeName(composition.attackType)},
        {"magicBaseMaximumHit", composition.magicBaseMaximumHit},
        {"weapon", CreateWeaponJson(composition.weapon)}};

    if (!composition.equipmentProvenance.empty())
    {
        compositionJson["equipmentProvenance"] =
            CreateEquipmentProvenanceJson(composition.equipmentProvenance);
    }

    return compositionJson;
}

nlohmann::json CreateMovementTargetJson(
    const std::optional<MovementTarget>& movementTarget)
{
    if (!movementTarget.has_value())
    {
        return nullptr;
    }

    if (movementTarget->kind == MovementTargetKind::SceneCoordinate)
    {
        return nlohmann::json{
            {"kind", "SceneCoordinate"},
            {"coordinate", CreateCoordinateJson(movementTarget->sceneCoordinate)}};
    }

    return nlohmann::json{
        {"kind", "Actor"},
        {"actorId", movementTarget->actorId}};
}

nlohmann::json CreateSceneMembershipJson(
    const SceneMembership& membership)
{
    return nlohmann::json{
        {"sceneId", membership.sceneId},
        {"coordinate", CreateCoordinateJson(membership.coordinate)}};
}

std::vector<ActorId> GetSortedActorIds(
    const std::unordered_map<ActorId, nlohmann::json>& actors)
{
    std::vector<ActorId> actorIds;
    actorIds.reserve(actors.size());

    for (const auto& [actorId, actor] : actors)
    {
        actorIds.push_back(actorId);
    }

    std::sort(actorIds.begin(), actorIds.end());
    return actorIds;
}

std::string CreateSceneEntityKey(const nlohmann::json& sceneEntity)
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
    const std::unordered_map<std::string, nlohmann::json>& sceneEntities)
{
    std::vector<std::string> keys;
    keys.reserve(sceneEntities.size());

    for (const auto& [key, sceneEntity] : sceneEntities)
    {
        keys.push_back(key);
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
}  // namespace

nlohmann::json EncounterRecorder::CreateEmptyCompletedTick(int tick)
{
    return nlohmann::json{
        {"tick", tick},
        {"actorFacts", nlohmann::json::array()},
        {"sceneEntityFacts", nlohmann::json::array()},
        {"attacks", nlohmann::json::array()},
        {"damageApplications", nlohmann::json::array()},
        {"visibleProjectiles", nlohmann::json::array()}};
}

nlohmann::json EncounterRecorder::CreateActorSnapshot(
    const World& world,
    ActorId actorId)
{
    const ActorCore* actor = world.GetActorCore(actorId);
    const SceneMembership* membership = world.GetSceneMembership(actorId);

    if (actor == nullptr || membership == nullptr)
    {
        return nullptr;
    }

    const Player* player = world.GetPlayer(actorId);
    const Npc* npc = world.GetNpc(actorId);
    nlohmann::json snapshot = {
        {"id", actorId},
        {"kind", player != nullptr ? "Player" : "Npc"},
        {"present", true},
        {"sceneMembership", CreateSceneMembershipJson(*membership)},
        {"size", actor->size},
        {"speed", actor->speed},
        {"combatComposition",
         CreateCombatCompositionJson(actor->combatComposition)},
        {"debug",
         {{"movementTarget", nullptr}, {"attackTimer", actor->attackTimer}}}};

    if (player != nullptr)
    {
        snapshot["playerIndex"] = player->playerIndex;
        snapshot["debug"]["movementTarget"] =
            CreateMovementTargetJson(player->movementTarget);
    }
    else if (npc != nullptr)
    {
        snapshot["npcIndex"] = npc->npcIndex;
        snapshot["debug"]["movementTarget"] =
            CreateMovementTargetJson(npc->movementTarget);
    }

    return snapshot;
}

std::unordered_map<ActorId, nlohmann::json>
EncounterRecorder::CreatePlacedActors(const World& world)
{
    std::unordered_map<ActorId, nlohmann::json> actors;

    for (const auto& [actorId, membership] : world.GetSceneMemberships())
    {
        nlohmann::json snapshot = CreateActorSnapshot(world, actorId);

        if (!snapshot.is_null())
        {
            actors.emplace(actorId, std::move(snapshot));
        }
    }

    return actors;
}

nlohmann::json EncounterRecorder::CreateActorChanges(
    const std::unordered_map<ActorId, nlohmann::json>& previousActors,
    const std::unordered_map<ActorId, nlohmann::json>& currentActors)
{
    nlohmann::json changes = nlohmann::json::array();

    for (ActorId actorId : GetSortedActorIds(previousActors))
    {
        if (!currentActors.contains(actorId))
        {
            changes.push_back(
                {{"id", actorId}, {"present", false}});
        }
    }

    for (ActorId actorId : GetSortedActorIds(currentActors))
    {
        const nlohmann::json& current = currentActors.at(actorId);
        const auto previous = previousActors.find(actorId);

        if (previous == previousActors.end())
        {
            changes.push_back(current);
            continue;
        }

        const nlohmann::json& prior = previous->second;
        nlohmann::json change = {{"id", actorId}};

        if (current.at("sceneMembership") != prior.at("sceneMembership"))
        {
            change["sceneMembership"] = current.at("sceneMembership");
        }

        if (current.at("combatComposition") != prior.at("combatComposition"))
        {
            nlohmann::json currentComposition =
                current.at("combatComposition");
            nlohmann::json priorComposition = prior.at("combatComposition");
            const int currentHitpoints =
                currentComposition.at("stats").at("hitpoints").get<int>();

            currentComposition["stats"]["hitpoints"] =
                priorComposition.at("stats").at("hitpoints");

            if (currentComposition == priorComposition)
            {
                change["currentHitpoints"] = currentHitpoints;
            }
            else
            {
                change["combatComposition"] =
                    current.at("combatComposition");
            }
        }

        if (current.at("debug").at("movementTarget") !=
            prior.at("debug").at("movementTarget"))
        {
            change["debug"]["movementTarget"] =
                current.at("debug").at("movementTarget");
        }

        if (change.size() > 1)
        {
            change["debug"]["attackTimer"] =
                current.at("debug").at("attackTimer");
            changes.push_back(change);
        }
    }

    return changes;
}

std::unordered_map<std::string, nlohmann::json>
EncounterRecorder::CreateSceneEntities(const World& world)
{
    std::unordered_map<std::string, nlohmann::json> sceneEntities;

    const SceneId sceneId = world.GetDefaultSceneId();
    const Scene* scene = world.TryGetScene(sceneId);

    if (scene == nullptr)
    {
        return sceneEntities;
    }

    for (const SceneEntityPlacement& placement :
         scene->GetSceneEntityPlacements())
    {
        nlohmann::json sceneEntity = {
            {"kind",
             placement.kind == SceneEntityPlacement::Kind::GameObject
                 ? "GameObject"
                 : "WallObject"},
            {"sceneId", sceneId},
            {"id", placement.id},
            {"coordinate", CreateCoordinateJson(placement.coordinate)},
            {"direction", CardinalDirectionName(placement.direction)},
            {"collision", CreateCollisionProfileJson(placement.collisionProfile)}};

        if (placement.kind == SceneEntityPlacement::Kind::GameObject)
        {
            sceneEntity["sizeX"] = placement.sizeX;
            sceneEntity["sizeY"] = placement.sizeY;
        }
        else if (placement.directionCount > 1)
        {
            sceneEntity["directions"] = CreateWallDirectionsJson(placement);
        }

        sceneEntities.emplace(CreateSceneEntityKey(sceneEntity), sceneEntity);
    }

    return sceneEntities;
}

nlohmann::json EncounterRecorder::CreateSceneEntityChanges(
    const std::unordered_map<std::string, nlohmann::json>& previousSceneEntities,
    const std::unordered_map<std::string, nlohmann::json>& currentSceneEntities)
{
    nlohmann::json changes = nlohmann::json::array();

    for (const std::string& key : GetSortedSceneEntityKeys(previousSceneEntities))
    {
        if (!currentSceneEntities.contains(key))
        {
            nlohmann::json removal = previousSceneEntities.at(key);
            removal["present"] = false;
            changes.push_back(removal);
        }
    }

    for (const std::string& key : GetSortedSceneEntityKeys(currentSceneEntities))
    {
        if (!previousSceneEntities.contains(key))
        {
            nlohmann::json placement = currentSceneEntities.at(key);
            placement["present"] = true;
            changes.push_back(placement);
        }
    }

    return changes;
}

nlohmann::json EncounterRecorder::CreateProjectileJson(
    const ProjectileMetadata& projectile)
{
    return nlohmann::json{
        {"projectileId", projectile.projectileId},
        {"source", CreateScenePositionJson(projectile.source)},
        {"targetActorId", projectile.targetActorId},
        {"lastKnownTargetCenter",
         CreateScenePositionJson(projectile.lastKnownTargetCenter)},
        {"totalTicks", projectile.totalTicks}};
}

nlohmann::json EncounterRecorder::CreateProjectileSnapshotJson(
    const ProjectileSnapshot& projectile)
{
    return nlohmann::json{
        {"projectileId", projectile.projectileId},
        {"source", CreateScenePositionJson(projectile.source)},
        {"targetActorId", projectile.targetActorId},
        {"lastKnownTargetCenter",
         CreateScenePositionJson(projectile.lastKnownTargetCenter)},
        {"elapsedTicks", projectile.elapsedTicks},
        {"totalTicks", projectile.totalTicks}};
}

nlohmann::json EncounterRecorder::CreateProjectileSnapshots(const World& world)
{
    std::vector<ProjectileSnapshot> projectiles = world.GetProjectileSnapshots();
    std::sort(
        projectiles.begin(),
        projectiles.end(),
        [](const ProjectileSnapshot& left, const ProjectileSnapshot& right)
        {
            return std::tuple{
                       left.targetActorId,
                       left.projectileId,
                       left.source.plane,
                       left.source.x,
                       left.source.y} <
                   std::tuple{
                       right.targetActorId,
                       right.projectileId,
                       right.source.plane,
                       right.source.x,
                       right.source.y};
        });

    nlohmann::json projectileJson = nlohmann::json::array();

    for (const ProjectileSnapshot& projectile : projectiles)
    {
        projectileJson.push_back(CreateProjectileSnapshotJson(projectile));
    }

    return projectileJson;
}

EncounterRecorder::EncounterRecorder(
    std::string encounterName,
    double secondsPerTick)
    : m_EncounterName(std::move(encounterName)),
      m_SecondsPerTick(secondsPerTick)
{
    if (m_EncounterName.empty())
    {
        throw std::invalid_argument("Encounter Recording name cannot be empty");
    }

    if (m_SecondsPerTick <= 0.0)
    {
        throw std::invalid_argument(
            "Encounter Recording seconds per Tick must be positive");
    }
}

void EncounterRecorder::RecordInitialState(const Engine& engine)
{
    m_PreviousActors = CreatePlacedActors(engine.GetWorld());
    m_PreviousSceneEntities = CreateSceneEntities(engine.GetWorld());
    nlohmann::json actors = nlohmann::json::array();
    nlohmann::json sceneEntities = nlohmann::json::array();

    for (ActorId actorId : GetSortedActorIds(m_PreviousActors))
    {
        actors.push_back(m_PreviousActors.at(actorId));
    }

    for (const std::string& key : GetSortedSceneEntityKeys(m_PreviousSceneEntities))
    {
        sceneEntities.push_back(m_PreviousSceneEntities.at(key));
    }

    m_Recording = nlohmann::json{
        {"version", 2},
        {"metadata",
         {{"encounterName", m_EncounterName},
          {"secondsPerTick", m_SecondsPerTick}}},
        {"initialTick", static_cast<int>(engine.GetCurrentTick())},
        {"initialFacts",
         {{"actorFacts", CreateVersion2ActorFacts(actors)},
          {"sceneEntityFacts", CreateVersion2SceneEntityFacts(sceneEntities)},
          {"visibleProjectiles", CreateProjectileSnapshots(engine.GetWorld())}}},
        {"completedTicks", nlohmann::json::array()}};
}

void EncounterRecorder::RecordCompletedTick(const Engine& engine)
{
    if (m_Recording.is_null())
    {
        throw std::logic_error(
            "Encounter Recording must record initial state before Ticks");
    }

    const std::unordered_map<ActorId, nlohmann::json> currentActors =
        CreatePlacedActors(engine.GetWorld());
    const std::unordered_map<std::string, nlohmann::json> currentSceneEntities =
        CreateSceneEntities(engine.GetWorld());
    const nlohmann::json actorChanges =
        CreateActorChanges(m_PreviousActors, currentActors);
    const nlohmann::json sceneEntityChanges =
        CreateSceneEntityChanges(m_PreviousSceneEntities, currentSceneEntities);
    nlohmann::json tick =
        CreateEmptyCompletedTick(static_cast<int>(engine.GetCurrentTick()));
    tick["actorFacts"] = CreateVersion2ActorFacts(actorChanges);
    tick["attacks"] = m_PendingAttacks;
    tick["damageApplications"] = m_PendingDamageApplications;
    tick["sceneEntityFacts"] = CreateVersion2SceneEntityFacts(
        sceneEntityChanges);
    tick["visibleProjectiles"] = CreateProjectileSnapshots(engine.GetWorld());
    m_PreviousActors = currentActors;
    m_PreviousSceneEntities = currentSceneEntities;
    m_PendingAttacks = nlohmann::json::array();
    m_PendingDamageApplications = nlohmann::json::array();
    m_Recording["completedTicks"].push_back(tick);
}

std::string EncounterRecorder::ExportJson() const
{
    if (m_Recording.is_null())
    {
        throw std::logic_error(
            "Encounter Recording cannot export before initial state");
    }

    return m_Recording.dump();
}

void EncounterRecorder::OnAttackQueued(
    const CombatService::AttackObservation& attack)
{
    nlohmann::json queuedDamageEvents = nlohmann::json::array();

    for (const CombatService::QueuedDamageEventObservation& damageEvent :
         attack.queuedDamageEvents)
    {
        m_RecordedQueuedDamageEventIds.insert(damageEvent.id);
        queuedDamageEvents.push_back(
            {{"id", damageEvent.id},
             {"attackId", damageEvent.attackId},
             {"targetId", damageEvent.targetId},
             {"damage", damageEvent.damage},
             {"delayTicks", damageEvent.delayTicks}});
    }

    nlohmann::json attackJson = {
        {"id", attack.id},
        {"tick", attack.tick},
        {"attackerId", attack.attackerId},
        {"targetId", attack.targetId},
        {"callback", attack.callback},
        {"queuedDamageEvents", queuedDamageEvents}};

    if (attack.projectile.has_value())
    {
        attackJson["projectile"] = CreateProjectileJson(attack.projectile.value());
    }

    m_PendingAttacks.push_back(attackJson);
}

void EncounterRecorder::OnDamageApplied(
    const CombatService::DamageApplicationObservation& damageApplication)
{
    if (!m_RecordedQueuedDamageEventIds.contains(
            damageApplication.damageEventId))
    {
        return;
    }

    m_PendingDamageApplications.push_back(
        {{"damageEventId", damageApplication.damageEventId},
         {"attackId", damageApplication.attackId},
         {"tick", damageApplication.tick},
         {"targetId", damageApplication.targetId},
         {"queuedDamage", damageApplication.queuedDamage},
         {"appliedDamage", damageApplication.appliedDamage}});
}

}  // namespace osrssim::recording
