#include "Engine.h"
#include "DatabaseService.h"
#include "DpsService.h"
#include "EquipmentDatabase.h"
#include "EquipmentSet.h"
#include "LineOfSight.h"
#include "Scene.h"
#include "Tile.h"
#include "World.h"

#include <emscripten/bind.h>
#include <emscripten/val.h>

#include <sstream>
#include <string>
#include <vector>

namespace
{

osrssim::World& GetEngineWorld(osrssim::Engine& engine)
{
    return engine.GetWorld();
}

osrssim::Scene* TryGetWorldScene(osrssim::World& world, osrssim::SceneId sceneId)
{
    return world.TryGetScene(sceneId);
}

bool PlaceSceneGameObject(
    osrssim::Scene& scene,
    osrssim::SceneCoordinate coordinate,
    osrssim::EntityId id,
    osrssim::CardinalDirection direction,
    int sizeX,
    int sizeY,
    const osrssim::CollisionProfile& collisionProfile)
{
    return scene.PlaceGameObject(
        coordinate,
        id,
        direction,
        sizeX,
        sizeY,
        collisionProfile);
}

bool IsGameObjectTile(
    const osrssim::Scene& scene,
    osrssim::SceneCoordinate coordinate)
{
    const osrssim::Tile* tile = scene.TryGetTile(coordinate);
    return tile != nullptr && tile->gameObject.has_value();
}

bool HasSceneLineOfSight(
    const osrssim::Scene& scene,
    osrssim::SceneCoordinate sourceAnchor,
    int sourceActorSize,
    osrssim::SceneCoordinate target,
    int range)
{
    osrssim::LineOfSight lineOfSight(scene);
    return lineOfSight.HasLineOfSight(
        sourceAnchor,
        sourceActorSize,
        target,
        range);
}

bool HasSceneActorLineOfSight(
    const osrssim::Scene& scene,
    osrssim::SceneCoordinate sourceAnchor,
    int sourceActorSize,
    osrssim::SceneCoordinate targetAnchor,
    int targetActorSize,
    int range)
{
    osrssim::LineOfSight lineOfSight(scene);
    return lineOfSight.HasLineOfSight(
        sourceAnchor,
        sourceActorSize,
        targetAnchor,
        targetActorSize,
        range);
}

emscripten::val GetTileFlagLabels(
    const osrssim::Scene& scene,
    osrssim::SceneCoordinate coordinate)
{
    const osrssim::Tile* tile = scene.TryGetTile(coordinate);
    emscripten::val labels = emscripten::val::array();
    int nextIndex = 0;

    if (tile == nullptr)
    {
        return labels;
    }

    auto appendFlag = [&](osrssim::TileFlag flag, const char* label)
    {
        if (!tile->HasFlag(flag))
        {
            return;
        }

        labels.set(nextIndex, label);
        ++nextIndex;
    };

    appendFlag(osrssim::TileFlag::Occupied, "Occupied");
    appendFlag(osrssim::TileFlag::BlockMovementNorthWest, "BlockMovementNorthWest");
    appendFlag(osrssim::TileFlag::BlockMovementNorth, "BlockMovementNorth");
    appendFlag(osrssim::TileFlag::BlockMovementNorthEast, "BlockMovementNorthEast");
    appendFlag(osrssim::TileFlag::BlockMovementEast, "BlockMovementEast");
    appendFlag(osrssim::TileFlag::BlockMovementSouthEast, "BlockMovementSouthEast");
    appendFlag(osrssim::TileFlag::BlockMovementSouth, "BlockMovementSouth");
    appendFlag(osrssim::TileFlag::BlockMovementSouthWest, "BlockMovementSouthWest");
    appendFlag(osrssim::TileFlag::BlockMovementWest, "BlockMovementWest");
    appendFlag(osrssim::TileFlag::BlockMovementFull, "BlockMovementFull");
    appendFlag(osrssim::TileFlag::BlockMovementObject, "BlockMovementObject");
    appendFlag(osrssim::TileFlag::BlockMovementFloor, "BlockMovementFloor");
    appendFlag(
        osrssim::TileFlag::BlockMovementFloorDecoration,
        "BlockMovementFloorDecoration");
    appendFlag(osrssim::TileFlag::BlockLineOfSightNorth, "BlockLineOfSightNorth");
    appendFlag(osrssim::TileFlag::BlockLineOfSightEast, "BlockLineOfSightEast");
    appendFlag(osrssim::TileFlag::BlockLineOfSightSouth, "BlockLineOfSightSouth");
    appendFlag(osrssim::TileFlag::BlockLineOfSightWest, "BlockLineOfSightWest");
    appendFlag(osrssim::TileFlag::BlockLineOfSightFull, "BlockLineOfSightFull");

    return labels;
}

void AppendCoordinateJson(
    std::ostringstream& output,
    osrssim::SceneCoordinate coordinate)
{
    output << "{\"x\":" << coordinate.x << ",\"y\":" << coordinate.y
           << ",\"plane\":" << coordinate.plane << "}";
}

void AppendActorMovementTargetJson(
    std::ostringstream& output,
    const std::optional<osrssim::MovementTarget>& movementTarget)
{
    if (!movementTarget.has_value())
    {
        output << "null";
        return;
    }

    if (movementTarget->kind == osrssim::MovementTargetKind::SceneCoordinate)
    {
        output << "{\"kind\":\"SceneCoordinate\",\"coordinate\":";
        AppendCoordinateJson(output, movementTarget->sceneCoordinate);
        output << "}";
        return;
    }

    output << "{\"kind\":\"Actor\",\"actorId\":" << movementTarget->actorId
           << "}";
}

void AppendWeaponDefinitionJson(
    std::ostringstream& output,
    const osrssim::WeaponDefinition& weaponDefinition)
{
    output << "{\"id\":" << weaponDefinition.id
           << ",\"range\":" << weaponDefinition.range
           << ",\"speed\":" << weaponDefinition.speed << "}";
}

std::string GetActorSnapshotJson(
    const osrssim::World& world,
    osrssim::ActorId actorId)
{
    const osrssim::ActorCore* actor = world.GetActorCore(actorId);
    const osrssim::SceneMembership* membership =
        world.GetSceneMembership(actorId);

    if (actor == nullptr || membership == nullptr)
    {
        return "";
    }

    const osrssim::Player* player = world.GetPlayer(actorId);
    const osrssim::Npc* npc = world.GetNpc(actorId);
    std::ostringstream output;

    output << "{\"id\":" << actorId << ",\"kind\":\""
           << (player != nullptr ? "Player" : "NPC") << "\",\"coordinate\":";
    AppendCoordinateJson(output, membership->coordinate);
    output << ",\"size\":" << actor->size << ",\"speed\":" << actor->speed
           << ",\"weapon\":";
    AppendWeaponDefinitionJson(output, actor->weapon);
    output << ",\"attackTimer\":" << actor->attackTimer
           << ",\"movementTarget\":";

    if (player != nullptr)
    {
        AppendActorMovementTargetJson(output, player->movementTarget);
    }
    else if (npc != nullptr)
    {
        AppendActorMovementTargetJson(output, npc->movementTarget);
    }
    else
    {
        output << "null";
    }

    output << "}";
    return output.str();
}

emscripten::val GetActorSnapshot(
    const osrssim::World& world,
    osrssim::ActorId actorId)
{
    const std::string snapshot = GetActorSnapshotJson(world, actorId);

    if (snapshot.empty())
    {
        return emscripten::val::null();
    }

    return emscripten::val(snapshot);
}

std::string GetDefaultEquipmentJson()
{
    return osrssim::EquipmentDatabase::GetDefaultJson();
}

osrssim::EquipmentDatabase GetDatabaseServiceEquipmentDatabase(
    const osrssim::DatabaseService& service)
{
    return service.GetEquipmentDatabase();
}

}  // namespace

