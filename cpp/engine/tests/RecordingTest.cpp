#include "encounter/ActiveEncounter.h"
#include "encounter/EncounterRunner.h"
#include "recording/EncounterRecorder.h"
#include "recording/RecordingPlayback.h"

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
}
