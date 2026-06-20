#pragma once

#include "CombatService.h"
#include "EquipmentDatabase.h"
#include "WeaponDatabase.h"

#include <string>
#include <unordered_map>

namespace osrssim
{

class DatabaseService
{
private:
    EquipmentDatabase m_EquipmentDatabase;
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
        const std::string& weaponsJson);

    const EquipmentDatabase& GetEquipmentDatabase() const;
    const WeaponDatabase& GetWeaponDatabase() const;
    void ConfigureCombatService(CombatService& combatService) const;
};

}  // namespace osrssim
