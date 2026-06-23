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

std::optional<ActorId> EncounterContext::CreatePlayer(
    int size,
    int speed,
    CombatComposition combatComposition)
{
    return m_Engine.CreatePlayer(size, speed, combatComposition);
}

std::optional<ActorId> EncounterContext::CreateNpc(
    int size,
    int speed,
    CombatComposition combatComposition)
{
    return m_Engine.CreateNpc(size, speed, combatComposition);
}

bool EncounterContext::RemovePlayer(ActorId actorId)
{
    return m_Engine.RemovePlayer(actorId);
}

bool EncounterContext::RemoveNpc(ActorId actorId)
{
    return m_Engine.RemoveNpc(actorId);
}

Tick EncounterContext::GetCurrentTick() const
{
    return m_Engine.GetCurrentTick();
}

}  // namespace osrssim
