#include "encounter/ActiveEncounter.h"
#include "encounter/EncounterRunner.h"
#include "EquipmentSet.h"
#include "recording/EncounterRecorder.h"
#include "recording/RecordingPlayback.h"
#include "WeaponDatabase.h"

#include <nlohmann/json.hpp>

#include <cassert>
#include <memory>
#include <stdexcept>

namespace
{
class EmptyEncounter : public osrssim::encounter::ActiveEncounter
{
public:
    void Initialize(osrssim::EncounterContext&) override
    {
    }

    bool IsComplete(const osrssim::EncounterContext&) const override
    {
        return false;
    }
};

osrssim::CombatComposition CreateCombatComposition(int hitpoints, int baseHitpoints)
{
    osrssim::CombatComposition composition;
    composition.stats.attack = 70;
    composition.stats.strength = 71;
    composition.stats.defence = 72;
    composition.stats.ranged = 73;
    composition.stats.magic = 74;
    composition.stats.hitpoints = hitpoints;
    composition.baseStats.hitpoints = baseHitpoints;
    composition.bonuses.slashAttack = 15;
    composition.attackType = osrssim::AttackType::Slash;
    composition.magicBaseMaximumHit = 9;
    composition.weapon = {12, 4, 6, 88};
    return composition;
}

osrssim::CombatComposition CreateEquipmentSetCombatComposition()
{
    const osrssim::WeaponDatabase weaponDatabase =
        osrssim::WeaponDatabase::LoadFromJson(R"({
            "version": 1,
            "weapons": [
                {
                    "id": 0,
                    "name": "Unarmed",
                    "range": 1,
                    "speed": 4,
                    "attackCallbackName": "standard_attack"
                },
                {
                    "id": 12,
                    "name": "Sample bow",
                    "range": 4,
                    "speed": 6,
                    "projectileId": 88,
                    "attackCallbackName": "standard_attack"
                }
            ]
        })");

    osrssim::CombatStats stats;
    stats.attack = 70;
    stats.strength = 71;
    stats.defence = 72;
    stats.ranged = 73;
    stats.magic = 74;
    stats.hitpoints = 82;

    osrssim::EquipmentBonuses amuletBonuses;
    amuletBonuses.slashAttack = 15;

    osrssim::EquipmentSet equipmentSet;
    osrssim::EquipmentPiece amulet;
    amulet.id = 2001;
    amulet.slot = osrssim::EquipmentSlot::Amulet;
    amulet.bonuses = amuletBonuses;
    equipmentSet.SetEquipmentPiece(amulet);

    osrssim::EquipmentPiece weapon;
    weapon.id = 2002;
    weapon.slot = osrssim::EquipmentSlot::Weapon;
    weapon.hasWeapon = true;
    weapon.weaponId = 12;
    equipmentSet.SetEquipmentPiece(weapon);

    osrssim::CombatComposition composition =
        equipmentSet.BuildCombatComposition(
            stats,
            osrssim::AttackType::Slash,
            9,
            weaponDatabase);
    composition.baseStats.hitpoints = 99;
    return composition;
}

class PlacedActorEncounter : public osrssim::encounter::ActiveEncounter
{
private:
    osrssim::ActorId m_PlayerId = 0;
    osrssim::ActorId m_UnplacedNpcId = 0;

public:
    osrssim::ActorId GetPlayerId() const
    {
        return m_PlayerId;
    }

    osrssim::ActorId GetUnplacedNpcId() const
    {
        return m_UnplacedNpcId;
    }

    void Initialize(osrssim::EncounterContext& context) override
    {
        m_PlayerId = context
                         .CreatePlayer(
                             1,
                             2,
                             CreateCombatComposition(82, 99))
                         .value();
        m_UnplacedNpcId =
            context.CreateNpc(2, 1, CreateCombatComposition(50, 50)).value();

        assert(context.GetWorld().PlaceActor(
            m_PlayerId,
            context.GetWorld().GetDefaultSceneId(),
            {10, 11, 0}));
    }

    bool IsComplete(const osrssim::EncounterContext&) const override
    {
        return false;
    }
};

class EquipmentProvenanceEncounter : public osrssim::encounter::ActiveEncounter
{
private:
    osrssim::ActorId m_PlayerId = 0;

public:
    osrssim::ActorId GetPlayerId() const
    {
        return m_PlayerId;
    }

    void Initialize(osrssim::EncounterContext& context) override
    {
        m_PlayerId =
            context.CreatePlayer(1, 2, CreateEquipmentSetCombatComposition())
                .value();

        assert(context.GetWorld().PlaceActor(
            m_PlayerId,
            context.GetWorld().GetDefaultSceneId(),
            {10, 11, 0}));
    }

