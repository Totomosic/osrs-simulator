#include "CombatCompositionDatabase.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <initializer_list>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>

namespace osrssim
{
namespace
{

using Json = nlohmann::json;

constexpr int SupportedCombatCompositionJsonVersion = 1;
constexpr int SupportedSavedCombatCompositionJsonVersion = 1;

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

int GetRequiredPositiveInt(const Json& value, const std::string& key)
{
    const int result = GetRequiredInt(value, key);
    if (result < 1)
    {
        throw std::invalid_argument(key + " must be at least 1");
    }

    return result;
}

int GetOptionalInt(const Json& value, const std::string& key)
{
    if (!value.contains(key))
    {
        return 0;
    }

    if (!value.at(key).is_number_integer())
    {
        throw std::invalid_argument(key + " must be an integer");
    }

    return value.at(key).get<int>();
}

double GetOptionalDouble(const Json& value, const std::string& key)
{
    if (!value.contains(key))
    {
        return 0.0;
    }

    if (!value.at(key).is_number())
    {
        throw std::invalid_argument(key + " must be a number");
    }

    return value.at(key).get<double>();
}

std::string GetRequiredString(const Json& value, const std::string& key)
{
    if (!value.contains(key) || !value.at(key).is_string())
    {
        throw std::invalid_argument(key + " must be a string");
    }

    return value.at(key).get<std::string>();
}

CombatCompositionId GetRequiredCombatCompositionId(
    const Json& value,
    const std::string& key)
{
    if (!value.contains(key) || !value.at(key).is_number_integer())
    {
        throw std::invalid_argument(key + " must be an integer");
    }

    const Json& id = value.at(key);
    if (id.is_number_unsigned())
    {
        return id.get<CombatCompositionId>();
    }

    const long long signedId = id.get<long long>();
    if (signedId < 0)
    {
        throw std::invalid_argument(key + " must be non-negative");
    }

    return static_cast<CombatCompositionId>(signedId);
}

WeaponId GetRequiredWeaponId(const Json& value, const std::string& key)
{
    if (!value.contains(key) || !value.at(key).is_number_integer())
    {
        throw std::invalid_argument(key + " must be an integer");
    }

    const Json& id = value.at(key);
    if (id.is_number_unsigned())
    {
        return id.get<WeaponId>();
    }

    const long long signedId = id.get<long long>();
    if (signedId < 0)
    {
        throw std::invalid_argument(key + " must be non-negative");
    }

    return static_cast<WeaponId>(signedId);
}

CombatStats ParseCombatStats(const Json& value)
{
    RequireObject(value, "stats");

    if (!HasOnlyKeys(
            value,
            {
                "attack",
                "strength",
                "defence",
                "ranged",
                "magic",
                "hitpoints",
            }))
    {
        throw std::invalid_argument("stats contains an unknown field");
    }

    CombatStats stats;
    stats.attack = GetRequiredPositiveInt(value, "attack");
    stats.strength = GetRequiredPositiveInt(value, "strength");
    stats.defence = GetRequiredPositiveInt(value, "defence");
    stats.ranged = GetRequiredPositiveInt(value, "ranged");
    stats.magic = GetRequiredPositiveInt(value, "magic");
    stats.hitpoints = GetRequiredPositiveInt(value, "hitpoints");

    return stats;
}

EquipmentBonuses ParseEquipmentBonuses(const Json& value)
{
    RequireObject(value, "bonuses");

    if (!HasOnlyKeys(
            value,
            {
                "stabAttack",
                "slashAttack",
                "crushAttack",
                "magicAttack",
                "rangedAttack",
                "stabDefence",
                "slashDefence",
                "crushDefence",
                "magicDefence",
                "rangedDefenceLight",
                "rangedDefenceStandard",
                "rangedDefenceHeavy",
                "meleeStrength",
                "rangedStrength",
                "magicDamagePercent",
            }))
    {
        throw std::invalid_argument("bonuses contains an unknown field");
    }

    EquipmentBonuses bonuses;
    bonuses.stabAttack = GetOptionalInt(value, "stabAttack");
    bonuses.slashAttack = GetOptionalInt(value, "slashAttack");
    bonuses.crushAttack = GetOptionalInt(value, "crushAttack");
    bonuses.magicAttack = GetOptionalInt(value, "magicAttack");
    bonuses.rangedAttack = GetOptionalInt(value, "rangedAttack");
    bonuses.stabDefence = GetOptionalInt(value, "stabDefence");
    bonuses.slashDefence = GetOptionalInt(value, "slashDefence");
    bonuses.crushDefence = GetOptionalInt(value, "crushDefence");
    bonuses.magicDefence = GetOptionalInt(value, "magicDefence");
    bonuses.rangedDefenceLight =
        GetOptionalInt(value, "rangedDefenceLight");
    bonuses.rangedDefenceStandard =
        GetOptionalInt(value, "rangedDefenceStandard");
    bonuses.rangedDefenceHeavy =
        GetOptionalInt(value, "rangedDefenceHeavy");
    bonuses.meleeStrength = GetOptionalInt(value, "meleeStrength");
    bonuses.rangedStrength = GetOptionalInt(value, "rangedStrength");
    bonuses.magicDamagePercent =
        GetOptionalDouble(value, "magicDamagePercent");

    return bonuses;
}

AttackType ParseAttackType(const std::string& attackType)
{
    if (attackType == "stab")
    {
        return AttackType::Stab;
    }
    if (attackType == "slash")
    {
        return AttackType::Slash;
    }
    if (attackType == "crush")
    {
        return AttackType::Crush;
    }
    if (attackType == "magic")
    {
        return AttackType::Magic;
    }
    if (attackType == "ranged_light")
    {
        return AttackType::RangedLight;
    }
    if (attackType == "ranged_standard")
    {
        return AttackType::RangedStandard;
    }
    if (attackType == "ranged_heavy")
    {
        return AttackType::RangedHeavy;
    }

    throw std::invalid_argument("attackType is not supported");
}

std::string AttackTypeToString(AttackType attackType)
{
    switch (attackType)
    {
        case AttackType::Stab:
            return "stab";
        case AttackType::Slash:
            return "slash";
        case AttackType::Crush:
            return "crush";
        case AttackType::Magic:
            return "magic";
        case AttackType::RangedLight:
            return "ranged_light";
        case AttackType::RangedStandard:
            return "ranged_standard";
        case AttackType::RangedHeavy:
            return "ranged_heavy";
    }

    throw std::invalid_argument("attackType is not supported");
}

CombatCompositionRecord ParseCombatCompositionRecord(
    const Json& value,
    const WeaponDatabase& weaponDatabase,
    CombatCompositionSource source)
{
    RequireObject(value, "combat composition record");

    if (!HasOnlyKeys(
            value,
            {
                "id",
                "name",
                "stats",
                "bonuses",
                "attackType",
                "magicBaseMaximumHit",
                "weaponId",
            }))
    {
        throw std::invalid_argument(
            "combat composition record contains an unknown field");
    }

    if (!value.contains("stats"))
    {
        throw std::invalid_argument("stats is required");
    }
    if (!value.contains("bonuses"))
    {
        throw std::invalid_argument("bonuses is required");
    }

    const WeaponId weaponId = GetRequiredWeaponId(value, "weaponId");
    const WeaponRecord* weaponRecord =
        weaponDatabase.TryGetWeaponRecord(weaponId);
    if (weaponRecord == nullptr)
    {
        throw std::invalid_argument(
            "combat composition references an unknown weapon ID");
    }

    CombatCompositionRecord record;
    record.id = GetRequiredCombatCompositionId(value, "id");
    record.name = GetRequiredString(value, "name");
    record.source = source;
    record.composition.stats = ParseCombatStats(value.at("stats"));
    record.composition.bonuses = ParseEquipmentBonuses(value.at("bonuses"));
    record.composition.attackType =
        ParseAttackType(GetRequiredString(value, "attackType"));
    record.composition.magicBaseMaximumHit =
        GetOptionalInt(value, "magicBaseMaximumHit");
    record.composition.weapon = weaponRecord->weapon;

    if (record.name.empty())
    {
        throw std::invalid_argument(
            "combat composition name must not be empty");
    }

    if (record.composition.magicBaseMaximumHit < 0)
    {
        throw std::invalid_argument(
            "magicBaseMaximumHit must be non-negative");
    }

    return record;
}

Json CombatStatsToJson(const CombatStats& stats)
{
    return Json{
        {"attack", stats.attack},
        {"strength", stats.strength},
        {"defence", stats.defence},
        {"ranged", stats.ranged},
        {"magic", stats.magic},
        {"hitpoints", stats.hitpoints},
    };
}

Json EquipmentBonusesToJson(const EquipmentBonuses& bonuses)
{
    return Json{
        {"stabAttack", bonuses.stabAttack},
        {"slashAttack", bonuses.slashAttack},
        {"crushAttack", bonuses.crushAttack},
        {"magicAttack", bonuses.magicAttack},
        {"rangedAttack", bonuses.rangedAttack},
        {"stabDefence", bonuses.stabDefence},
        {"slashDefence", bonuses.slashDefence},
        {"crushDefence", bonuses.crushDefence},
        {"magicDefence", bonuses.magicDefence},
        {"rangedDefenceLight", bonuses.rangedDefenceLight},
        {"rangedDefenceStandard", bonuses.rangedDefenceStandard},
        {"rangedDefenceHeavy", bonuses.rangedDefenceHeavy},
        {"meleeStrength", bonuses.meleeStrength},
        {"rangedStrength", bonuses.rangedStrength},
        {"magicDamagePercent", bonuses.magicDamagePercent},
    };
}

Json CombatCompositionRecordToJson(const CombatCompositionRecord& record)
{
    return Json{
        {"id", record.id},
        {"name", record.name},
        {"stats", CombatStatsToJson(record.composition.stats)},
        {"bonuses", EquipmentBonusesToJson(record.composition.bonuses)},
        {"attackType", AttackTypeToString(record.composition.attackType)},
        {"magicBaseMaximumHit", record.composition.magicBaseMaximumHit},
        {"weaponId", record.composition.weapon.id},
    };
}

}  // namespace

CombatCompositionDatabase CombatCompositionDatabase::LoadFromJson(
    const std::string& json,
    const WeaponDatabase& weaponDatabase)
{
    Json document;

    try
    {
        document = Json::parse(json);
    }
    catch (const Json::parse_error& error)
    {
        throw std::invalid_argument(error.what());
    }

    RequireObject(document, "combat composition document");

    if (!HasOnlyKeys(document, {"version", "combatCompositions"}))
    {
        throw std::invalid_argument(
            "combat composition document contains an unknown field");
    }

    if (GetRequiredInt(document, "version") !=
        SupportedCombatCompositionJsonVersion)
    {
        throw std::invalid_argument(
            "unsupported combat composition document version");
    }

    if (!document.contains("combatCompositions") ||
        !document.at("combatCompositions").is_array())
    {
        throw std::invalid_argument("combatCompositions must be an array");
    }

    CombatCompositionDatabase database;

    for (const Json& recordJson : document.at("combatCompositions"))
    {
        database.AddBuiltInCombatCompositionRecord(
            ParseCombatCompositionRecord(
                recordJson,
                weaponDatabase,
                CombatCompositionSource::BuiltIn));
    }

    return database;
}

void CombatCompositionDatabase::LoadSavedCombatCompositionRecordsFromJson(
    const std::string& json,
    const WeaponDatabase& weaponDatabase)
{
    Json document;

    try
    {
        document = Json::parse(json);
    }
    catch (const Json::parse_error& error)
    {
        throw std::invalid_argument(error.what());
    }

    RequireObject(document, "saved combat composition document");

    if (!HasOnlyKeys(document, {"version", "savedCombatCompositions"}))
    {
        throw std::invalid_argument(
            "saved combat composition document contains an unknown field");
    }

    if (GetRequiredInt(document, "version") !=
        SupportedSavedCombatCompositionJsonVersion)
    {
        throw std::invalid_argument(
            "unsupported saved combat composition document version");
    }

    if (!document.contains("savedCombatCompositions") ||
        !document.at("savedCombatCompositions").is_array())
    {
        throw std::invalid_argument(
            "savedCombatCompositions must be an array");
    }

    CombatCompositionDatabase loadedDatabase;
    for (const CombatCompositionRecord& record : m_BuiltInRecords)
    {
        loadedDatabase.AddBuiltInCombatCompositionRecord(record);
    }

    for (const Json& recordJson : document.at("savedCombatCompositions"))
    {
        loadedDatabase.AddSavedCombatCompositionRecord(
            ParseCombatCompositionRecord(
                recordJson,
                weaponDatabase,
                CombatCompositionSource::Saved));
    }

    m_SavedRecords = loadedDatabase.m_SavedRecords;
    m_SavedRecordIndexById = loadedDatabase.m_SavedRecordIndexById;
}

std::string CombatCompositionDatabase::ExportSavedCombatCompositionRecordsToJson()
    const
{
    Json document;
    document["version"] = SupportedSavedCombatCompositionJsonVersion;
    document["savedCombatCompositions"] = Json::array();

    for (const CombatCompositionRecord& record : m_SavedRecords)
    {
        document["savedCombatCompositions"].push_back(
            CombatCompositionRecordToJson(record));
    }

    return document.dump();
}

CombatCompositionId CombatCompositionDatabase::CreateSavedCombatCompositionRecord(
    const std::string& name,
    const CombatComposition& composition)
{
    for (CombatCompositionId id = FirstSavedCombatCompositionId;
         id < std::numeric_limits<CombatCompositionId>::max();
         ++id)
    {
        if (!m_SavedRecordIndexById.contains(id))
        {
            CombatCompositionRecord record;
            record.id = id;
            record.name = name;
            record.source = CombatCompositionSource::Saved;
            record.composition = composition;
            AddSavedCombatCompositionRecord(record);
            return id;
        }
    }

    throw std::runtime_error("no saved combat composition IDs are available");
}

void CombatCompositionDatabase::UpdateSavedCombatCompositionRecord(
    CombatCompositionId id,
    const std::string& name,
    const CombatComposition& composition)
{
    if (id < FirstSavedCombatCompositionId)
    {
        throw std::invalid_argument(
            "saved combat composition ID is outside the saved range");
    }

    const auto iterator = m_SavedRecordIndexById.find(id);
    if (iterator == m_SavedRecordIndexById.end())
    {
        throw std::out_of_range("saved combat composition ID was not found");
    }

    if (name.empty())
    {
        throw std::invalid_argument(
            "combat composition name must not be empty");
    }

    CombatCompositionRecord& record = m_SavedRecords[iterator->second];
    record.name = name;
    record.composition = composition;
}

bool CombatCompositionDatabase::DeleteSavedCombatCompositionRecord(
    CombatCompositionId id)
{
    const auto iterator = m_SavedRecordIndexById.find(id);
    if (iterator == m_SavedRecordIndexById.end())
    {
        return false;
    }

    m_SavedRecords.erase(m_SavedRecords.begin() + iterator->second);
    m_SavedRecordIndexById.clear();
    for (std::size_t index = 0; index < m_SavedRecords.size(); ++index)
    {
        m_SavedRecordIndexById.emplace(m_SavedRecords[index].id, index);
    }

    return true;
}

const CombatCompositionRecord*
CombatCompositionDatabase::TryGetCombatCompositionRecord(
    CombatCompositionId id) const
{
    const auto builtInIterator = m_BuiltInRecordIndexById.find(id);

    if (builtInIterator != m_BuiltInRecordIndexById.end())
    {
        return &m_BuiltInRecords[builtInIterator->second];
    }

    const auto savedIterator = m_SavedRecordIndexById.find(id);
    if (savedIterator != m_SavedRecordIndexById.end())
    {
        return &m_SavedRecords[savedIterator->second];
    }

    return nullptr;
}

bool CombatCompositionDatabase::HasCombatCompositionRecord(
    CombatCompositionId id) const
{
    return TryGetCombatCompositionRecord(id) != nullptr;
}

CombatCompositionRecord CombatCompositionDatabase::GetCombatCompositionRecord(
    CombatCompositionId id) const
{
    const CombatCompositionRecord* record =
        TryGetCombatCompositionRecord(id);

    if (record == nullptr)
    {
        throw std::out_of_range("combat composition ID was not found");
    }

    return *record;
}

std::vector<CombatCompositionRecord>
CombatCompositionDatabase::GetAllCombatCompositionRecords() const
{
    std::vector<CombatCompositionRecord> records = m_BuiltInRecords;
    records.insert(records.end(), m_SavedRecords.begin(), m_SavedRecords.end());
    return records;
}

std::vector<CombatCompositionRecord>
CombatCompositionDatabase::GetCombatCompositionRecordsBySource(
    CombatCompositionSource source) const
{
    if (source == CombatCompositionSource::BuiltIn)
    {
        return m_BuiltInRecords;
    }

    return m_SavedRecords;
}

void CombatCompositionDatabase::AddBuiltInCombatCompositionRecord(
    const CombatCompositionRecord& record)
{
    if (record.id >= FirstSavedCombatCompositionId)
    {
        throw std::invalid_argument(
            "built-in combat composition ID is in the saved range");
    }

    if (m_BuiltInRecordIndexById.contains(record.id))
    {
        throw std::invalid_argument("duplicate combat composition ID");
    }

    m_BuiltInRecordIndexById.emplace(record.id, m_BuiltInRecords.size());
    m_BuiltInRecords.push_back(record);
}

void CombatCompositionDatabase::AddSavedCombatCompositionRecord(
    const CombatCompositionRecord& record)
{
    if (record.id < FirstSavedCombatCompositionId)
    {
        throw std::invalid_argument(
            "saved combat composition ID is outside the saved range");
    }

    if (m_BuiltInRecordIndexById.contains(record.id) ||
        m_SavedRecordIndexById.contains(record.id))
    {
        throw std::invalid_argument("duplicate combat composition ID");
    }

    if (record.name.empty())
    {
        throw std::invalid_argument(
            "combat composition name must not be empty");
    }

    CombatCompositionRecord savedRecord = record;
    savedRecord.source = CombatCompositionSource::Saved;
    m_SavedRecordIndexById.emplace(savedRecord.id, m_SavedRecords.size());
    m_SavedRecords.push_back(savedRecord);
}

}  // namespace osrssim
