#include "encounter/ActiveEncounter.h"
#include "encounter/EncounterRunner.h"

#include <cassert>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
class RecordingEncounter : public osrssim::encounter::ActiveEncounter
{
private:
    std::vector<std::string>* m_Events = nullptr;
    bool m_CompleteBeforeTick = false;
    bool m_CompleteAfterTick = false;

public:
    explicit RecordingEncounter(std::vector<std::string>& events)
        : m_Events(&events)
    {
    }

    void CompleteBeforeTick()
    {
        m_CompleteBeforeTick = true;
    }

    void CompleteAfterTick()
    {
        m_CompleteAfterTick = true;
    }

    void Initialize(osrssim::EncounterContext& context) override
    {
        m_Events->push_back(
            "initialize@" + std::to_string(context.GetCurrentTick()));
        context.GetWorld().CreatePlayer(1, 1, osrssim::CombatComposition{});
    }

    void BeforeEngineTick(osrssim::EncounterContext& context) override
    {
        m_Events->push_back(
            "before@" + std::to_string(context.GetCurrentTick()));
    }

    void AfterEngineTick(osrssim::EncounterContext& context) override
    {
        m_Events->push_back(
            "after@" + std::to_string(context.GetCurrentTick()));
        m_CompleteBeforeTick = m_CompleteAfterTick;
    }

    bool IsComplete(const osrssim::EncounterContext&) const override
    {
        return m_CompleteBeforeTick;
    }
};
}  // namespace

int main()
{
    {
        std::vector<std::string> events;
        auto encounter = std::make_unique<RecordingEncounter>(events);
        RecordingEncounter* encounterHandle = encounter.get();
        osrssim::encounter::EncounterRunner runner(std::move(encounter));

        bool threw = false;

        try
        {
            runner.Step();
        }
        catch (const std::logic_error&)
        {
            threw = true;
        }

        assert(threw);
        assert(runner.GetEngine().GetCurrentTick() == 0);

        runner.Start();
        runner.Start();

        assert((events == std::vector<std::string>{"initialize@0"}));
        assert(runner.GetEngine().GetCurrentTick() == 0);
        assert(runner.GetEngine().GetWorld().GetPlayers().size() == 1);

        assert(runner.Step());
        assert((
            events ==
            std::vector<std::string>{
                "initialize@0",
                "before@0",
                "after@1"}));
        assert(runner.GetEngine().GetCurrentTick() == 1);

        encounterHandle->CompleteBeforeTick();

        assert(!runner.Step());
        assert(runner.GetEngine().GetCurrentTick() == 1);
        assert((
            events ==
            std::vector<std::string>{
                "initialize@0",
                "before@0",
                "after@1"}));
    }

    {
        std::vector<std::string> events;
        auto encounter = std::make_unique<RecordingEncounter>(events);
        RecordingEncounter* encounterHandle = encounter.get();
        osrssim::encounter::EncounterRunner runner(std::move(encounter));

        runner.Start();
        encounterHandle->CompleteAfterTick();

        assert(runner.Step());
        assert(runner.GetEngine().GetCurrentTick() == 1);
        assert(!runner.Step());
        assert(runner.GetEngine().GetCurrentTick() == 1);
    }
}
