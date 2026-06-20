#pragma once

#include "DpsService.h"

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

namespace osrssim
{

enum class EquipmentSlot
{
    Head,
    Cape,
    Amulet,
    Weapon,
    Body,
    Shield,
    Legs,
    Hands,
    Feet,
    Ring,
    Ammo
};

struct EquipmentPiece
{
    int id = 0;
    std::string name;
    EquipmentSlot slot = EquipmentSlot::Weapon;
    EquipmentBonuses bonuses;
    bool hasWeapon = false;
    WeaponId weaponId = 0;
};

class EquipmentDatabase
{
private:
    std::vector<EquipmentPiece> m_Pieces;
    std::unordered_map<int, std::size_t> m_PieceIndexById;

public:
    static EquipmentDatabase LoadFromJson(const std::string& json);
    static EquipmentDatabase LoadDefault();
    static const std::string& GetDefaultJson();

    const EquipmentPiece* TryGetEquipmentPiece(int id) const;
    bool HasEquipmentPiece(int id) const;
    EquipmentPiece GetEquipmentPiece(int id) const;
    std::vector<EquipmentPiece> GetAllEquipmentPieces() const;
    std::vector<EquipmentPiece> GetEquipmentPiecesBySlot(
        EquipmentSlot slot) const;

private:
    void AddEquipmentPiece(const EquipmentPiece& piece);
};

}  // namespace osrssim
