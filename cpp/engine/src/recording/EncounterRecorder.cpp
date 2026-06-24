#include "recording/EncounterRecorder.h"

#include <algorithm>
#include <stdexcept>
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

nlohmann::json CreateWeaponJson(const WeaponDefinition& weapon)
{
    return nlohmann::json{
        {"id", weapon.id},
        {"range", weapon.range},
        {"speed", weapon.speed},
        {"projectileId", weapon.projectileId}};
}

nlohmann::json CreateCombatCompositionJson(
    const CombatComposition& composition)
{
    return nlohmann::json{
        {"stats", CreateStatsJson(composition.stats)},
        {"baseStats", CreateStatsJson(composition.baseStats)},
        {"bonuses", CreateBonusesJson(composition.bonuses)},
        {"attackType", AttackTypeName(composition.attackType)},
        {"magicBaseMaximumHit", composition.magicBaseMaximumHit},
        {"weapon", CreateWeaponJson(composition.weapon)}};
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
}  // namespace

nlohmann::json EncounterRecorder::CreateEmptyTick(int tick)
{
    return nlohmann::json{
        {"tick", tick},
        {"actors", nlohmann::json::array()},
        {"attacks", nlohmann::json::array()},
        {"damageApplications", nlohmann::json::array()},
        {"sceneChanges", nlohmann::json::array()},
        {"projectiles", nlohmann::json::array()}};
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
    nlohmann::json actors = nlohmann::json::array();

    for (ActorId actorId : GetSortedActorIds(m_PreviousActors))
    {
        actors.push_back(m_PreviousActors.at(actorId));
    }

    m_Recording = nlohmann::json{
        {"version", 1},
        {"metadata",
         {{"encounterName", m_EncounterName},
          {"secondsPerTick", m_SecondsPerTick}}},
        {"initialState",
         {{"tick", static_cast<int>(engine.GetCurrentTick())},
          {"actors", actors},
          {"sceneEntities", nlohmann::json::array()},
          {"projectiles", nlohmann::json::array()}}},
        {"ticks", nlohmann::json::array()}};
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
    nlohmann::json tick = CreateEmptyTick(static_cast<int>(engine.GetCurrentTick()));
    tick["actors"] = CreateActorChanges(m_PreviousActors, currentActors);
    m_PreviousActors = currentActors;
    m_Recording["ticks"].push_back(tick);
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

}  // namespace osrssim::recording
