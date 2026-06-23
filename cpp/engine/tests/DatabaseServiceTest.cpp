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
        "weapons": "weapons.json",
        "combatCompositions": "combat_compositions.json",
        "npcs": "npcs.json"
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
            "weaponId": 300
        }
    ]
})";

const std::string EmptyEquipmentJson = R"({
    "version": 1,
    "equipmentPieces": []
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
            "projectileId": 44,
            "attackCallbackName": "standard_attack"
        }
    ]
})";

const std::string CombatCompositionsJson = R"({
    "version": 1,
    "combatCompositions": [
        {
            "id": 1000,
            "name": "Goblin",
            "stats": {
                "attack": 1,
                "strength": 1,
                "defence": 1,
                "ranged": 1,
                "magic": 1,
                "hitpoints": 5
            },
            "bonuses": {},
            "attackType": "slash",
            "magicBaseMaximumHit": 0,
            "weaponId": 0
        }
    ]
})";

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

osrssim::CombatComposition CombatCompositionWithWeapon(
    osrssim::WeaponDefinition weapon)
{
    osrssim::CombatComposition combatComposition;
    combatComposition.weapon = weapon;
    return combatComposition;
}

}  // namespace

int main()
{
    const osrssim::DatabaseService service =
        osrssim::DatabaseService::LoadFromDocuments(
            ManifestJson,
            {
                {"equipment", EquipmentJson},
                {"weapons", WeaponsJson},
                {"combatCompositions", CombatCompositionsJson},
                {"npcs", NpcsJson},
            });

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
    assert(weaponDatabase.GetWeaponRecord(300).weapon.projectileId == 0);
    assert(weaponDatabase.GetWeaponRecord(301).weapon.projectileId == 44);
    assert(weaponDatabase.GetAllWeaponRecords().size() == 3);

    const osrssim::CombatCompositionDatabase& combatCompositionDatabase =
        service.GetCombatCompositionDatabase();
    assert(combatCompositionDatabase.HasCombatCompositionRecord(1000));
    assert(combatCompositionDatabase.GetCombatCompositionRecord(1000)
               .composition.weapon.id == 0);

    const osrssim::NpcDatabase& npcDatabase = service.GetNpcDatabase();
    assert(npcDatabase.HasNpcDefinition(2000));
    assert(npcDatabase.GetNpcDefinition(2000).combatCompositionId == 1000);
    assert(npcDatabase.GetNpcDefinition(2000).size == 1);

    assert(ThrowsInvalidArgument(
        []
        {
            osrssim::DatabaseService::LoadFromDocuments(
                R"({
                    "version": 1,
                    "documents": {
                        "equipment": "equipment.json",
                        "weapons": "weapons.json",
                        "combatCompositions": "combat_compositions.json"
                    },
                    "extra": true
                })",
                {
                    {"equipment", EquipmentJson},
                    {"weapons", WeaponsJson},
                    {"combatCompositions", CombatCompositionsJson},
                    {"npcs", NpcsJson},
                });
        }));

    assert(ThrowsInvalidArgument(
        []
        {
            osrssim::DatabaseService::LoadFromDocuments(
                ManifestJson,
                {
                    {"weapons", WeaponsJson},
                    {"combatCompositions", CombatCompositionsJson},
                    {"npcs", NpcsJson},
                });
        }));

    assert(ThrowsInvalidArgument(
        []
        {
            osrssim::DatabaseService::LoadFromDocuments(
                ManifestJson,
                {
                    {"equipment", EquipmentJson},
                    {"weapons", WeaponsJson},
                    {"npcs", NpcsJson},
                });
        }));

    assert(ThrowsInvalidArgument(
        []
        {
            osrssim::DatabaseService::LoadFromDocuments(
                ManifestJson,
                {
                    {"equipment", EquipmentJson},
                    {"combatCompositions", CombatCompositionsJson},
                    {"npcs", NpcsJson},
                });
        }));

    assert(ThrowsInvalidArgument(
        []
        {
            osrssim::DatabaseService::LoadFromDocuments(
                ManifestJson,
                {
                    {"equipment", EmptyEquipmentJson},
                    {"weapons", WeaponsJson},
                    {"combatCompositions", CombatCompositionsJson},
                });
        }));

    assert(ThrowsInvalidArgument(
        []
        {
            osrssim::DatabaseService::LoadFromDocuments(
                ManifestJson,
                {
                    {"equipment", EmptyEquipmentJson},
                    {"weapons", WeaponsJson},
                    {"combatCompositions", R"({
                        "version": 1,
                        "combatCompositions": [
                            {
                                "id": 1000,
                                "name": "Broken composition",
                                "stats": {
                                    "attack": 1,
                                    "strength": 1,
                                    "defence": 1,
                                    "ranged": 1,
                                    "magic": 1,
                                    "hitpoints": 5
                                },
                                "bonuses": {},
                                "attackType": "slash",
                                "magicBaseMaximumHit": 0,
                                "weaponId": 999
                            }
                        ]
                    })"},
                    {"npcs", NpcsJson},
                });
        }));

    assert(ThrowsInvalidArgument(
        []
        {
            osrssim::DatabaseService::LoadFromDocuments(
                ManifestJson,
                {
                    {"equipment", R"({
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
                    {"weapons", WeaponsJson},
                    {"combatCompositions", CombatCompositionsJson},
                    {"npcs", NpcsJson},
                });
        }));

    assert(ThrowsInvalidArgument(
        []
        {
            osrssim::DatabaseService::LoadFromDocuments(
                ManifestJson,
                {
                    {"equipment", R"({
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
                    {"weapons", WeaponsJson},
                    {"combatCompositions", CombatCompositionsJson},
                    {"npcs", NpcsJson},
                });
        }));

    assert(ThrowsInvalidArgument(
        []
        {
            osrssim::DatabaseService::LoadFromDocuments(
                ManifestJson,
                {
                    {"equipment", R"({
                    "version": 1,
                    "equipmentPieces": [
                        {
                            "id": 101,
                            "name": "Broken bow",
                            "slot": "weapon",
                            "bonuses": {},
                            "weaponId": 999
                        }
                    ]
                })"},
                    {"weapons", WeaponsJson},
                    {"combatCompositions", CombatCompositionsJson},
                    {"npcs", NpcsJson},
                });
        }));

    assert(ThrowsInvalidArgument(
        []
        {
            osrssim::DatabaseService::LoadFromDocuments(
                ManifestJson,
                {
                    {"equipment", EmptyEquipmentJson},
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
                })"},
                    {"combatCompositions", CombatCompositionsJson},
                    {"npcs", NpcsJson},
                });
        }));

    assert(ThrowsInvalidArgument(
        []
        {
            osrssim::DatabaseService::LoadFromDocuments(
                ManifestJson,
                {
                    {"equipment", EmptyEquipmentJson},
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
                })"},
                    {"combatCompositions", CombatCompositionsJson},
                    {"npcs", NpcsJson},
                });
        }));

    assert(ThrowsInvalidArgument(
        []
        {
            osrssim::DatabaseService::LoadFromDocuments(
                ManifestJson,
                {
                    {"equipment", EmptyEquipmentJson},
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
                })"},
                    {"combatCompositions", CombatCompositionsJson},
                    {"npcs", NpcsJson},
                });
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
                return true;
            });

        const osrssim::DatabaseService serviceWithCustomCallback =
            osrssim::DatabaseService::LoadFromDocuments(
                ManifestJson,
                {
                    {"equipment", EmptyEquipmentJson},
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
                })"},
                    {"combatCompositions", CombatCompositionsJson},
                    {"npcs", NpcsJson},
                },
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
                return true;
            });

        const osrssim::DatabaseService serviceWithCustomCallback =
            osrssim::DatabaseService::LoadFromDocuments(
                ManifestJson,
                {
                    {"equipment", EmptyEquipmentJson},
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
                })"},
                    {"combatCompositions", CombatCompositionsJson},
                    {"npcs", NpcsJson},
                },
                combatService);

        serviceWithCustomCallback.ConfigureCombatService(combatService);

        osrssim::World world;
        const osrssim::ActorId attackerId = world.CreatePlayer(1, 1, osrssim::CombatComposition{}).value();
        const osrssim::ActorId targetId = world.CreateNpc(1, 1, osrssim::CombatComposition{}).value();
        assert(world.SetActorCombatComposition(attackerId, CombatCompositionWithWeapon({900, 8, 5})));

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
                {
                    {"equipment", EmptyEquipmentJson},
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
                })"},
                    {"combatCompositions", CombatCompositionsJson},
                    {"npcs", NpcsJson},
                });
        }));

    assert(ThrowsInvalidArgument(
        []
        {
            osrssim::DatabaseService::LoadFromDocuments(
                ManifestJson,
                {
                    {"equipment", EmptyEquipmentJson},
                    {"weapons", WeaponsJson},
                    {"combatCompositions", CombatCompositionsJson},
                    {"npcs", R"({
                        "version": 1,
                        "npcs": [
                            {
                                "id": 2001,
                                "name": "Missing composition",
                                "size": 1,
                                "speed": 1,
                                "combatCompositionId": 9999
                            }
                        ]
                    })"},
                });
        }));

    assert(ThrowsInvalidArgument(
        []
        {
            osrssim::DatabaseService::LoadFromDocuments(
                ManifestJson,
                {
                    {"equipment", EmptyEquipmentJson},
                    {"weapons", WeaponsJson},
                    {"combatCompositions", CombatCompositionsJson},
                    {"npcs", R"({
                        "version": 1,
                        "npcs": [
                            {
                                "id": 2002,
                                "name": "Saved composition",
                                "size": 1,
                                "speed": 1,
                                "combatCompositionId": 9000000000000000000
                            }
                        ]
                    })"},
                });
        }));

    return 0;
}
