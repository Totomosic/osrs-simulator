#include "NpcDatabase.h"

#include <cassert>
#include <stdexcept>
#include <vector>

namespace
{

const std::string NpcsJson = R"({
    "version": 1,
    "npcs": [
        {
            "id": 2000,
            "name": "Goblin",
            "combatLevel": 2,
            "size": 1,
            "speed": 1,
            "combatCompositionId": 1000
        },
        {
            "id": 2001,
            "name": "Goblin",
            "size": 2,
            "speed": 0,
            "combatCompositionId": 1001
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
    const osrssim::NpcDatabase database =
        osrssim::NpcDatabase::LoadFromJson(NpcsJson);

    assert(database.HasNpcDefinition(2000));
    assert(database.HasNpcDefinition(2001));

    const osrssim::NpcDefinition firstDefinition =
        database.GetNpcDefinition(2000);
    assert(firstDefinition.id == 2000);
    assert(firstDefinition.name == "Goblin");
    assert(firstDefinition.hasCombatLevel);
    assert(firstDefinition.combatLevel == 2);
    assert(firstDefinition.size == 1);
    assert(firstDefinition.speed == 1);
    assert(firstDefinition.combatCompositionId == 1000);

    const std::vector<osrssim::NpcDefinition> definitions =
        database.GetAllNpcDefinitions();
    assert(definitions.size() == 2);
    assert(definitions[0].id == 2000);
    assert(definitions[1].id == 2001);
    assert(definitions[1].name == "Goblin");
    assert(!definitions[1].hasCombatLevel);
    assert(definitions[1].size == 2);
    assert(definitions[1].speed == 0);

    assert(ThrowsInvalidArgument(
        []
        {
            osrssim::NpcDatabase::LoadFromJson(R"({
                "version": 1,
                "npcs": [
                    {
                        "id": 2000,
                        "name": "First",
                        "size": 1,
                        "speed": 1,
                        "combatCompositionId": 1000
                    },
                    {
                        "id": 2000,
                        "name": "Duplicate",
                        "size": 1,
                        "speed": 1,
                        "combatCompositionId": 1000
                    }
                ]
            })");
        }));

    assert(ThrowsInvalidArgument(
        []
        {
            osrssim::NpcDatabase::LoadFromJson(R"({
                "version": 1,
                "npcs": [
                    {
                        "id": 2002,
                        "name": "Invalid size",
                        "size": 0,
                        "speed": 1,
                        "combatCompositionId": 1000
                    }
                ]
            })");
        }));

    assert(ThrowsInvalidArgument(
        []
        {
            osrssim::NpcDatabase::LoadFromJson(R"({
                "version": 1,
                "npcs": [
                    {
                        "id": 2003,
                        "name": "Invalid speed",
                        "size": 1,
                        "speed": -1,
                        "combatCompositionId": 1000
                    }
                ]
            })");
        }));

    return 0;
}
