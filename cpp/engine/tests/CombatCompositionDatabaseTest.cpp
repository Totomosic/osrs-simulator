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