    bool IsComplete(const osrssim::EncounterContext&) const override
    {
        return false;
    }
};

class ChangingActorEncounter : public osrssim::encounter::ActiveEncounter
{
private:
    osrssim::ActorId m_PlayerId = 0;
    osrssim::ActorId m_NpcId = 0;

public:
    osrssim::ActorId GetPlayerId() const
    {
        return m_PlayerId;
    }

    osrssim::ActorId GetNpcId() const
    {
        return m_NpcId;
    }

    void Initialize(osrssim::EncounterContext& context) override
    {
        m_PlayerId =
            context.CreatePlayer(1, 2, CreateCombatComposition(82, 99)).value();
        m_NpcId = context.CreateNpc(2, 1, CreateCombatComposition(50, 50)).value();

        assert(context.GetWorld().PlaceActor(
            m_PlayerId,
            context.GetWorld().GetDefaultSceneId(),
            {10, 11, 0}));
    }

    void AfterEngineTick(osrssim::EncounterContext& context) override
    {
        osrssim::World& world = context.GetWorld();

        if (context.GetCurrentTick() == 1)
        {
            assert(world.PlaceActor(
                m_NpcId,
                world.GetDefaultSceneId(),
                {20, 21, 0}));
            assert(world.PlaceActor(
                m_PlayerId,
                world.GetDefaultSceneId(),
                {12, 13, 0}));

            osrssim::CombatComposition composition =
                *world.GetActorCombatComposition(m_PlayerId);
            composition.stats.hitpoints = 77;
            assert(world.SetActorCombatComposition(m_PlayerId, composition));
            assert(world.SetActorMovementTarget(m_PlayerId, m_NpcId));
            assert(world.SetActorAttackTimer(m_PlayerId, 6));
        }
        else if (context.GetCurrentTick() == 2)
        {
            osrssim::CombatComposition composition =
                *world.GetActorCombatComposition(m_PlayerId);
            composition.stats.attack = 80;
            composition.stats.hitpoints = 76;
            assert(world.SetActorCombatComposition(m_PlayerId, composition));
            assert(world.SetActorAttackTimer(m_PlayerId, 5));
            assert(world.SetActorAttackTimer(m_NpcId, 7));
        }
        else if (context.GetCurrentTick() == 3)
        {
            assert(world.RemoveActorSceneMembership(m_PlayerId));
        }
    }

    bool IsComplete(const osrssim::EncounterContext&) const override
    {
        return false;
    }
};

class SceneEntityEncounter : public osrssim::encounter::ActiveEncounter
{
public:
    void Initialize(osrssim::EncounterContext& context) override
    {
        osrssim::Scene* scene =
            context.GetWorld().TryGetScene(context.GetWorld().GetDefaultSceneId());
        assert(scene != nullptr);

        osrssim::CollisionProfile gameObjectCollision;
        gameObjectCollision.blocksMovement = true;
        gameObjectCollision.blocksLineOfSight = true;

        assert(scene->PlaceGameObject(
            {30, 31, 0},
            2001,
            osrssim::CardinalDirection::East,
            2,
            1,
            gameObjectCollision));

        osrssim::CollisionProfile wallCollision;
        wallCollision.blocksMovement = true;

        assert(scene->PlaceWallObject(
            {40, 41, 0},
            3001,
            osrssim::CardinalDirection::North,
            wallCollision));

        osrssim::CollisionProfile lineOfSightCollision;
        lineOfSightCollision.blocksLineOfSight = true;

        assert(scene->PlaceWallObject(
            {42, 41, 0},
            3002,
            osrssim::CardinalDirection::North,
            wallCollision,
            osrssim::CardinalDirection::East,
            lineOfSightCollision));
    }

    void AfterEngineTick(osrssim::EncounterContext& context) override
    {
        osrssim::Scene* scene =
            context.GetWorld().TryGetScene(context.GetWorld().GetDefaultSceneId());
        assert(scene != nullptr);

        if (context.GetCurrentTick() == 1)
        {
            osrssim::CollisionProfile collision;
            collision.blocksLineOfSight = true;

            assert(scene->RemoveGameObject({30, 31, 0}));
            assert(scene->PlaceGameObject(
                {50, 51, 0},
                2002,
                osrssim::CardinalDirection::South,
                collision));
        }
        else if (context.GetCurrentTick() == 2)
        {
            assert(scene->RemoveWallObject({40, 41, 0}));
            assert(scene->RemoveWallObject({42, 41, 0}));
        }
    }

