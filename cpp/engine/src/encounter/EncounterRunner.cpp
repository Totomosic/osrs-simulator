#include "encounter/EncounterRunner.h"

#include "EncounterContext.h"

#include <stdexcept>

namespace osrssim::encounter
{

EncounterRunner::EncounterRunner(std::unique_ptr<ActiveEncounter> activeEncounter)
    : m_ActiveEncounter(std::move(activeEncounter))
{
    if (m_ActiveEncounter == nullptr)
    {
        throw std::invalid_argument("Active Encounter cannot be null");
    }
}

void EncounterRunner::Start()
{
    if (m_Started)
    {
        return;
    }

    EncounterContext context(m_Engine);
    m_ActiveEncounter->Initialize(context);
    m_Started = true;

    if (m_Recorder != nullptr)
    {
        m_Recorder->RecordInitialState(m_Engine);
    }
}

bool EncounterRunner::AttachRecorder(recording::EncounterRecorder& recorder)
{
    if (m_Started)
    {
        return false;
    }

    m_Recorder = &recorder;
    m_Engine.GetCombatService().AddObserver(recorder);
    return true;
}

bool EncounterRunner::Step()
{
    if (!m_Started)
    {
        throw std::logic_error("Encounter Runner must be started before Step");
    }

    EncounterContext context(m_Engine);

    if (m_ActiveEncounter->IsComplete(context))
    {
        return false;
    }

    m_ActiveEncounter->BeforeEngineTick(context);
    m_Engine.Step();
    m_ActiveEncounter->AfterEngineTick(context);

    if (m_Recorder != nullptr)
    {
        m_Recorder->RecordCompletedTick(m_Engine);
    }

    return true;
}

Engine& EncounterRunner::GetEngine()
{
    return m_Engine;
}

const Engine& EncounterRunner::GetEngine() const
{
    return m_Engine;
}

}  // namespace osrssim::encounter
