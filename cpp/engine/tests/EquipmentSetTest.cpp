#include "EquipmentSet.h"

#include <cassert>
#include <vector>

namespace
{

osrssim::EquipmentPiece MakePiece(
    int id,
    osrssim::EquipmentSlot slot,
    const osrssim::EquipmentBonuses& bonuses)
{
    osrssim::EquipmentPiece piece;

    piece.id = id;
    piece.name = "Test piece";
    piece.slot = slot;
    piece.bonuses = bonuses;

    return piece;
}

osrssim::EquipmentPiece MakeWeapon(
    int id,
    const osrssim::EquipmentBonuses& bonuses,
    osrssim::WeaponDefinition weapon)
{
    osrssim::EquipmentPiece piece =
        MakePiece(id, osrssim::EquipmentSlot::Weapon, bonuses);

    piece.hasWeapon = true;
    piece.weapon = weapon;

    return piece;
}

}  // namespace

int main()
{
    osrssim::EquipmentBonuses amuletBonuses;
    amuletBonuses.stabAttack = 3;
    amuletBonuses.slashAttack = 4;
    amuletBonuses.meleeStrength = 10;

    osrssim::EquipmentBonuses replacementAmuletBonuses;
    replacementAmuletBonuses.slashAttack = 7;
    replacementAmuletBonuses.meleeStrength = 12;

    osrssim::EquipmentBonuses bodyBonuses;
    bodyBonuses.stabDefence = 82;
    bodyBonuses.slashDefence = 80;
    bodyBonuses.magicDefence = -30;
    bodyBonuses.rangedDefenceStandard = 80;

    osrssim::EquipmentBonuses weaponBonuses;
    weaponBonuses.slashAttack = 67;
    weaponBonuses.meleeStrength = 66;

    osrssim::EquipmentSet equipmentSet;
    equipmentSet.SetEquipmentPiece(
        MakePiece(100, osrssim::EquipmentSlot::Amulet, amuletBonuses));
    equipmentSet.SetEquipmentPiece(
        MakePiece(
            101,
            osrssim::EquipmentSlot::Amulet,
            replacementAmuletBonuses));
    equipmentSet.SetEquipmentPiece(
        MakePiece(102, osrssim::EquipmentSlot::Body, bodyBonuses));
    equipmentSet.SetEquipmentPiece(
        MakeWeapon(
            103,
            weaponBonuses,
            osrssim::WeaponDefinition{300, 1, 5}));

    const std::vector<osrssim::EquipmentPiece> pieces =
        equipmentSet.GetEquipmentPieces();
    assert(pieces.size() == 3);
    assert(equipmentSet.GetEquipmentPiece(osrssim::EquipmentSlot::Amulet).id == 101);

    const osrssim::EquipmentBonuses summedBonuses =
        equipmentSet.GetEquipmentBonuses();
    assert(summedBonuses.stabAttack == 0);
    assert(summedBonuses.slashAttack == 74);
    assert(summedBonuses.meleeStrength == 78);
    assert(summedBonuses.stabDefence == 82);
    assert(summedBonuses.slashDefence == 80);
    assert(summedBonuses.magicDefence == -30);
    assert(summedBonuses.rangedDefenceStandard == 80);

    osrssim::CombatStats combatStats;
    combatStats.attack = 99;
    combatStats.strength = 99;
    combatStats.defence = 75;

    const osrssim::AttackComposition attackComposition =
        equipmentSet.BuildAttackComposition(
            combatStats,
            osrssim::AttackType::Slash);
    assert(attackComposition.attackType == osrssim::AttackType::Slash);
    assert(attackComposition.stats.attack == 99);
    assert(attackComposition.bonuses.slashAttack == 74);
    assert(attackComposition.bonuses.meleeStrength == 78);
    assert(attackComposition.weapon.id == 300);
    assert(attackComposition.weapon.range == 1);
    assert(attackComposition.weapon.speed == 5);

    const osrssim::DefenceComposition defenceComposition =
        equipmentSet.BuildDefenceComposition(combatStats);
    assert(defenceComposition.stats.defence == 75);
    assert(defenceComposition.bonuses.stabDefence == 82);
    assert(defenceComposition.bonuses.magicDefence == -30);

    osrssim::EquipmentSet unarmedSet;
    const osrssim::AttackComposition unarmedAttackComposition =
        unarmedSet.BuildAttackComposition(
            combatStats,
            osrssim::AttackType::Crush);
    assert(unarmedAttackComposition.weapon.id == 0);
    assert(unarmedAttackComposition.weapon.range == 1);
    assert(unarmedAttackComposition.weapon.speed == 4);

    return 0;
}
