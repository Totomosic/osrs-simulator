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
    if (!HasOnlyKeys(
            documents,
            {"equipment", "weapons", "combatCompositions", "npcs"}))
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

    const std::string combatCompositionsDocument =
        GetRequiredString(documents, "combatCompositions");
    if (combatCompositionsDocument.empty())
    {
        throw std::invalid_argument(
            "combat compositions document path must not be empty");
    }

    const std::string npcsDocument = GetRequiredString(documents, "npcs");
    if (npcsDocument.empty())
    {
        throw std::invalid_argument("NPCs document path must not be empty");
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

void ValidateNpcCombatCompositions(
    const NpcDatabase& npcDatabase,
    const CombatCompositionDatabase& combatCompositionDatabase)
{
    for (const NpcDefinition& definition : npcDatabase.GetAllNpcDefinitions())
    {
        if (definition.combatCompositionId >=
            CombatCompositionDatabase::FirstSavedCombatCompositionId)
        {
            throw std::invalid_argument(
                "NPC definition references a non-built-in combat composition");
        }

        const CombatCompositionRecord* record =
            combatCompositionDatabase.TryGetCombatCompositionRecord(
                definition.combatCompositionId);
        if (record == nullptr)
        {
            throw std::invalid_argument(
                "NPC definition references an unknown combat composition ID");
        }
        if (record->source != CombatCompositionSource::BuiltIn)
        {
            throw std::invalid_argument(
                "NPC definition references a non-built-in combat composition");
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

    const auto combatCompositionsDocument =
        documents.find("combatCompositions");
    if (combatCompositionsDocument == documents.end())
    {
        throw std::invalid_argument(
            "combat compositions document is missing");
    }

    const auto npcsDocument = documents.find("npcs");
    if (npcsDocument == documents.end())
    {
        throw std::invalid_argument("NPCs document is missing");
    }

    DatabaseService service;
    service.m_EquipmentDatabase =
        EquipmentDatabase::LoadFromJson(equipmentDocument->second);
    service.m_WeaponDatabase =
        WeaponDatabase::LoadFromJson(weaponsDocument->second);
    service.m_CombatCompositionDatabase =
        CombatCompositionDatabase::LoadFromJson(
            combatCompositionsDocument->second,
            service.m_WeaponDatabase);
    service.m_NpcDatabase = NpcDatabase::LoadFromJson(npcsDocument->second);
    ValidateWeaponCallbacks(service.m_WeaponDatabase, combatService);
    ValidateEquipmentWeapons(
        service.m_EquipmentDatabase,
        service.m_WeaponDatabase);
    ValidateNpcCombatCompositions(
        service.m_NpcDatabase,
        service.m_CombatCompositionDatabase);

    return service;
}

DatabaseService DatabaseService::LoadFromJsonDocuments(
    const std::string& manifestJson,
    const std::string& equipmentJson,
    const std::string& weaponsJson,
    const std::string& combatCompositionsJson,
    const std::string& npcsJson)
{
    return LoadFromDocuments(
        manifestJson,
        {
            {"equipment", equipmentJson},
            {"weapons", weaponsJson},
            {"combatCompositions", combatCompositionsJson},
            {"npcs", npcsJson},
        });
}

const CombatCompositionDatabase&
DatabaseService::GetCombatCompositionDatabase() const
{
    return m_CombatCompositionDatabase;
}

const EquipmentDatabase& DatabaseService::GetEquipmentDatabase() const
{
    return m_EquipmentDatabase;
}

const NpcDatabase& DatabaseService::GetNpcDatabase() const
{
    return m_NpcDatabase;
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
