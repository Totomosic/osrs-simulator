#include "EquipmentSet.h"

#include <stdexcept>

namespace osrssim
{
namespace
{

void AddEquipmentBonuses(
    EquipmentBonuses& total,
    const EquipmentBonuses& bonuses)
{
    total.stabAttack += bonuses.stabAttack;
    total.slashAttack += bonuses.slashAttack;
    total.crushAttack += bonuses.crushAttack;
    total.magicAttack += bonuses.magicAttack;
    total.rangedAttack += bonuses.rangedAttack;
    total.stabDefence += bonuses.stabDefence;
    total.slashDefence += bonuses.slashDefence;
    total.crushDefence += bonuses.crushDefence;
    total.magicDefence += bonuses.magicDefence;
    total.rangedDefenceLight += bonuses.rangedDefenceLight;
    total.rangedDefenceStandard += bonuses.rangedDefenceStandard;
    total.rangedDefenceHeavy += bonuses.rangedDefenceHeavy;
    total.meleeStrength += bonuses.meleeStrength;
    total.rangedStrength += bonuses.rangedStrength;
    total.magicDamagePercent += bonuses.magicDamagePercent;
}

}  // namespace

void EquipmentSet::SetEquipmentPiece(const EquipmentPiece& piece)
{
    m_PiecesBySlot[piece.slot] = piece;
}

bool EquipmentSet::HasEquipmentPiece(EquipmentSlot slot) const
{
    return TryGetEquipmentPiece(slot) != nullptr;
}

const EquipmentPiece* EquipmentSet::TryGetEquipmentPiece(
    EquipmentSlot slot) const
{
    const auto iterator = m_PiecesBySlot.find(slot);

    if (iterator == m_PiecesBySlot.end())
    {
        return nullptr;
    }

    return &iterator->second;
}

EquipmentPiece EquipmentSet::GetEquipmentPiece(EquipmentSlot slot) const
{
    const EquipmentPiece* piece = TryGetEquipmentPiece(slot);

    if (piece == nullptr)
    {
        throw std::out_of_range("equipment slot is empty");
    }

    return *piece;
}

std::vector<EquipmentPiece> EquipmentSet::GetEquipmentPieces() const
{
    std::vector<EquipmentPiece> pieces;

    for (const auto& [slot, piece] : m_PiecesBySlot)
    {
        pieces.push_back(piece);
    }

    return pieces;
}

EquipmentBonuses EquipmentSet::GetEquipmentBonuses() const
{
    EquipmentBonuses total;

    for (const auto& [slot, piece] : m_PiecesBySlot)
    {
        AddEquipmentBonuses(total, piece.bonuses);
    }

    return total;
}

CombatComposition EquipmentSet::BuildCombatComposition(
    const CombatStats& stats,
    AttackType attackType,
    int magicBaseMaximumHit,
    const WeaponDatabase& weaponDatabase) const
{
    CombatComposition composition;

    composition.stats = stats;
    composition.bonuses = GetEquipmentBonuses();
    composition.attackType = attackType;
    composition.magicBaseMaximumHit = magicBaseMaximumHit;

    const EquipmentPiece* weaponPiece =
        TryGetEquipmentPiece(EquipmentSlot::Weapon);
    if (weaponPiece != nullptr && weaponPiece->hasWeapon)
    {
        composition.weapon =
            weaponDatabase.GetWeaponRecord(weaponPiece->weaponId).weapon;
    }
    else
    {
        composition.weapon = weaponDatabase.GetWeaponRecord(0).weapon;
    }

    return composition;
}

AttackComposition EquipmentSet::BuildAttackComposition(
    const CombatStats& stats,
    AttackType attackType,
    const WeaponDatabase& weaponDatabase) const
{
    const CombatComposition combatComposition =
        BuildCombatComposition(stats, attackType, 0, weaponDatabase);

    AttackComposition composition;
    composition.attackType = combatComposition.attackType;
    composition.stats = combatComposition.stats;
    composition.bonuses = combatComposition.bonuses;
    composition.weapon = combatComposition.weapon;

    return composition;
}

DefenceComposition EquipmentSet::BuildDefenceComposition(
    const CombatStats& stats) const
{
    DefenceComposition composition;

    composition.stats = stats;
    composition.bonuses = GetEquipmentBonuses();

    return composition;
}

}  // namespace osrssim
