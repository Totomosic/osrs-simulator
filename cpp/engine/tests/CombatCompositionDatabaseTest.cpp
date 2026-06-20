#include "CombatCompositionDatabase.h"
#include "WeaponDatabase.h"

#include <cassert>
#include <stdexcept>
#include <vector>

namespace
{

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
            "name": "Shortbow",
            "range": 7,
            "speed": 4,
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
            "weaponId": 300
        },
        {
            "id": 1001,
            "name": "Goblin",
            "stats": {
                "attack": 3,
                "strength": 2,
                "defence": 2,
                "ranged": 1,
                "magic": 1,
                "hitpoints": 7
            },
            "bonuses": {
                "rangedAttack": 4,
                "rangedStrength": 5
            },
            "attackType": "ranged_standard",
            "magicBaseMaximumHit": 0,
            "weaponId": 301
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

bool ThrowsOutOfRange(auto callback)
{
    try
    {
        callback();
    }
    catch (const std::out_of_range&)
    {
        return true;
    }

    return false;
}

}  // namespace

int main()
{
    const osrssim::WeaponDatabase weaponDatabase =
        osrssim::WeaponDatabase::LoadFromJson(WeaponsJson);
    const osrssim::CombatCompositionDatabase database =
        osrssim::CombatCompositionDatabase::LoadFromJson(
            CombatCompositionsJson,
            weaponDatabase);

    assert(database.HasCombatCompositionRecord(1000));
    assert(database.HasCombatCompositionRecord(1001));

    const osrssim::CombatCompositionRecord firstRecord =
        database.GetCombatCompositionRecord(1000);
    assert(firstRecord.id == 1000);
    assert(firstRecord.name == "Goblin");
    assert(firstRecord.source == osrssim::CombatCompositionSource::BuiltIn);
    assert(firstRecord.composition.stats.hitpoints == 5);
    assert(firstRecord.composition.attackType == osrssim::AttackType::Slash);
    assert(firstRecord.composition.weapon.id == 300);
    assert(firstRecord.composition.weapon.range == 1);
    assert(firstRecord.composition.weapon.speed == 4);

    const std::vector<osrssim::CombatCompositionRecord> records =
        database.GetAllCombatCompositionRecords();
    assert(records.size() == 2);
    assert(records[0].id == 1000);
    assert(records[1].id == 1001);
    assert(records[1].name == "Goblin");
    assert(records[1].composition.attackType ==
           osrssim::AttackType::RangedStandard);
    assert(records[1].composition.bonuses.rangedAttack == 4);
    assert(records[1].composition.weapon.id == 301);

    const std::vector<osrssim::CombatCompositionRecord> builtInRecords =
        database.GetCombatCompositionRecordsBySource(
            osrssim::CombatCompositionSource::BuiltIn);
    assert(builtInRecords.size() == 2);
    assert(builtInRecords[0].id == 1000);

    assert(database.GetCombatCompositionRecordsBySource(
                       osrssim::CombatCompositionSource::Saved)
               .empty());

    osrssim::CombatCompositionDatabase savedDatabase = database;
    osrssim::CombatComposition savedComposition = firstRecord.composition;
    savedComposition.stats.attack = 70;
    savedComposition.bonuses.slashAttack = 45;
    savedComposition.weapon = weaponDatabase.GetWeaponRecord(300).weapon;

    const osrssim::CombatCompositionId savedId =
        savedDatabase.CreateSavedCombatCompositionRecord(
            "Saved scimitar setup",
            savedComposition);
    assert(savedId ==
           osrssim::CombatCompositionDatabase::FirstSavedCombatCompositionId);
    assert(savedDatabase.HasCombatCompositionRecord(savedId));
    assert(savedDatabase.GetCombatCompositionRecord(savedId).source ==
           osrssim::CombatCompositionSource::Saved);
    assert(savedDatabase.GetCombatCompositionRecord(savedId).composition.stats.attack ==
           70);
    assert(database.GetCombatCompositionRecordsBySource(
                       osrssim::CombatCompositionSource::Saved)
               .empty());

    savedComposition.weapon = weaponDatabase.GetWeaponRecord(301).weapon;
    savedComposition.bonuses.rangedAttack = 32;
    savedDatabase.UpdateSavedCombatCompositionRecord(
        savedId,
        "Saved bow setup",
        savedComposition);
    assert(savedDatabase.GetCombatCompositionRecord(savedId).name ==
           "Saved bow setup");
    assert(savedDatabase.GetCombatCompositionRecord(savedId).composition.weapon.id ==
           301);

    const std::vector<osrssim::CombatCompositionRecord> savedRecords =
        savedDatabase.GetCombatCompositionRecordsBySource(
            osrssim::CombatCompositionSource::Saved);
    assert(savedRecords.size() == 1);
    assert(savedRecords[0].id == savedId);

    const std::vector<osrssim::CombatCompositionRecord> combinedRecords =
        savedDatabase.GetAllCombatCompositionRecords();
    assert(combinedRecords.size() == 3);
    assert(combinedRecords[0].source == osrssim::CombatCompositionSource::BuiltIn);
    assert(combinedRecords[2].source == osrssim::CombatCompositionSource::Saved);

    assert(ThrowsInvalidArgument(
        [&savedDatabase, savedComposition]
        {
            savedDatabase.UpdateSavedCombatCompositionRecord(
                1000,
                "Cannot mutate built-in",
                savedComposition);
        }));
    assert(ThrowsOutOfRange(
        [&savedDatabase, savedComposition]
        {
            savedDatabase.UpdateSavedCombatCompositionRecord(
                osrssim::CombatCompositionDatabase::
                        FirstSavedCombatCompositionId +
                    500,
                "Missing saved",
                savedComposition);
        }));
    assert(!savedDatabase.DeleteSavedCombatCompositionRecord(1000));

    const std::string exportedSavedJson =
        savedDatabase.ExportSavedCombatCompositionRecordsToJson();
    osrssim::CombatCompositionDatabase importedSavedDatabase = database;
    importedSavedDatabase.LoadSavedCombatCompositionRecordsFromJson(
        exportedSavedJson,
        weaponDatabase);
    assert(importedSavedDatabase.GetCombatCompositionRecordsBySource(
                                   osrssim::CombatCompositionSource::Saved)
               .size() == 1);
    assert(importedSavedDatabase.GetCombatCompositionRecord(savedId).name ==
           "Saved bow setup");
    assert(importedSavedDatabase.GetCombatCompositionRecord(savedId)
               .composition.weapon.id == 301);

    assert(importedSavedDatabase.DeleteSavedCombatCompositionRecord(savedId));
    assert(!importedSavedDatabase.HasCombatCompositionRecord(savedId));
    const osrssim::CombatCompositionId recreatedSavedId =
        importedSavedDatabase.CreateSavedCombatCompositionRecord(
            "Recreated",
            savedComposition);
    assert(recreatedSavedId == savedId);

    assert(ThrowsInvalidArgument(
        [&importedSavedDatabase, &weaponDatabase]
        {
            importedSavedDatabase.LoadSavedCombatCompositionRecordsFromJson(
                R"({
                    "version": 1,
                    "savedCombatCompositions": [
                        {
                            "id": 1000,
                            "name": "Outside saved range",
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
                            "weaponId": 300
                        }
                    ]
                })",
                weaponDatabase);
        }));
    assert(importedSavedDatabase.HasCombatCompositionRecord(recreatedSavedId));

    assert(ThrowsInvalidArgument(
        [&importedSavedDatabase, &weaponDatabase]
        {
            importedSavedDatabase.LoadSavedCombatCompositionRecordsFromJson(
                R"({
                    "version": 1,
                    "savedCombatCompositions": [
                        {
                            "id": 9000000000000000000,
                            "name": "Valid saved",
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
                            "weaponId": 300
                        },
                        {
                            "id": 9000000000000000001,
                            "name": "Missing weapon",
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
                })",
                weaponDatabase);
        }));
    assert(importedSavedDatabase.GetCombatCompositionRecordsBySource(
                                    osrssim::CombatCompositionSource::Saved)
               .size() == 1);
    assert(importedSavedDatabase.GetCombatCompositionRecord(recreatedSavedId)
               .name == "Recreated");

    assert(ThrowsInvalidArgument(
        [&weaponDatabase]
        {
            osrssim::CombatCompositionDatabase::LoadFromJson(
                R"({
                    "version": 1,
                    "combatCompositions": [
                        {
                            "id": 1000,
                            "name": "Broken",
                            "stats": {},
                            "bonuses": {},
                            "attackType": "slash",
                            "weaponId": 999
                        }
                    ]
                })",
                weaponDatabase);
        }));

    assert(ThrowsInvalidArgument(
        [&weaponDatabase]
        {
            osrssim::CombatCompositionDatabase::LoadFromJson(
                R"({
                    "version": 1,
                    "combatCompositions": [
                        {
                            "id": 1000,
                            "name": "First",
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
                            "weaponId": 300
                        },
                        {
                            "id": 1000,
                            "name": "Duplicate",
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
                            "weaponId": 300
                        }
                    ]
                })",
                weaponDatabase);
        }));

    assert(ThrowsInvalidArgument(
        [&weaponDatabase]
        {
            osrssim::CombatCompositionDatabase::LoadFromJson(
                R"({
                    "version": 1,
                    "combatCompositions": [
                        {
                            "id": 9000000000000000000,
                            "name": "Saved range",
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
                            "weaponId": 300
                        }
                    ]
                })",
                weaponDatabase);
        }));

    assert(ThrowsInvalidArgument(
        [&weaponDatabase]
        {
            osrssim::CombatCompositionDatabase::LoadFromJson(
                R"({
                    "version": 1,
                    "combatCompositions": [
                        {
                            "id": 1002,
                            "name": "Unknown field",
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
                            "weaponId": 300,
                            "extra": true
                        }
                    ]
                })",
                weaponDatabase);
        }));

    assert(ThrowsInvalidArgument(
        [&weaponDatabase]
        {
            osrssim::CombatCompositionDatabase::LoadFromJson(
                R"({
                    "version": 1,
                    "combatCompositions": [
                        {
                            "id": 1003,
                            "name": "Invalid stat",
                            "stats": {
                                "attack": 0,
                                "strength": 1,
                                "defence": 1,
                                "ranged": 1,
                                "magic": 1,
                                "hitpoints": 5
                            },
                            "bonuses": {},
                            "attackType": "slash",
                            "magicBaseMaximumHit": 0,
                            "weaponId": 300
                        }
                    ]
                })",
                weaponDatabase);
        }));

    assert(ThrowsInvalidArgument(
        [&weaponDatabase]
        {
            osrssim::CombatCompositionDatabase::LoadFromJson(
                R"({
                    "version": 1,
                    "combatCompositions": [
                        {
                            "id": 1004,
                            "name": "Invalid magic",
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
                            "magicBaseMaximumHit": -1,
                            "weaponId": 300
                        }
                    ]
                })",
                weaponDatabase);
        }));

    return 0;
}
