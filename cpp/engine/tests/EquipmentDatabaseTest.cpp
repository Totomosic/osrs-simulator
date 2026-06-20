#include "EquipmentDatabase.h"

#include <cassert>
#include <stdexcept>
#include <string>
#include <vector>

int main()
{
    const std::string json = R"({
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
            },
            {
                "id": 101,
                "name": "Amulet of strength",
                "slot": "amulet",
                "bonuses": {
                    "meleeStrength": 10
                }
            }
        ]
    })";

    const osrssim::EquipmentDatabase database =
        osrssim::EquipmentDatabase::LoadFromJson(json);

    const osrssim::EquipmentPiece* scimitar =
        database.TryGetEquipmentPiece(100);
    assert(scimitar != nullptr);
    assert(scimitar->id == 100);
    assert(scimitar->name == "Bronze scimitar");
    assert(scimitar->slot == osrssim::EquipmentSlot::Weapon);
    assert(scimitar->bonuses.slashAttack == 7);
    assert(scimitar->bonuses.stabAttack == 0);
    assert(scimitar->bonuses.meleeStrength == 6);
    assert(scimitar->hasWeapon);
    assert(scimitar->weaponId == 300);

    const osrssim::EquipmentPiece* amulet =
        database.TryGetEquipmentPiece(101);
    assert(amulet != nullptr);
    assert(amulet->slot == osrssim::EquipmentSlot::Amulet);
    assert(amulet->bonuses.meleeStrength == 10);
    assert(!amulet->hasWeapon);
    assert(database.TryGetEquipmentPiece(999) == nullptr);

    const std::vector<osrssim::EquipmentPiece> allPieces =
        database.GetAllEquipmentPieces();
    assert(allPieces.size() == 2);

    const std::vector<osrssim::EquipmentPiece> weapons =
        database.GetEquipmentPiecesBySlot(osrssim::EquipmentSlot::Weapon);
    assert(weapons.size() == 1);
    assert(weapons[0].id == 100);

    bool duplicateFailed = false;
    try
    {
        osrssim::EquipmentDatabase::LoadFromJson(R"({
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
                    "name": "Second",
                    "slot": "ring",
                    "bonuses": {}
                }
            ]
        })");
    }
    catch (const std::invalid_argument&)
    {
        duplicateFailed = true;
    }
    assert(duplicateFailed);

    bool weaponWithoutIdFailed = false;
    try
    {
        osrssim::EquipmentDatabase::LoadFromJson(R"({
            "version": 1,
            "equipmentPieces": [
                {
                    "id": 102,
                    "name": "Broken sword",
                    "slot": "weapon",
                    "bonuses": {}
                }
            ]
        })");
    }
    catch (const std::invalid_argument&)
    {
        weaponWithoutIdFailed = true;
    }
    assert(weaponWithoutIdFailed);

    bool weaponWithUnarmedIdFailed = false;
    try
    {
        osrssim::EquipmentDatabase::LoadFromJson(R"({
            "version": 1,
            "equipmentPieces": [
                {
                    "id": 102,
                    "name": "Broken sword",
                    "slot": "weapon",
                    "bonuses": {},
                    "weaponId": 0
                }
            ]
        })");
    }
    catch (const std::invalid_argument&)
    {
        weaponWithUnarmedIdFailed = true;
    }
    assert(weaponWithUnarmedIdFailed);

    bool nonWeaponWithDefinitionFailed = false;
    try
    {
        osrssim::EquipmentDatabase::LoadFromJson(R"({
            "version": 1,
            "equipmentPieces": [
                {
                    "id": 103,
                    "name": "Armed amulet",
                    "slot": "amulet",
                    "bonuses": {},
                    "weaponId": 103
                }
            ]
        })");
    }
    catch (const std::invalid_argument&)
    {
        nonWeaponWithDefinitionFailed = true;
    }
    assert(nonWeaponWithDefinitionFailed);

    return 0;
}
