#include "CombatCompositionDatabase.h"

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

constexpr int SupportedCombatCompositionJsonVersion = 1;

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

CombatCompositionRecord ParseBuiltInCombatCompositionRecord(
    const Json& value,
    const WeaponDatabase& weaponDatabase)
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
    record.source = CombatCompositionSource::BuiltIn;
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
            ParseBuiltInCombatCompositionRecord(recordJson, weaponDatabase));
    }

    return database;
}

const CombatCompositionRecord*
CombatCompositionDatabase::TryGetCombatCompositionRecord(
    CombatCompositionId id) const
{
    const auto iterator = m_BuiltInRecordIndexById.find(id);

    if (iterator == m_BuiltInRecordIndexById.end())
    {
        return nullptr;
    }

    return &m_BuiltInRecords[iterator->second];
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
    return m_BuiltInRecords;
}

std::vector<CombatCompositionRecord>
CombatCompositionDatabase::GetCombatCompositionRecordsBySource(
    CombatCompositionSource source) const
{
    if (source == CombatCompositionSource::BuiltIn)
    {
        return m_BuiltInRecords;
    }

    return {};
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

}  // namespace osrssim
