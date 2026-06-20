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
    if (!HasOnlyKeys(documents, {"equipment"}))
    {
        throw std::invalid_argument("documents contains an unknown dataset");
    }

    const std::string equipmentDocument =
        GetRequiredString(documents, "equipment");
    if (equipmentDocument.empty())
    {
        throw std::invalid_argument("equipment document path must not be empty");
    }
}

}  // namespace

DatabaseService DatabaseService::LoadFromDocuments(
    const std::string& manifestJson,
    const std::unordered_map<std::string, std::string>& documents)
{
    const Json manifest = ParseManifestJson(manifestJson);
    ValidateManifest(manifest);

    const auto equipmentDocument = documents.find("equipment");
    if (equipmentDocument == documents.end())
    {
        throw std::invalid_argument("equipment document is missing");
    }

    DatabaseService service;
    service.m_EquipmentDatabase =
        EquipmentDatabase::LoadFromJson(equipmentDocument->second);

    return service;
}

DatabaseService DatabaseService::LoadFromJsonDocuments(
    const std::string& manifestJson,
    const std::string& equipmentJson)
{
    return LoadFromDocuments(manifestJson, {{"equipment", equipmentJson}});
}

const EquipmentDatabase& DatabaseService::GetEquipmentDatabase() const
{
    return m_EquipmentDatabase;
}

}  // namespace osrssim
