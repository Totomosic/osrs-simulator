#include "DatabaseService.h"

#include <cassert>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace
{

const std::string ManifestJson = R"({
    "version": 1,
    "documents": {
        "equipment": "equipment.json"
    }
})";

const std::string EquipmentJson = R"({
    "version": 1,
    "equipmentPieces": [
        {
            "id": 100,
            "name": "Bronze scimitar",
            "slot": "weapon",
            "bonuses": {
                "slashAttack": 7,
                "meleeStrength": 6
            },
            "weapon": {
                "id": 300,
                "range": 1,
                "speed": 4
            }
        }
    ]
})";

bool ThrowsInvalidArgument(auto callback)
{
    try
    {
        callback();
    }
    catch (const std::invalid_argument&)
    {
        return true;
    }

    return false;
}

}  // namespace

int main()
{
    const osrssim::DatabaseService service =
        osrssim::DatabaseService::LoadFromDocuments(
            ManifestJson,
            {{"equipment", EquipmentJson}});

    const osrssim::EquipmentDatabase& equipmentDatabase =
        service.GetEquipmentDatabase();
    assert(equipmentDatabase.HasEquipmentPiece(100));
    assert(equipmentDatabase.GetEquipmentPiece(100).name == "Bronze scimitar");

    assert(ThrowsInvalidArgument(
        []
        {
            osrssim::DatabaseService::LoadFromDocuments(
                R"({
                    "version": 1,
                    "documents": {
                        "equipment": "equipment.json"
                    },
                    "extra": true
                })",
                {{"equipment", EquipmentJson}});
        }));

    assert(ThrowsInvalidArgument(
        []
        {
            osrssim::DatabaseService::LoadFromDocuments(
                ManifestJson,
                {});
        }));

    assert(ThrowsInvalidArgument(
        []
        {
            osrssim::DatabaseService::LoadFromDocuments(
                ManifestJson,
                {{"equipment", R"({
                    "version": 1,
                    "equipmentPieces": [
                        {
                            "id": 100,
                            "name": "First",
                            "slot": "amulet",
                            "bonuses": {}
                        },
                        {
                            "id": 100,
                            "name": "Duplicate",
                            "slot": "ring",
                            "bonuses": {}
                        }
                    ]
                })"}});
        }));

    assert(ThrowsInvalidArgument(
        []
        {
            osrssim::DatabaseService::LoadFromDocuments(
                ManifestJson,
                {{"equipment", R"({
                    "version": 1,
                    "equipmentPieces": [
                        {
                            "id": -1,
                            "name": "Invalid",
                            "slot": "ring",
                            "bonuses": {}
                        }
                    ]
                })"}});
        }));

    assert(ThrowsInvalidArgument(
        []
        {
            osrssim::DatabaseService::LoadFromDocuments(
                ManifestJson,
                {{"equipment", R"({
                    "version": 1,
                    "equipmentPieces": [
                        {
                            "id": 101,
                            "name": "Broken bow",
                            "slot": "weapon",
                            "bonuses": {},
                            "weapon": {
                                "id": 301,
                                "range": 0,
                                "speed": 4
                            }
                        }
                    ]
                })"}});
        }));

    return 0;
}
