#include "EncounterContext.h"

#include "Engine.h"

namespace osrssim
{

EncounterContext::EncounterContext(Engine& engine)
    : m_Engine(engine)
{
}

World& EncounterContext::GetWorld()
{
    return m_Engine.GetWorld();
}

const World& EncounterContext::GetWorld() const
{
    return m_Engine.GetWorld();
}

CombatService& EncounterContext::GetCombatService()
{
    return m_Engine.GetCombatService();
}

const CombatService& EncounterContext::GetCombatService() const
{
    return m_Engine.GetCombatService();
}

Tick EncounterContext::GetCurrentTick() const
{
    return m_Engine.GetCurrentTick();
}

}  // namespace osrssim
