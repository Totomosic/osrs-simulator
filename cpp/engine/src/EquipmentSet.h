#pragma once

#include "EquipmentDatabase.h"
#include "WeaponDatabase.h"

#include <unordered_map>
#include <vector>

namespace osrssim
{

class EquipmentSet
{
private:
    std::unordered_map<EquipmentSlot, EquipmentPiece> m_PiecesBySlot;

public:
    void SetEquipmentPiece(const EquipmentPiece& piece);
    bool HasEquipmentPiece(EquipmentSlot slot) const;
    const EquipmentPiece* TryGetEquipmentPiece(EquipmentSlot slot) const;
    EquipmentPiece GetEquipmentPiece(EquipmentSlot slot) const;
    std::vector<EquipmentPiece> GetEquipmentPieces() const;
    EquipmentBonuses GetEquipmentBonuses() const;
    CombatComposition BuildCombatComposition(
        const CombatStats& stats,
        AttackType attackType,
        int magicBaseMaximumHit,
        const WeaponDatabase& weaponDatabase) const;
    AttackComposition BuildAttackComposition(
        const CombatStats& stats,
        AttackType attackType,
        const WeaponDatabase& weaponDatabase) const;
    DefenceComposition BuildDefenceComposition(
        const CombatStats& stats) const;
};

}  // namespace osrssim