    bool IsComplete(const osrssim::EncounterContext&) const override
    {
        return false;
    }
};

class StandardAttackRecordingEncounter
    : public osrssim::encounter::ActiveEncounter
{
private:
    osrssim::ActorId m_PlayerId = 0;
    osrssim::ActorId m_NpcId = 0;

public:
    osrssim::ActorId GetPlayerId() const
    {
        return m_PlayerId;
    }

    osrssim::ActorId GetNpcId() const
    {
        return m_NpcId;
    }

    void Initialize(osrssim::EncounterContext& context) override
    {
        osrssim::CombatComposition playerComposition =
            CreateCombatComposition(82, 99);
        playerComposition.weapon = {12, 4, 6, 88};

        osrssim::CombatComposition npcComposition =
            CreateCombatComposition(50, 50);
        npcComposition.weapon = {0, 1, 4, 0};

        m_PlayerId = context.CreatePlayer(1, 2, playerComposition).value();
        m_NpcId = context.CreateNpc(1, 1, npcComposition).value();

        assert(context.GetWorld().PlaceActor(
            m_PlayerId,
            context.GetWorld().GetDefaultSceneId(),
            {10, 10, 0}));
        assert(context.GetWorld().PlaceActor(
            m_NpcId,
            context.GetWorld().GetDefaultSceneId(),
            {11, 10, 0}));
        assert(context.GetWorld().SetActorMovementTarget(m_PlayerId, m_NpcId));
    }

    bool IsComplete(const osrssim::EncounterContext&) const override
    {
        return false;
    }
};

class CustomAttackRecordingEncounter
    : public osrssim::encounter::ActiveEncounter
{
private:
    osrssim::ActorId m_PlayerId = 0;
    osrssim::ActorId m_NpcId = 0;

public:
    osrssim::ActorId GetPlayerId() const
    {
        return m_PlayerId;
    }

    osrssim::ActorId GetNpcId() const
    {
        return m_NpcId;
    }

    void Initialize(osrssim::EncounterContext& context) override
    {
        osrssim::CombatComposition playerComposition =
            CreateCombatComposition(82, 99);
        playerComposition.weapon = {77, 1, 4, 0};

        osrssim::CombatComposition npcComposition =
            CreateCombatComposition(20, 20);
        npcComposition.weapon = {0, 1, 4, 0};

        m_PlayerId = context.CreatePlayer(1, 2, playerComposition).value();
        m_NpcId = context.CreateNpc(1, 1, npcComposition).value();

        assert(context.GetWorld().PlaceActor(
            m_PlayerId,
            context.GetWorld().GetDefaultSceneId(),
            {10, 10, 0}));
        assert(context.GetWorld().PlaceActor(
            m_NpcId,
            context.GetWorld().GetDefaultSceneId(),
            {11, 10, 0}));
        assert(context.GetWorld().SetActorMovementTarget(m_PlayerId, m_NpcId));
    }

    bool IsComplete(const osrssim::EncounterContext&) const override
    {
        return false;
    }
};
}  // namespace

