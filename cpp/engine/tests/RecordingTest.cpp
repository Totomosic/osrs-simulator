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
}
