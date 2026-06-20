#include "DatabaseService.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <initializer_list>
#include <stdexcept>
#include <string>
#include <string_view>

namespace osrssim
{
namespace
{

using Json = nlohmann::json;

constexpr int SupportedDatasetManifestVersion = 1;

bool HasOnlyKeys(
    const Json& value,
    std::initializer_list<std::string_view> allowedKeys)
{
    for (const auto& item : value.items())
    {
        const std::string_view key = item.key();
        const bool isAllowed = std::find(
            allowedKeys.begin(),
            allowedKeys.end(),
            key) != allowedKeys.end();

        if (!isAllowed)
        {
            return false;
        }
    }

    return true;
}

void RequireObject(const Json& value, const std::string& description)
{
    if (!value.is_object())
    {
        throw std::invalid_argument(description + " must be an object");
    }
}

int GetRequiredInt(const Json& value, const std::string& key)
{
    if (!value.contains(key) || !value.at(key).is_number_integer())
    {
        throw std::invalid_argument(key + " must be an integer");
    }

    return value.at(key).get<int>();
}

std::string GetRequiredString(const Json& value, const std::string& key)
{
    if (!value.contains(key) || !value.at(key).is_string())
    {
        throw std::invalid_argument(key + " must be a string");
    }

    return value.at(key).get<std::string>();
}

Json ParseManifestJson(const std::string& manifestJson)
{
    try
    {
        return Json::parse(manifestJson);
    }
    catch (const Json::parse_error& error)
    {
        throw std::invalid_argument(error.what());
    }
}

void ValidateManifest(const Json& manifest)
{
    RequireObject(manifest, "dataset manifest");

    if (!HasOnlyKeys(manifest, {"version", "documents"}))
    {
        throw std::invalid_argument("dataset manifest contains an unknown field");
    }

    if (GetRequiredInt(manifest, "version") != SupportedDatasetManifestVersion)
    {
        throw std::invalid_argument("unsupported dataset manifest version");
    }

    if (!manifest.contains("documents") ||
        !manifest.at("documents").is_object())
    {
        throw std::invalid_argument("documents must be an object");
    }

    const Json& documents = manifest.at("documents");
    if (!HasOnlyKeys(documents, {"equipment", "weapons"}))
    {
        throw std::invalid_argument("documents contains an unknown dataset");
    }

    const std::string equipmentDocument =
        GetRequiredString(documents, "equipment");
    if (equipmentDocument.empty())
    {
        throw std::invalid_argument("equipment document path must not be empty");
    }

    const std::string weaponsDocument =
        GetRequiredString(documents, "weapons");
    if (weaponsDocument.empty())
    {
        throw std::invalid_argument("weapons document path must not be empty");
    }
}

void ValidateWeaponCallbacks(
    const WeaponDatabase& weaponDatabase,
    const CombatService& combatService)
{
    for (const WeaponRecord& record : weaponDatabase.GetAllWeaponRecords())
    {
        if (!combatService.HasAttackCallbackName(record.attackCallbackName))
        {
            throw std::invalid_argument(
                "weapon record references an unknown attack callback name");
        }
    }
}

void ValidateEquipmentWeapons(
    const EquipmentDatabase& equipmentDatabase,
    const WeaponDatabase& weaponDatabase)
{
    for (const EquipmentPiece& piece : equipmentDatabase.GetAllEquipmentPieces())
    {
        if (piece.hasWeapon &&
            !weaponDatabase.HasWeaponRecord(piece.weaponId))
        {
            throw std::invalid_argument(
                "equipment piece references an unknown weapon ID");
        }
    }
}

}  // namespace

DatabaseService DatabaseService::LoadFromDocuments(
    const std::string& manifestJson,
    const std::unordered_map<std::string, std::string>& documents)
{
    const CombatService combatService;
    return LoadFromDocuments(manifestJson, documents, combatService);
}

DatabaseService DatabaseService::LoadFromDocuments(
    const std::string& manifestJson,
    const std::unordered_map<std::string, std::string>& documents,
    const CombatService& combatService)
{
    const Json manifest = ParseManifestJson(manifestJson);
    ValidateManifest(manifest);

    const auto equipmentDocument = documents.find("equipment");
    if (equipmentDocument == documents.end())
    {
        throw std::invalid_argument("equipment document is missing");
    }

    const auto weaponsDocument = documents.find("weapons");
    if (weaponsDocument == documents.end())
    {
        throw std::invalid_argument("weapons document is missing");
    }

    DatabaseService service;
    service.m_EquipmentDatabase =
        EquipmentDatabase::LoadFromJson(equipmentDocument->second);
    service.m_WeaponDatabase =
        WeaponDatabase::LoadFromJson(weaponsDocument->second);
    ValidateWeaponCallbacks(service.m_WeaponDatabase, combatService);
    ValidateEquipmentWeapons(
        service.m_EquipmentDatabase,
        service.m_WeaponDatabase);

    return service;
}

DatabaseService DatabaseService::LoadFromJsonDocuments(
    const std::string& manifestJson,
    const std::string& equipmentJson,
    const std::string& weaponsJson)
{
    return LoadFromDocuments(
        manifestJson,
        {{"equipment", equipmentJson}, {"weapons", weaponsJson}});
}

const EquipmentDatabase& DatabaseService::GetEquipmentDatabase() const
{
    return m_EquipmentDatabase;
}

const WeaponDatabase& DatabaseService::GetWeaponDatabase() const
{
    return m_WeaponDatabase;
}

void DatabaseService::ConfigureCombatService(
    CombatService& combatService) const
{
    for (const WeaponRecord& record : m_WeaponDatabase.GetAllWeaponRecords())
    {
        combatService.BindWeaponAttackCallbackName(
            record.weapon.id,
            record.attackCallbackName);
    }
}

}  // namespace osrssim
