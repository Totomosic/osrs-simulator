#pragma once

#include "DpsService.h"
#include "Types.h"

#include <optional>

namespace osrssim
{

class CombatService;
class Engine;
class World;

class EncounterContext
{
private:
    Engine& m_Engine;

public:
    explicit EncounterContext(Engine& engine);

    World& GetWorld();
    const World& GetWorld() const;
    CombatService& GetCombatService();
    const CombatService& GetCombatService() const;
    std::optional<ActorId> CreatePlayer(
        int size,
        int speed,
        CombatComposition combatComposition);
    std::optional<ActorId> CreateNpc(
        int size,
        int speed,
        CombatComposition combatComposition);
    bool RemovePlayer(ActorId actorId);
    bool RemoveNpc(ActorId actorId);
    Tick GetCurrentTick() const;
};

}  // namespace osrssim