int main()
{
    {
        osrssim::recording::EncounterRecorder recorder("Minimal Sample", 0.6);
        osrssim::encounter::EncounterRunner runner(
            std::make_unique<EmptyEncounter>());

        assert(runner.AttachRecorder(recorder));
        runner.Start();
        assert(!runner.AttachRecorder(recorder));
        assert(runner.Step());
        assert(runner.Step());

        const nlohmann::json recording =
            nlohmann::json::parse(recorder.ExportJson());

        assert(recording.at("version") == 1);
        assert(recording.at("metadata").at("encounterName") == "Minimal Sample");
        assert(recording.at("metadata").at("secondsPerTick") == 0.6);
        assert(recording.at("initialState").at("tick") == 0);
        assert(recording.at("initialState").at("actors").empty());
        assert(recording.at("initialState").at("sceneEntities").empty());
        assert(recording.at("initialState").at("projectiles").empty());
        assert(recording.at("ticks").size() == 2);
        assert(recording.at("ticks").at(0).at("tick") == 1);
        assert(recording.at("ticks").at(1).at("tick") == 2);
        assert(recording.at("ticks").at(0).at("actors").empty());
        assert(recording.at("ticks").at(0).at("attacks").empty());
        assert(recording.at("ticks").at(0).at("damageApplications").empty());
        assert(recording.at("ticks").at(0).at("sceneChanges").empty());
        assert(recording.at("ticks").at(0).at("projectiles").empty());

        osrssim::recording::RecordingPlayback playback =
            osrssim::recording::RecordingPlayback::LoadFromJson(
                recorder.ExportJson());

        assert(playback.GetEncounterName() == "Minimal Sample");
        assert(playback.GetSecondsPerTick() == 0.6);
        assert(playback.GetInitialTick() == 0);
        assert(playback.GetCurrentTick() == 0);
        assert(playback.GetLastTick() == 2);
        assert(!playback.PreviousTick());
        assert(playback.NextTick());
        assert(playback.GetCurrentTick() == 1);
        assert(playback.GoToTick(2));
        assert(playback.GetCurrentTick() == 2);
        assert(!playback.NextTick());
    }

    {
        bool threw = false;

        try
        {
            osrssim::recording::RecordingPlayback::LoadFromJson(
                R"({"version":2,"metadata":{"encounterName":"Bad","secondsPerTick":0.6},"initialState":{"tick":0},"ticks":[]})");
        }
        catch (const std::invalid_argument&)
        {
            threw = true;
        }

        assert(threw);
    }

    {
        auto encounter = std::make_unique<PlacedActorEncounter>();
        PlacedActorEncounter* encounterPtr = encounter.get();
        osrssim::recording::EncounterRecorder recorder("Actor State", 0.6);
        osrssim::encounter::EncounterRunner runner(std::move(encounter));

        assert(runner.AttachRecorder(recorder));
        runner.GetEngine().GetCombatService().SetDpsSeed(12345);
        runner.Start();

        const nlohmann::json recording =
            nlohmann::json::parse(recorder.ExportJson());
        const nlohmann::json& initialActors =
            recording.at("initialState").at("actors");

        assert(initialActors.size() == 1);
        assert(initialActors.at(0).at("id") == encounterPtr->GetPlayerId());
        assert(initialActors.at(0).at("kind") == "Player");
        assert(initialActors.at(0).at("present") == true);
        assert(initialActors.at(0).at("sceneMembership").at("sceneId") == 1);
        assert(
            initialActors.at(0)
                .at("sceneMembership")
                .at("coordinate")
                .at("x") == 10);
        assert(
            initialActors.at(0)
                .at("sceneMembership")
                .at("coordinate")
                .at("y") == 11);
        assert(initialActors.at(0).at("size") == 1);
        assert(initialActors.at(0).at("speed") == 2);
        assert(
            initialActors.at(0)
                .at("combatComposition")
                .at("stats")
                .at("hitpoints") == 82);
        assert(
            initialActors.at(0)
                .at("combatComposition")
                .at("baseStats")
                .at("hitpoints") == 99);
        assert(
            initialActors.at(0)
                .at("combatComposition")
                .at("weapon")
                .at("projectileId") == 88);
        assert(
            !initialActors.at(0)
                 .at("combatComposition")
                 .contains("equipmentProvenance"));
        assert(initialActors.at(0).at("debug").at("movementTarget").is_null());
        assert(initialActors.at(0).at("debug").at("attackTimer") == 0);

        for (const nlohmann::json& actor : initialActors)
        {
            assert(actor.at("id") != encounterPtr->GetUnplacedNpcId());
        }

        osrssim::recording::RecordingPlayback playback =
            osrssim::recording::RecordingPlayback::LoadFromJson(
                recorder.ExportJson());
        const nlohmann::json playbackActors =
            nlohmann::json::parse(playback.GetActorsJson());

        assert(playbackActors == initialActors);
    }

    {
        auto encounter = std::make_unique<EquipmentProvenanceEncounter>();
        EquipmentProvenanceEncounter* encounterPtr = encounter.get();
        osrssim::recording::EncounterRecorder recorder(
            "Equipment Provenance",
            0.6);
        osrssim::encounter::EncounterRunner runner(std::move(encounter));

        assert(runner.AttachRecorder(recorder));
        runner.Start();

        const nlohmann::json recording =
            nlohmann::json::parse(recorder.ExportJson());
        const nlohmann::json& initialActor =
            recording.at("initialState").at("actors").at(0);
        const nlohmann::json& provenance =
            initialActor.at("combatComposition").at("equipmentProvenance");

        assert(initialActor.at("id") == encounterPtr->GetPlayerId());
        assert(provenance.size() == 2);
        assert(provenance.at(0).at("slot") == "Amulet");
        assert(provenance.at(0).at("pieceId") == 2001);
        assert(!provenance.at(0).contains("name"));
        assert(!provenance.at(0).contains("bonuses"));
        assert(provenance.at(1).at("slot") == "Weapon");
        assert(provenance.at(1).at("pieceId") == 2002);

        osrssim::recording::RecordingPlayback playback =
            osrssim::recording::RecordingPlayback::LoadFromJson(
                recorder.ExportJson());
        const nlohmann::json playbackActors =
            nlohmann::json::parse(playback.GetActorsJson());

        assert(
            playbackActors.at(0)
                .at("combatComposition")
                .at("equipmentProvenance") == provenance);
    }

    {
        auto encounter = std::make_unique<ChangingActorEncounter>();
        ChangingActorEncounter* encounterPtr = encounter.get();
        osrssim::recording::EncounterRecorder recorder("Changing Actors", 0.6);
        osrssim::encounter::EncounterRunner runner(std::move(encounter));

        assert(runner.AttachRecorder(recorder));
        runner.Start();
        assert(runner.Step());
        assert(runner.Step());
        assert(runner.Step());

        const nlohmann::json recording =
            nlohmann::json::parse(recorder.ExportJson());

        const nlohmann::json& tickOneActors =
            recording.at("ticks").at(0).at("actors");
        assert(tickOneActors.size() == 2);
        assert(tickOneActors.at(0).at("id") == encounterPtr->GetPlayerId());
        assert(!tickOneActors.at(0).contains("combatComposition"));
        assert(tickOneActors.at(0).at("currentHitpoints") == 77);
        assert(
            tickOneActors.at(0)
                .at("sceneMembership")
                .at("coordinate")
                .at("x") == 12);
        assert(
            tickOneActors.at(0)
                .at("debug")
                .at("movementTarget")
                .at("kind") == "Actor");
        assert(
            tickOneActors.at(0)
                .at("debug")
                .at("movementTarget")
                .at("actorId") == encounterPtr->GetNpcId());
        assert(tickOneActors.at(0).at("debug").at("attackTimer") == 6);
        assert(tickOneActors.at(1).at("id") == encounterPtr->GetNpcId());
        assert(tickOneActors.at(1).at("present") == true);
        assert(tickOneActors.at(1).at("kind") == "Npc");
        assert(tickOneActors.at(1).at("debug").at("attackTimer") == -1);

        const nlohmann::json& tickTwoActors =
            recording.at("ticks").at(1).at("actors");
        assert(tickTwoActors.size() == 1);
        assert(tickTwoActors.at(0).at("id") == encounterPtr->GetPlayerId());
        assert(!tickTwoActors.at(0).contains("currentHitpoints"));
        assert(
            tickTwoActors.at(0)
                .at("combatComposition")
                .at("stats")
                .at("attack") == 80);
        assert(
            tickTwoActors.at(0)
                .at("combatComposition")
                .at("stats")
                .at("hitpoints") == 76);
        assert(tickTwoActors.at(0).at("debug").at("attackTimer") == 5);

        const nlohmann::json& tickThreeActors =
            recording.at("ticks").at(2).at("actors");
        assert(tickThreeActors.size() == 1);
        assert(tickThreeActors.at(0).at("id") == encounterPtr->GetPlayerId());
        assert(tickThreeActors.at(0).at("present") == false);

        osrssim::recording::RecordingPlayback playback =
            osrssim::recording::RecordingPlayback::LoadFromJson(
                recorder.ExportJson());

        assert(playback.GoToTick(1));
        nlohmann::json playbackActors =
            nlohmann::json::parse(playback.GetActorsJson());
        assert(playbackActors.size() == 2);
        assert(playbackActors.at(0).at("id") == encounterPtr->GetPlayerId());
        assert(
            playbackActors.at(0)
                .at("combatComposition")
                .at("stats")
                .at("hitpoints") == 77);
        assert(
            playbackActors.at(0)
                .at("sceneMembership")
                .at("coordinate")
                .at("x") == 12);

        assert(playback.GoToTick(2));
        playbackActors = nlohmann::json::parse(playback.GetActorsJson());
        assert(
            playbackActors.at(0)
                .at("combatComposition")
                .at("stats")
                .at("attack") == 80);
        assert(
            playbackActors.at(0)
                .at("combatComposition")
                .at("stats")
                .at("hitpoints") == 76);
        assert(playbackActors.at(1).at("id") == encounterPtr->GetNpcId());
        assert(playbackActors.at(1).at("debug").at("attackTimer") == -1);

        assert(playback.GoToTick(3));
        playbackActors = nlohmann::json::parse(playback.GetActorsJson());
        assert(playbackActors.size() == 1);
        assert(playbackActors.at(0).at("id") == encounterPtr->GetNpcId());
    }

    {
        osrssim::recording::EncounterRecorder recorder("Scene Entities", 0.6);
        osrssim::encounter::EncounterRunner runner(
            std::make_unique<SceneEntityEncounter>());

        assert(runner.AttachRecorder(recorder));
        runner.Start();
        assert(runner.Step());
        assert(runner.Step());

        const nlohmann::json recording =
            nlohmann::json::parse(recorder.ExportJson());
        const nlohmann::json& initialSceneEntities =
            recording.at("initialState").at("sceneEntities");

        assert(initialSceneEntities.size() == 3);
        assert(initialSceneEntities.at(0).at("kind") == "GameObject");
        assert(initialSceneEntities.at(0).at("sceneId") == 1);
        assert(initialSceneEntities.at(0).at("id") == 2001);
        assert(initialSceneEntities.at(0).at("coordinate").at("x") == 30);
        assert(initialSceneEntities.at(0).at("coordinate").at("y") == 31);
        assert(initialSceneEntities.at(0).at("coordinate").at("plane") == 0);
        assert(initialSceneEntities.at(0).at("direction") == "East");
        assert(initialSceneEntities.at(0).at("sizeX") == 2);
        assert(initialSceneEntities.at(0).at("sizeY") == 1);
        assert(
            initialSceneEntities.at(0).at("collision").at("blocksMovement") ==
            true);
        assert(
            initialSceneEntities.at(0)
                .at("collision")
                .at("blocksLineOfSight") == true);
        assert(!initialSceneEntities.at(0).contains("flags"));
        assert(initialSceneEntities.at(1).at("kind") == "WallObject");
        assert(initialSceneEntities.at(1).at("id") == 3001);
        assert(initialSceneEntities.at(1).at("direction") == "North");
        assert(initialSceneEntities.at(2).at("kind") == "WallObject");
        assert(initialSceneEntities.at(2).at("id") == 3002);
        assert(initialSceneEntities.at(2).at("directions").size() == 2);
        assert(
            initialSceneEntities.at(2)
                .at("directions")
                .at(1)
                .at("direction") == "East");

        const nlohmann::json& tickOneChanges =
            recording.at("ticks").at(0).at("sceneChanges");
        assert(tickOneChanges.size() == 2);
        assert(tickOneChanges.at(0).at("present") == false);
        assert(tickOneChanges.at(0).at("kind") == "GameObject");
        assert(tickOneChanges.at(0).at("id") == 2001);
        assert(tickOneChanges.at(1).at("present") == true);
        assert(tickOneChanges.at(1).at("kind") == "GameObject");
        assert(tickOneChanges.at(1).at("id") == 2002);

        const nlohmann::json& tickTwoChanges =
            recording.at("ticks").at(1).at("sceneChanges");
        assert(tickTwoChanges.size() == 2);
        assert(tickTwoChanges.at(0).at("present") == false);
        assert(tickTwoChanges.at(0).at("kind") == "WallObject");
        assert(tickTwoChanges.at(0).at("id") == 3001);
        assert(tickTwoChanges.at(1).at("present") == false);
        assert(tickTwoChanges.at(1).at("id") == 3002);

        osrssim::recording::RecordingPlayback playback =
            osrssim::recording::RecordingPlayback::LoadFromJson(
                recorder.ExportJson());
        nlohmann::json playbackSceneEntities =
            nlohmann::json::parse(playback.GetSceneEntitiesJson());

        assert(playbackSceneEntities == initialSceneEntities);

        assert(playback.GoToTick(1));
        playbackSceneEntities =
            nlohmann::json::parse(playback.GetSceneEntitiesJson());
        assert(playbackSceneEntities.size() == 3);
        assert(playbackSceneEntities.at(0).at("id") == 3001);
        assert(playbackSceneEntities.at(1).at("id") == 3002);
        assert(playbackSceneEntities.at(2).at("id") == 2002);

        assert(playback.GoToTick(2));
        playbackSceneEntities =
            nlohmann::json::parse(playback.GetSceneEntitiesJson());
        assert(playbackSceneEntities.size() == 1);
        assert(playbackSceneEntities.at(0).at("id") == 2002);
    }

    {
        bool threw = false;

        try
        {
            osrssim::recording::RecordingPlayback::LoadFromJson(R"({
                "version": 1,
                "metadata": {
                    "encounterName": "Invalid Scene Entity",
                    "secondsPerTick": 0.6
                },
                "initialState": {
                    "tick": 0,
                    "actors": [],
                    "sceneEntities": [
                        {
                            "kind": "GameObject",
                            "sceneId": 1,
                            "id": 1,
                            "coordinate": { "x": 10, "y": 10, "plane": 0 },
                            "direction": "North",
                            "sizeX": 1,
                            "sizeY": 1,
                            "collision": {
                                "blocksMovement": true,
                                "blocksLineOfSight": false
                            }
                        },
                        {
                            "kind": "GameObject",
                            "sceneId": 1,
                            "id": 2,
                            "coordinate": { "x": 10, "y": 10, "plane": 0 },
                            "direction": "North",
                            "sizeX": 1,
                            "sizeY": 1,
                            "collision": {
                                "blocksMovement": true,
                                "blocksLineOfSight": false
                            }
                        }
                    ],
                    "projectiles": []
                },
                "ticks": []
            })");
        }
        catch (const std::invalid_argument&)
        {
            threw = true;
        }

        assert(threw);
    }

    {
        auto encounter = std::make_unique<StandardAttackRecordingEncounter>();
        StandardAttackRecordingEncounter* encounterPtr = encounter.get();
        osrssim::recording::EncounterRecorder recorder(
            "Standard Attack Recording",
            0.6);
        osrssim::encounter::EncounterRunner runner(std::move(encounter));

        assert(runner.AttachRecorder(recorder));
        runner.Start();
        assert(runner.Step());
        assert(runner.Step());

        const nlohmann::json recording =
            nlohmann::json::parse(recorder.ExportJson());
        const nlohmann::json& tickOne = recording.at("ticks").at(0);
        const nlohmann::json& tickTwo = recording.at("ticks").at(1);

        assert(tickOne.at("attacks").size() == 1);
        const nlohmann::json& attack = tickOne.at("attacks").at(0);
        assert(attack.at("id") == 1);
        assert(attack.at("attackerId") == encounterPtr->GetPlayerId());
        assert(attack.at("targetId") == encounterPtr->GetNpcId());
        assert(attack.at("callback") == "standard_attack");
        assert(attack.at("queuedDamageEvents").size() == 1);
        assert(attack.at("queuedDamageEvents").at(0).at("id") == 1);
        assert(attack.at("queuedDamageEvents").at(0).at("attackId") == 1);
        assert(
            attack.at("queuedDamageEvents").at(0).at("targetId") ==
            encounterPtr->GetNpcId());
        assert(attack.at("queuedDamageEvents").at(0).at("damage").get<int>() >= 0);
        assert(
            attack.at("queuedDamageEvents").at(0).at("damage").get<int>() <= 50);
        assert(attack.at("projectile").at("projectileId") == 88);
        assert(attack.at("projectile").at("targetActorId") == encounterPtr->GetNpcId());
        assert(tickOne.at("projectiles").size() == 1);
        assert(tickOne.at("projectiles").at(0).at("projectileId") == 88);
        assert(tickOne.at("projectiles").at(0).at("elapsedTicks") == 0);
        assert(tickOne.at("projectiles").at(0).at("totalTicks") == 1);

        assert(tickTwo.at("damageApplications").size() == 1);
        const nlohmann::json& damageApplication =
            tickTwo.at("damageApplications").at(0);
        assert(damageApplication.at("damageEventId") == 1);
        assert(damageApplication.at("attackId") == 1);
        assert(damageApplication.at("targetId") == encounterPtr->GetNpcId());
        assert(damageApplication.at("queuedDamage") ==
               attack.at("queuedDamageEvents").at(0).at("damage"));
        assert(damageApplication.at("appliedDamage") ==
               attack.at("queuedDamageEvents").at(0).at("damage"));
        assert(tickTwo.at("projectiles").empty());

        osrssim::recording::RecordingPlayback playback =
            osrssim::recording::RecordingPlayback::LoadFromJson(
                recorder.ExportJson());

        assert(playback.GoToTick(1));
        assert(nlohmann::json::parse(playback.GetAttacksJson()) ==
               tickOne.at("attacks"));
        assert(nlohmann::json::parse(playback.GetDamageApplicationsJson()).empty());
        assert(nlohmann::json::parse(playback.GetProjectilesJson()) ==
               tickOne.at("projectiles"));

        assert(playback.GoToTick(2));
        assert(nlohmann::json::parse(playback.GetAttacksJson()).empty());
        assert(nlohmann::json::parse(playback.GetDamageApplicationsJson()) ==
               tickTwo.at("damageApplications"));
        assert(nlohmann::json::parse(playback.GetProjectilesJson()).empty());
    }

    {
        auto encounter = std::make_unique<CustomAttackRecordingEncounter>();
        CustomAttackRecordingEncounter* encounterPtr = encounter.get();
        osrssim::recording::EncounterRecorder recorder(
            "Named Custom Attack Recording",
            0.6);
        osrssim::encounter::EncounterRunner runner(std::move(encounter));
        osrssim::CombatService& combatService =
            runner.GetEngine().GetCombatService();

        combatService.RegisterAttackCallbackName(
            "dragon_claws",
            [&combatService](
                osrssim::World& world,
                osrssim::ActorId attackerId,
                osrssim::ActorId targetId,
                osrssim::Tick currentTick,
                const osrssim::WeaponDefinition&)
            {
                osrssim::CombatService::AttackObservation attack =
                    combatService.CreateAttackObservation(
                        world,
                        attackerId,
                        targetId,
                        currentTick);

                assert(combatService.QueueStructuredDamageEvent(
                    world,
                    attack,
                    targetId,
                    7,
                    1));
                assert(combatService.QueueStructuredDamageEvent(
                    world,
                    attack,
                    targetId,
                    5,
                    1));
                combatService.RecordAttackObservation(attack);
                return true;
            });
        combatService.BindWeaponAttackCallbackName(77, "dragon_claws");

        assert(runner.AttachRecorder(recorder));
        runner.Start();
        assert(runner.Step());
        assert(runner.Step());

        const nlohmann::json recording =
            nlohmann::json::parse(recorder.ExportJson());
        const nlohmann::json& tickOne = recording.at("ticks").at(0);
        const nlohmann::json& attack = tickOne.at("attacks").at(0);

        assert(tickOne.at("attacks").size() == 1);
        assert(attack.at("id") == 1);
        assert(attack.at("attackerId") == encounterPtr->GetPlayerId());
        assert(attack.at("targetId") == encounterPtr->GetNpcId());
        assert(attack.at("callback") == "dragon_claws");
        assert(attack.at("queuedDamageEvents").size() == 2);
        assert(attack.at("queuedDamageEvents").at(0).at("id") == 1);
        assert(attack.at("queuedDamageEvents").at(0).at("attackId") == 1);
        assert(attack.at("queuedDamageEvents").at(0).at("damage") == 7);
        assert(attack.at("queuedDamageEvents").at(1).at("id") == 2);
        assert(attack.at("queuedDamageEvents").at(1).at("attackId") == 1);
        assert(attack.at("queuedDamageEvents").at(1).at("damage") == 5);
        assert(!attack.contains("projectile"));

        const nlohmann::json& damageApplications =
            recording.at("ticks").at(1).at("damageApplications");
        assert(damageApplications.size() == 2);
        assert(damageApplications.at(0).at("damageEventId") == 1);
        assert(damageApplications.at(0).at("attackId") == 1);
        assert(damageApplications.at(0).at("queuedDamage") == 7);
        assert(damageApplications.at(0).at("appliedDamage") == 7);
        assert(damageApplications.at(1).at("damageEventId") == 2);
        assert(damageApplications.at(1).at("attackId") == 1);
        assert(damageApplications.at(1).at("queuedDamage") == 5);
        assert(damageApplications.at(1).at("appliedDamage") == 5);
    }

    {
        auto encounter = std::make_unique<CustomAttackRecordingEncounter>();
        CustomAttackRecordingEncounter* encounterPtr = encounter.get();
        osrssim::recording::EncounterRecorder recorder(
            "Anonymous Custom Attack Recording",
            0.6);
        osrssim::encounter::EncounterRunner runner(std::move(encounter));
        osrssim::CombatService& combatService =
            runner.GetEngine().GetCombatService();

        combatService.RegisterWeaponAttackCallback(
            77,
            [](
                osrssim::World&,
                osrssim::ActorId,
                osrssim::ActorId,
                osrssim::Tick,
                const osrssim::WeaponDefinition&)
            {
                return true;
            });

        assert(runner.AttachRecorder(recorder));
        runner.Start();
        assert(runner.Step());

        const nlohmann::json recording =
            nlohmann::json::parse(recorder.ExportJson());
        const nlohmann::json& attacks =
            recording.at("ticks").at(0).at("attacks");

        assert(attacks.size() == 1);
        assert(attacks.at(0).at("attackerId") == encounterPtr->GetPlayerId());
        assert(attacks.at(0).at("targetId") == encounterPtr->GetNpcId());
        assert(attacks.at(0).at("callback") == "anonymous_attack_callback");
        assert(attacks.at(0).at("queuedDamageEvents").empty());
        assert(recording.at("ticks").at(0).at("damageApplications").empty());
    }
}
