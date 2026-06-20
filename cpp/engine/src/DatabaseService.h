#pragma once

#include "EquipmentDatabase.h"

#include <string>
#include <unordered_map>

namespace osrssim
{

class DatabaseService
{
private:
    EquipmentDatabase m_EquipmentDatabase;

public:
    static DatabaseService LoadFromDocuments(
        const std::string& manifestJson,
        const std::unordered_map<std::string, std::string>& documents);
    static DatabaseService LoadFromJsonDocuments(
        const std::string& manifestJson,
        const std::string& equipmentJson);

    const EquipmentDatabase& GetEquipmentDatabase() const;
};

}  // namespace osrssim
