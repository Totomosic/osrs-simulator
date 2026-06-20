#pragma once

#include "Types.h"

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

namespace osrssim
{

struct NpcDefinition
{
    NpcId id = 0;
    std::string name;
    bool hasCombatLevel = false;
    int combatLevel = 0;
    int size = 1;
    int speed = 1;
    CombatCompositionId combatCompositionId = 0;
};

class NpcDatabase
{
private:
    std::vector<NpcDefinition> m_Definitions;
    std::unordered_map<NpcId, std::size_t> m_DefinitionIndexById;

public:
    static NpcDatabase LoadFromJson(const std::string& json);

    const NpcDefinition* TryGetNpcDefinition(NpcId id) const;
    bool HasNpcDefinition(NpcId id) const;
    NpcDefinition GetNpcDefinition(NpcId id) const;
    std::vector<NpcDefinition> GetAllNpcDefinitions() const;

private:
    void AddNpcDefinition(const NpcDefinition& definition);
};

}  // namespace osrssim
