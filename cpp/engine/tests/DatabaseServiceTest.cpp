#include "CombatService.h"
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
        "equipment": "equipment.json",
        "weapons": "weapons.json"
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

const std::string WeaponsJson = R"({
    "version": 1,
    "weapons": [
        {
            "id": 0,
            "name": "Unarmed",
            "range": 1,
            "speed": 4,
            "attackCallbackName": "standard_attack"
        },
        {
            "id": 300,
            "name": "Bronze scimitar",
            "range": 1,
            "speed": 4,
            "attackCallbackName": "standard_attack"
        },
        {
            "id": 301,
            "name": "Bronze scimitar",
            "range": 2,
            "speed": 5,
            "attackCallbackName": "standard_attack"
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
            {{"equipment", EquipmentJson}, {"weapons", WeaponsJson}});

    const osrssim::EquipmentDatabase& equipmentDatabase =
        service.GetEquipmentDatabase();
    assert(equipmentDatabase.HasEquipmentPiece(100));
    assert(equipmentDatabase.GetEquipmentPiece(100).name == "Bronze scimitar");

    const osrssim::WeaponDatabase& weaponDatabase =
        service.GetWeaponDatabase();
    assert(weaponDatabase.HasWeaponRecord(0));
    assert(weaponDatabase.HasWeaponRecord(300));
    assert(weaponDatabase.GetWeaponRecord(0).name == "Unarmed");
    assert(weaponDatabase.GetWeaponRecord(0).weapon.id == 0);
    assert(weaponDatabase.GetWeaponRecord(0).weapon.range == 1);
    assert(weaponDatabase.GetWeaponRecord(0).weapon.speed == 4);
    assert(weaponDatabase.GetWeaponRecord(300).attackCallbackName ==
           "standard_attack");
    assert(weaponDatabase.GetAllWeaponRecords().size() == 3);

    assert(ThrowsInvalidArgument(
        []
        {
            osrssim::DatabaseService::LoadFromDocuments(
                R"({
                    "version": 1,
                    "documents": {
                        "equipment": "equipment.json",
                        "weapons": "weapons.json"
                    },
                    "extra": true
                })",
                {{"equipment", EquipmentJson}, {"weapons", WeaponsJson}});
        }));

    assert(ThrowsInvalidArgument(
        []
        {
            osrssim::DatabaseService::LoadFromDocuments(
                ManifestJson,
                {{"weapons", WeaponsJson}});
        }));

    assert(ThrowsInvalidArgument(
        []
        {
            osrssim::DatabaseService::LoadFromDocuments(
                ManifestJson,
                {{"equipment", EquipmentJson}});
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
                })"},
                 {"weapons", WeaponsJson}});
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
                })"},
                 {"weapons", WeaponsJson}});
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
                })"},
                 {"weapons", WeaponsJson}});
        }));

    assert(ThrowsInvalidArgument(
        []
        {
            osrssim::DatabaseService::LoadFromDocuments(
                ManifestJson,
                {{"equipment", EquipmentJson},
                 {"weapons", R"({
                    "version": 1,
                    "weapons": [
                        {
                            "id": 1,
                            "name": "First",
                            "range": 1,
                            "speed": 4,
                            "attackCallbackName": "standard_attack"
                        },
                        {
                            "id": 1,
                            "name": "Duplicate",
                            "range": 1,
                            "speed": 4,
                            "attackCallbackName": "standard_attack"
                        }
                    ]
                })"}});
        }));

    assert(ThrowsInvalidArgument(
        []
        {
            osrssim::DatabaseService::LoadFromDocuments(
                ManifestJson,
                {{"equipment", EquipmentJson},
                 {"weapons", R"({
                    "version": 1,
                    "weapons": [
                        {
                            "id": 1,
                            "name": "Broken",
                            "range": 0,
                            "speed": 4,
                            "attackCallbackName": "standard_attack"
                        }
                    ]
                })"}});
        }));

    assert(ThrowsInvalidArgument(
        []
        {
            osrssim::DatabaseService::LoadFromDocuments(
                ManifestJson,
                {{"equipment", EquipmentJson},
                 {"weapons", R"({
                    "version": 1,
                    "weapons": [
                        {
                            "id": 1,
                            "name": "Broken",
                            "range": 1,
                            "speed": 4,
                            "attackCallbackName": "missing_callback"
                        }
                    ]
                })"}});
        }));

    {
        osrssim::CombatService combatService;
        combatService.RegisterAttackCallbackName(
            "special_attack",
            [](
                osrssim::World&,
                osrssim::ActorId,
                osrssim::ActorId,
                osrssim::Tick,
                const osrssim::WeaponDefinition&)
            {
            });

        const osrssim::DatabaseService serviceWithCustomCallback =
            osrssim::DatabaseService::LoadFromDocuments(
                ManifestJson,
                {{"equipment", EquipmentJson},
                 {"weapons", R"({
                    "version": 1,
                    "weapons": [
                        {
                            "id": 0,
                            "name": "Unarmed",
                            "range": 1,
                            "speed": 4,
                            "attackCallbackName": "standard_attack"
                        },
                        {
                            "id": 900,
                            "name": "Shared special",
                            "range": 8,
                            "speed": 5,
                            "attackCallbackName": "special_attack"
                        },
                        {
                            "id": 901,
                            "name": "Shared special",
                            "range": 9,
                            "speed": 6,
                            "attackCallbackName": "special_attack"
                        }
                    ]
                })"}},
                combatService);

        assert(serviceWithCustomCallback.GetWeaponDatabase()
                   .GetWeaponRecord(900)
                   .attackCallbackName == "special_attack");
        assert(serviceWithCustomCallback.GetWeaponDatabase()
                   .GetWeaponRecord(901)
                   .attackCallbackName == "special_attack");
    }

    {
        osrssim::CombatService combatService;
        int attackCount = 0;
        osrssim::WeaponDefinition callbackWeapon;
        combatService.RegisterAttackCallbackName(
            "special_attack",
            [&attackCount, &callbackWeapon](
                osrssim::World&,
                osrssim::ActorId,
                osrssim::ActorId,
                osrssim::Tick,
                const osrssim::WeaponDefinition& weapon)
            {
                ++attackCount;
                callbackWeapon = weapon;
            });

        const osrssim::DatabaseService serviceWithCustomCallback =
            osrssim::DatabaseService::LoadFromDocuments(
                ManifestJson,
                {{"equipment", EquipmentJson},
                 {"weapons", R"({
                    "version": 1,
                    "weapons": [
                        {
                            "id": 0,
                            "name": "Unarmed",
                            "range": 1,
                            "speed": 4,
                            "attackCallbackName": "standard_attack"
                        },
                        {
                            "id": 900,
                            "name": "Shared special",
                            "range": 8,
                            "speed": 5,
                            "attackCallbackName": "special_attack"
                        }
                    ]
                })"}},
                combatService);

        serviceWithCustomCallback.ConfigureCombatService(combatService);

        osrssim::World world;
        const osrssim::ActorId attackerId = world.CreatePlayer(1, 1);
        const osrssim::ActorId targetId = world.CreateNpc(1, 1);
        assert(world.SetActorWeaponDefinition(attackerId, {900, 8, 5}));

        assert(combatService.DispatchAttack(world, attackerId, targetId, 1));
        assert(attackCount == 1);
        assert(callbackWeapon == (osrssim::WeaponDefinition{900, 8, 5}));
        assert(world.GetActorAttackTimer(attackerId) == 5);
    }

    assert(ThrowsInvalidArgument(
        []
        {
            osrssim::DatabaseService::LoadFromDocuments(
                ManifestJson,
                {{"equipment", EquipmentJson},
                 {"weapons", R"({
                    "version": 1,
                    "weapons": [
                        {
                            "id": 1,
                            "name": "No Unarmed",
                            "range": 1,
                            "speed": 4,
                            "attackCallbackName": "standard_attack"
                        }
                    ]
                })"}});
        }));

    return 0;
}
