#include "WeaponDatabase.h"

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

constexpr int SupportedWeaponJsonVersion = 1;

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

std::string GetRequiredString(const Json& value, const std::string& key)
{
    if (!value.contains(key) || !value.at(key).is_string())
    {
        throw std::invalid_argument(key + " must be a string");
    }

    return value.at(key).get<std::string>();
}

WeaponRecord ParseWeaponRecord(const Json& value)
{
    RequireObject(value, "weapon record");

    if (!HasOnlyKeys(
            value,
            {"id", "name", "range", "speed", "attackCallbackName"}))
    {
        throw std::invalid_argument("weapon record contains an unknown field");
    }

    WeaponRecord record;
    record.weapon.id = GetRequiredWeaponId(value, "id");
    record.weapon.range = GetRequiredPositiveInt(value, "range");
    record.weapon.speed = GetRequiredPositiveInt(value, "speed");
    record.name = GetRequiredString(value, "name");
    record.attackCallbackName =
        GetRequiredString(value, "attackCallbackName");

    if (record.name.empty())
    {
        throw std::invalid_argument("weapon name must not be empty");
    }

    if (record.attackCallbackName.empty())
    {
        throw std::invalid_argument("attack callback name must not be empty");
    }

    return record;
}

}  // namespace

WeaponDatabase WeaponDatabase::LoadFromJson(const std::string& json)
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

    RequireObject(document, "weapon document");

    if (!HasOnlyKeys(document, {"version", "weapons"}))
    {
        throw std::invalid_argument("weapon document contains an unknown field");
    }

    if (GetRequiredInt(document, "version") != SupportedWeaponJsonVersion)
    {
        throw std::invalid_argument("unsupported weapon document version");
    }

    if (!document.contains("weapons") || !document.at("weapons").is_array())
    {
        throw std::invalid_argument("weapons must be an array");
    }

    WeaponDatabase database;

    for (const Json& recordJson : document.at("weapons"))
    {
        database.AddWeaponRecord(ParseWeaponRecord(recordJson));
    }

    const WeaponRecord* unarmed = database.TryGetWeaponRecord(0);
    if (unarmed == nullptr)
    {
        throw std::invalid_argument("weapon records require Unarmed weapon ID 0");
    }

    if (unarmed->name != "Unarmed" || unarmed->weapon.range != 1 ||
        unarmed->weapon.speed != 4)
    {
        throw std::invalid_argument(
            "Unarmed weapon record must use ID 0, range 1, and speed 4");
    }

    return database;
}

const WeaponRecord* WeaponDatabase::TryGetWeaponRecord(WeaponId id) const
{
    const auto iterator = m_RecordIndexById.find(id);

    if (iterator == m_RecordIndexById.end())
    {
        return nullptr;
    }

    return &m_Records[iterator->second];
}

bool WeaponDatabase::HasWeaponRecord(WeaponId id) const
{
    return TryGetWeaponRecord(id) != nullptr;
}

WeaponRecord WeaponDatabase::GetWeaponRecord(WeaponId id) const
{
    const WeaponRecord* record = TryGetWeaponRecord(id);

    if (record == nullptr)
    {
        throw std::out_of_range("weapon ID was not found");
    }

    return *record;
}

std::vector<WeaponRecord> WeaponDatabase::GetAllWeaponRecords() const
{
    return m_Records;
}

void WeaponDatabase::AddWeaponRecord(const WeaponRecord& record)
{
    if (m_RecordIndexById.contains(record.weapon.id))
    {
        throw std::invalid_argument("duplicate weapon ID");
    }

    m_RecordIndexById.emplace(record.weapon.id, m_Records.size());
    m_Records.push_back(record);
}

}  // namespace osrssim
