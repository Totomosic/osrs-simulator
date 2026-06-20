#pragma once

#include "CombatService.h"
#include "CombatCompositionDatabase.h"
#include "EquipmentDatabase.h"
#include "NpcDatabase.h"
#include "WeaponDatabase.h"

#include <string>
#include <unordered_map>

namespace osrssim
{

class DatabaseService
{
private:
    CombatCompositionDatabase m_CombatCompositionDatabase;
    EquipmentDatabase m_EquipmentDatabase;
    NpcDatabase m_NpcDatabase;
    WeaponDatabase m_WeaponDatabase;

public:
    static DatabaseService LoadFromDocuments(
        const std::string& manifestJson,
        const std::unordered_map<std::string, std::string>& documents);
    static DatabaseService LoadFromDocuments(
        const std::string& manifestJson,
        const std::unordered_map<std::string, std::string>& documents,
        const CombatService& combatService);
    static DatabaseService LoadFromJsonDocuments(
        const std::string& manifestJson,
        const std::string& equipmentJson,
        const std::string& weaponsJson,
        const std::string& combatCompositionsJson,
        const std::string& npcsJson);

    const CombatCompositionDatabase& GetCombatCompositionDatabase() const;
    const EquipmentDatabase& GetEquipmentDatabase() const;
    const NpcDatabase& GetNpcDatabase() const;
    const WeaponDatabase& GetWeaponDatabase() const;
    void ConfigureCombatService(CombatService& combatService) const;
};

}  // namespace osrssim