EMSCRIPTEN_BINDINGS(osrssim_engine)
{
    emscripten::register_vector<osrssim::EquipmentPiece>(
        "EquipmentPieceVector");

    emscripten::value_object<osrssim::SceneCoordinate>("SceneCoordinate")
        .field("x", &osrssim::SceneCoordinate::x)
        .field("y", &osrssim::SceneCoordinate::y)
        .field("plane", &osrssim::SceneCoordinate::plane);

    emscripten::value_object<osrssim::CollisionProfile>("CollisionProfile")
        .field("blocksMovement", &osrssim::CollisionProfile::blocksMovement)
        .field(
            "blocksLineOfSight",
            &osrssim::CollisionProfile::blocksLineOfSight);

    emscripten::value_object<osrssim::WeaponDefinition>("WeaponDefinition")
        .field("id", &osrssim::WeaponDefinition::id)
        .field("range", &osrssim::WeaponDefinition::range)
        .field("speed", &osrssim::WeaponDefinition::speed);

    emscripten::enum_<osrssim::CardinalDirection>("CardinalDirection")
        .value("North", osrssim::CardinalDirection::North)
        .value("East", osrssim::CardinalDirection::East)
        .value("South", osrssim::CardinalDirection::South)
        .value("West", osrssim::CardinalDirection::West);

    emscripten::enum_<osrssim::AttackType>("AttackType")
        .value("Stab", osrssim::AttackType::Stab)
        .value("Slash", osrssim::AttackType::Slash)
        .value("Crush", osrssim::AttackType::Crush)
        .value("Magic", osrssim::AttackType::Magic)
        .value("RangedLight", osrssim::AttackType::RangedLight)
        .value("RangedStandard", osrssim::AttackType::RangedStandard)
        .value("RangedHeavy", osrssim::AttackType::RangedHeavy);

    emscripten::enum_<osrssim::DefenderKind>("DefenderKind")
        .value("Player", osrssim::DefenderKind::Player)
        .value("Npc", osrssim::DefenderKind::Npc);

    emscripten::enum_<osrssim::EquipmentSlot>("EquipmentSlot")
        .value("Head", osrssim::EquipmentSlot::Head)
        .value("Cape", osrssim::EquipmentSlot::Cape)
        .value("Amulet", osrssim::EquipmentSlot::Amulet)
        .value("Weapon", osrssim::EquipmentSlot::Weapon)
        .value("Body", osrssim::EquipmentSlot::Body)
        .value("Shield", osrssim::EquipmentSlot::Shield)
        .value("Legs", osrssim::EquipmentSlot::Legs)
        .value("Hands", osrssim::EquipmentSlot::Hands)
        .value("Feet", osrssim::EquipmentSlot::Feet)
        .value("Ring", osrssim::EquipmentSlot::Ring)
        .value("Ammo", osrssim::EquipmentSlot::Ammo);

    emscripten::value_object<osrssim::CombatStats>("CombatStats")
        .field("attack", &osrssim::CombatStats::attack)
        .field("strength", &osrssim::CombatStats::strength)
        .field("defence", &osrssim::CombatStats::defence)
        .field("ranged", &osrssim::CombatStats::ranged)
        .field("magic", &osrssim::CombatStats::magic)
        .field("hitpoints", &osrssim::CombatStats::hitpoints);

    emscripten::value_object<osrssim::EquipmentBonuses>("EquipmentBonuses")
        .field("stabAttack", &osrssim::EquipmentBonuses::stabAttack)
        .field("slashAttack", &osrssim::EquipmentBonuses::slashAttack)
        .field("crushAttack", &osrssim::EquipmentBonuses::crushAttack)
        .field("magicAttack", &osrssim::EquipmentBonuses::magicAttack)
        .field("rangedAttack", &osrssim::EquipmentBonuses::rangedAttack)
        .field("stabDefence", &osrssim::EquipmentBonuses::stabDefence)
        .field("slashDefence", &osrssim::EquipmentBonuses::slashDefence)
        .field("crushDefence", &osrssim::EquipmentBonuses::crushDefence)
        .field("magicDefence", &osrssim::EquipmentBonuses::magicDefence)
        .field(
            "rangedDefenceLight",
            &osrssim::EquipmentBonuses::rangedDefenceLight)
        .field(
            "rangedDefenceStandard",
            &osrssim::EquipmentBonuses::rangedDefenceStandard)
        .field(
            "rangedDefenceHeavy",
            &osrssim::EquipmentBonuses::rangedDefenceHeavy)
        .field("meleeStrength", &osrssim::EquipmentBonuses::meleeStrength)
        .field("rangedStrength", &osrssim::EquipmentBonuses::rangedStrength)
        .field(
            "magicDamagePercent",
            &osrssim::EquipmentBonuses::magicDamagePercent);

    emscripten::value_object<osrssim::StyleBonus>("StyleBonus")
        .field("attack", &osrssim::StyleBonus::attack)
        .field("strength", &osrssim::StyleBonus::strength)
        .field("defence", &osrssim::StyleBonus::defence)
        .field("ranged", &osrssim::StyleBonus::ranged)
        .field("magic", &osrssim::StyleBonus::magic);

    emscripten::value_object<osrssim::AttackComposition>(
        "AttackComposition")
        .field("attackType", &osrssim::AttackComposition::attackType)
        .field("stats", &osrssim::AttackComposition::stats)
        .field("bonuses", &osrssim::AttackComposition::bonuses)
        .field("weapon", &osrssim::AttackComposition::weapon);

    emscripten::value_object<osrssim::DefenceComposition>(
        "DefenceComposition")
        .field("stats", &osrssim::DefenceComposition::stats)
        .field("bonuses", &osrssim::DefenceComposition::bonuses);

    emscripten::value_object<osrssim::EquipmentPiece>("EquipmentPiece")
        .field("id", &osrssim::EquipmentPiece::id)
        .field("name", &osrssim::EquipmentPiece::name)
        .field("slot", &osrssim::EquipmentPiece::slot)
        .field("bonuses", &osrssim::EquipmentPiece::bonuses)
        .field("hasWeapon", &osrssim::EquipmentPiece::hasWeapon)
        .field("weapon", &osrssim::EquipmentPiece::weapon);

    emscripten::value_object<osrssim::DpsRequest>("DpsRequest")
        .field(
            "attackComposition",
            &osrssim::DpsRequest::attackComposition)
        .field(
            "defenceComposition",
            &osrssim::DpsRequest::defenceComposition)
        .field("attackerStyle", &osrssim::DpsRequest::attackerStyle)
        .field("defenderStyle", &osrssim::DpsRequest::defenderStyle)
        .field("defenderKind", &osrssim::DpsRequest::defenderKind)
        .field(
            "attackPrayerMultiplier",
            &osrssim::DpsRequest::attackPrayerMultiplier)
        .field(
            "strengthPrayerMultiplier",
            &osrssim::DpsRequest::strengthPrayerMultiplier)
        .field(
            "defencePrayerMultiplier",
            &osrssim::DpsRequest::defencePrayerMultiplier)
        .field(
            "attackLevelMultiplier",
            &osrssim::DpsRequest::attackLevelMultiplier)
        .field(
            "strengthLevelMultiplier",
            &osrssim::DpsRequest::strengthLevelMultiplier)
        .field(
            "defenceLevelMultiplier",
            &osrssim::DpsRequest::defenceLevelMultiplier)
        .field(
            "finalAttackRollMultiplier",
            &osrssim::DpsRequest::finalAttackRollMultiplier)
        .field(
            "finalDefenceRollMultiplier",
            &osrssim::DpsRequest::finalDefenceRollMultiplier)
        .field(
            "finalDamageMultiplier",
            &osrssim::DpsRequest::finalDamageMultiplier)
        .field(
            "magicBaseMaximumHit",
            &osrssim::DpsRequest::magicBaseMaximumHit);

    emscripten::value_object<osrssim::DpsResult>("DpsResult")
        .field("attackRoll", &osrssim::DpsResult::attackRoll)
        .field("defenceRoll", &osrssim::DpsResult::defenceRoll)
        .field("maximumHit", &osrssim::DpsResult::maximumHit)
        .field("hitChance", &osrssim::DpsResult::hitChance)
        .field(
            "expectedDamagePerAttack",
            &osrssim::DpsResult::expectedDamagePerAttack)
        .field("secondsPerAttack", &osrssim::DpsResult::secondsPerAttack)
        .field("dps", &osrssim::DpsResult::dps);

    emscripten::value_object<osrssim::DpsSampleResult>("DpsSampleResult")
        .field("attackRoll", &osrssim::DpsSampleResult::attackRoll)
        .field("defenceRoll", &osrssim::DpsSampleResult::defenceRoll)
        .field("maximumHit", &osrssim::DpsSampleResult::maximumHit)
        .field("hitChance", &osrssim::DpsSampleResult::hitChance)
        .field(
            "expectedDamagePerAttack",
            &osrssim::DpsSampleResult::expectedDamagePerAttack)
        .field("secondsPerAttack", &osrssim::DpsSampleResult::secondsPerAttack)
        .field("dps", &osrssim::DpsSampleResult::dps)
        .field("accuracyPassed", &osrssim::DpsSampleResult::accuracyPassed)
        .field("sampledDamage", &osrssim::DpsSampleResult::sampledDamage);

    emscripten::value_object<osrssim::DpsSampleAggregateResult>(
        "DpsSampleAggregateResult")
        .field("attackCount", &osrssim::DpsSampleAggregateResult::attackCount)
        .field(
            "totalSampledDamage",
            &osrssim::DpsSampleAggregateResult::totalSampledDamage)
        .field(
            "averageSampledDamagePerAttack",
            &osrssim::DpsSampleAggregateResult::averageSampledDamagePerAttack)
        .field("sampledDps", &osrssim::DpsSampleAggregateResult::sampledDps);

    emscripten::class_<osrssim::DpsService>("DpsService")
        .constructor<>()
        .function("CalculateExpected", &osrssim::DpsService::CalculateExpected)
        .function("SetSeed", &osrssim::DpsService::SetSeed)
        .function(
            "SampleSingleAttack",
            emscripten::select_overload<osrssim::DpsSampleResult(
                const osrssim::DpsRequest&)>(
                &osrssim::DpsService::SampleSingleAttack))
        .function(
            "SampleSingleAttackWithSeed",
            emscripten::select_overload<osrssim::DpsSampleResult(
                const osrssim::DpsRequest&,
                unsigned int) const>(&osrssim::DpsService::SampleSingleAttack))
        .function(
            "SampleAttacks",
            emscripten::select_overload<osrssim::DpsSampleAggregateResult(
                const osrssim::DpsRequest&,
                int)>(&osrssim::DpsService::SampleAttacks))
        .function(
            "SampleAttacksWithSeed",
            emscripten::select_overload<osrssim::DpsSampleAggregateResult(
                const osrssim::DpsRequest&,
                int,
                unsigned int) const>(&osrssim::DpsService::SampleAttacks))
        .class_function(
            "CalculateEffectiveLevel",
            &osrssim::DpsService::CalculateEffectiveLevel)
        .class_function(
            "CalculateAttackRoll",
            &osrssim::DpsService::CalculateAttackRoll)
        .class_function(
            "CalculateDefenceRoll",
            &osrssim::DpsService::CalculateDefenceRoll)
        .class_function(
            "CalculateStandardMaximumHit",
            &osrssim::DpsService::CalculateStandardMaximumHit)
        .class_function(
            "CalculateHitChance",
            &osrssim::DpsService::CalculateHitChance);

    emscripten::class_<osrssim::EquipmentDatabase>("EquipmentDatabase")
        .constructor<>()
        .class_function(
            "LoadFromJson",
            &osrssim::EquipmentDatabase::LoadFromJson)
        .class_function(
            "LoadDefault",
            &osrssim::EquipmentDatabase::LoadDefault)
        .class_function("GetDefaultJson", &GetDefaultEquipmentJson)
        .function(
            "HasEquipmentPiece",
            &osrssim::EquipmentDatabase::HasEquipmentPiece)
        .function(
            "GetEquipmentPiece",
            &osrssim::EquipmentDatabase::GetEquipmentPiece)
        .function(
            "GetAllEquipmentPieces",
            &osrssim::EquipmentDatabase::GetAllEquipmentPieces)
        .function(
            "GetEquipmentPiecesBySlot",
            &osrssim::EquipmentDatabase::GetEquipmentPiecesBySlot);

    emscripten::class_<osrssim::DatabaseService>("DatabaseService")
        .constructor<>()
        .class_function(
            "LoadFromJsonDocuments",
            &osrssim::DatabaseService::LoadFromJsonDocuments)
        .function(
            "GetEquipmentDatabase",
            &GetDatabaseServiceEquipmentDatabase);

    emscripten::class_<osrssim::EquipmentSet>("EquipmentSet")
        .constructor<>()
        .function(
            "SetEquipmentPiece",
            &osrssim::EquipmentSet::SetEquipmentPiece)
        .function(
            "HasEquipmentPiece",
            &osrssim::EquipmentSet::HasEquipmentPiece)
        .function(
            "GetEquipmentPiece",
            &osrssim::EquipmentSet::GetEquipmentPiece)
        .function(
            "GetEquipmentPieces",
            &osrssim::EquipmentSet::GetEquipmentPieces)
        .function(
            "GetEquipmentBonuses",
            &osrssim::EquipmentSet::GetEquipmentBonuses)
        .function(
            "BuildAttackComposition",
            &osrssim::EquipmentSet::BuildAttackComposition)
        .function(
            "BuildDefenceComposition",
            &osrssim::EquipmentSet::BuildDefenceComposition);

    emscripten::class_<osrssim::Scene>("Scene")
        .function("PlaceGameObject", &PlaceSceneGameObject)
        .function("RemoveGameObject", &osrssim::Scene::RemoveGameObject)
        .function("IsGameObjectTile", &IsGameObjectTile)
        .function("HasLineOfSight", &HasSceneLineOfSight)
        .function("HasActorLineOfSight", &HasSceneActorLineOfSight)
        .function("GetTileFlagLabels", &GetTileFlagLabels);

    emscripten::class_<osrssim::World>("World")
        .constructor<>()
        .function("GetDefaultSceneId", &osrssim::World::GetDefaultSceneId)
        .function(
            "TryGetScene",
            &TryGetWorldScene,
            emscripten::return_value_policy::reference(),
            emscripten::allow_raw_pointers())
        .function("CreatePlayer", &osrssim::World::CreatePlayer)
        .function("CreateNpc", &osrssim::World::CreateNpc)
        .function("PlaceActor", &osrssim::World::PlaceActor)
        .function("RemoveActor", &osrssim::World::RemoveActor)
        .function(
            "SetActorWeaponDefinition",
            &osrssim::World::SetActorWeaponDefinition)
        .function(
            "SetActorMovementTarget",
            &osrssim::World::SetActorMovementTarget)
        .function(
            "SetPlayerSceneCoordinateMovementTarget",
            &osrssim::World::SetPlayerSceneCoordinateMovementTarget)
        .function(
            "CanPlayerUseSceneCoordinateMovementTarget",
            &osrssim::World::CanPlayerUseSceneCoordinateMovementTarget)
        .function("GetActorSnapshot", &GetActorSnapshot);

    emscripten::class_<osrssim::Engine>("Engine")
        .constructor<>()
        .function("Step", &osrssim::Engine::Step)
        .function("GetCurrentTick", &osrssim::Engine::GetCurrentTick)
        .function(
            "GetWorld",
            &GetEngineWorld,
            emscripten::return_value_policy::reference());

}
