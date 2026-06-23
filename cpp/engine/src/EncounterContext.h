#pragma once

#include "Types.h"

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
    Tick GetCurrentTick() const;
};

}  // namespace osrssim
