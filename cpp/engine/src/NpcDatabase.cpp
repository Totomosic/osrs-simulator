#include "NpcDatabase.h"

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

constexpr int SupportedNpcJsonVersion = 1;

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

std::string GetRequiredString(const Json& value, const std::string& key)
{
    if (!value.contains(key) || !value.at(key).is_string())
    {
        throw std::invalid_argument(key + " must be a string");
    }

    return value.at(key).get<std::string>();
}

NpcId GetRequiredNpcId(const Json& value, const std::string& key)
{
    if (!value.contains(key) || !value.at(key).is_number_integer())
    {
        throw std::invalid_argument(key + " must be an integer");
    }

    const Json& id = value.at(key);
    if (id.is_number_unsigned())
    {
        return id.get<NpcId>();
    }

    const long long signedId = id.get<long long>();
    if (signedId < 0)
    {
        throw std::invalid_argument(key + " must be non-negative");
    }

    return static_cast<NpcId>(signedId);
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

NpcDefinition ParseNpcDefinition(const Json& value)
{
    RequireObject(value, "NPC definition");

    if (!HasOnlyKeys(
            value,
            {
                "id",
                "name",
                "combatLevel",
                "size",
                "speed",
                "combatCompositionId",
            }))
    {
        throw std::invalid_argument("NPC definition contains an unknown field");
    }

    NpcDefinition definition;
    definition.id = GetRequiredNpcId(value, "id");
    definition.name = GetRequiredString(value, "name");
    definition.hasCombatLevel = value.contains("combatLevel");
    definition.combatLevel = GetOptionalInt(value, "combatLevel");
    definition.size = GetRequiredInt(value, "size");
    definition.speed = GetRequiredInt(value, "speed");
    definition.combatCompositionId =
        GetRequiredCombatCompositionId(value, "combatCompositionId");

    if (definition.name.empty())
    {
        throw std::invalid_argument("NPC name must not be empty");
    }
    if (definition.hasCombatLevel && definition.combatLevel < 0)
    {
        throw std::invalid_argument("combatLevel must be non-negative");
    }
    if (definition.size < 1)
    {
        throw std::invalid_argument("size must be at least 1");
    }
    if (definition.speed < 0)
    {
        throw std::invalid_argument("speed must be non-negative");
    }

    return definition;
}

}  // namespace

NpcDatabase NpcDatabase::LoadFromJson(const std::string& json)
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

    RequireObject(document, "NPC document");

    if (!HasOnlyKeys(document, {"version", "npcs"}))
    {
        throw std::invalid_argument("NPC document contains an unknown field");
    }

    if (GetRequiredInt(document, "version") != SupportedNpcJsonVersion)
    {
        throw std::invalid_argument("unsupported NPC document version");
    }

    if (!document.contains("npcs") || !document.at("npcs").is_array())
    {
        throw std::invalid_argument("npcs must be an array");
    }

    NpcDatabase database;

    for (const Json& definitionJson : document.at("npcs"))
    {
        database.AddNpcDefinition(ParseNpcDefinition(definitionJson));
    }

    return database;
}

const NpcDefinition* NpcDatabase::TryGetNpcDefinition(NpcId id) const
{
    const auto iterator = m_DefinitionIndexById.find(id);

    if (iterator == m_DefinitionIndexById.end())
    {
        return nullptr;
    }

    return &m_Definitions[iterator->second];
}

bool NpcDatabase::HasNpcDefinition(NpcId id) const
{
    return TryGetNpcDefinition(id) != nullptr;
}

NpcDefinition NpcDatabase::GetNpcDefinition(NpcId id) const
{
    const NpcDefinition* definition = TryGetNpcDefinition(id);

    if (definition == nullptr)
    {
        throw std::out_of_range("NPC ID was not found");
    }

    return *definition;
}

std::vector<NpcDefinition> NpcDatabase::GetAllNpcDefinitions() const
{
    return m_Definitions;
}

void NpcDatabase::AddNpcDefinition(const NpcDefinition& definition)
{
    if (m_DefinitionIndexById.contains(definition.id))
    {
        throw std::invalid_argument("duplicate NPC ID");
    }

    m_DefinitionIndexById.emplace(definition.id, m_Definitions.size());
    m_Definitions.push_back(definition);
}

}  // namespace osrssim
