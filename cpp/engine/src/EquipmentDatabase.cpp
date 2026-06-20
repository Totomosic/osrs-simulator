#include "EquipmentDatabase.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <initializer_list>
#include <stdexcept>
#include <string>
#include <string_view>

namespace osrssim
{
namespace
{

using Json = nlohmann::json;

constexpr int SupportedEquipmentJsonVersion = 1;

bool HasOnlyKeys(
    const Json& value,
    std::initializer_list<std::string_view> allowedKeys)
{
    for (const auto& item : value.items())
    {
        const std::string_view key = item.key();
        const bool isAllowed = std::find(
            allowedKeys.begin(),
            allowedKeys.end(),
            key) != allowedKeys.end();

        if (!isAllowed)
        {
            return false;
        }
    }

    return true;
}

void RequireObject(
    const Json& value,
    const std::string& description)
{
    if (!value.is_object())
    {
        throw std::invalid_argument(description + " must be an object");
    }
}

int GetRequiredInt(const Json& value, const std::string& key)
{
    if (!value.contains(key) || !value.at(key).is_number_integer())
    {
        throw std::invalid_argument(key + " must be an integer");
    }

    return value.at(key).get<int>();
}

int GetRequiredNonNegativeInt(const Json& value, const std::string& key)
{
    const int result = GetRequiredInt(value, key);
    if (result < 0)
    {
        throw std::invalid_argument(key + " must be non-negative");
    }

    return result;
}

int GetRequiredPositiveInt(const Json& value, const std::string& key)
{
    const int result = GetRequiredInt(value, key);
    if (result < 1)
    {
        throw std::invalid_argument(key + " must be at least 1");
    }

    return result;
}

int GetOptionalInt(const Json& value, const std::string& key)
{
    if (!value.contains(key))
    {
        return 0;
    }

    if (!value.at(key).is_number_integer())
    {
        throw std::invalid_argument(key + " must be an integer");
    }

    return value.at(key).get<int>();
}

double GetOptionalDouble(const Json& value, const std::string& key)
{
    if (!value.contains(key))
    {
        return 0.0;
    }

    if (!value.at(key).is_number())
    {
        throw std::invalid_argument(key + " must be a number");
    }

    return value.at(key).get<double>();
}

std::string GetRequiredString(const Json& value, const std::string& key)
{
    if (!value.contains(key) || !value.at(key).is_string())
    {
        throw std::invalid_argument(key + " must be a string");
    }

    return value.at(key).get<std::string>();
}

EquipmentSlot ParseEquipmentSlot(const std::string& slot)
{
    if (slot == "head")
    {
        return EquipmentSlot::Head;
    }
    if (slot == "cape")
    {
        return EquipmentSlot::Cape;
    }
    if (slot == "amulet")
    {
        return EquipmentSlot::Amulet;
    }
    if (slot == "weapon")
    {
        return EquipmentSlot::Weapon;
    }
    if (slot == "body")
    {
        return EquipmentSlot::Body;
    }
    if (slot == "shield")
    {
        return EquipmentSlot::Shield;
    }
    if (slot == "legs")
    {
        return EquipmentSlot::Legs;
    }
    if (slot == "hands")
    {
        return EquipmentSlot::Hands;
    }
    if (slot == "feet")
    {
        return EquipmentSlot::Feet;
    }
    if (slot == "ring")
    {
        return EquipmentSlot::Ring;
    }
    if (slot == "ammo")
    {
        return EquipmentSlot::Ammo;
    }

    throw std::invalid_argument("slot is not a supported equipment slot");
}

EquipmentBonuses ParseEquipmentBonuses(const Json& value)
{
    RequireObject(value, "bonuses");

    if (!HasOnlyKeys(
            value,
            {
                "stabAttack",
                "slashAttack",
                "crushAttack",
                "magicAttack",
                "rangedAttack",
                "stabDefence",
                "slashDefence",
                "crushDefence",
                "magicDefence",
                "rangedDefenceLight",
                "rangedDefenceStandard",
                "rangedDefenceHeavy",
                "meleeStrength",
                "rangedStrength",
                "magicDamagePercent",
            }))
    {
        throw std::invalid_argument("bonuses contains an unknown field");
    }

    EquipmentBonuses bonuses;

    bonuses.stabAttack = GetOptionalInt(value, "stabAttack");
    bonuses.slashAttack = GetOptionalInt(value, "slashAttack");
    bonuses.crushAttack = GetOptionalInt(value, "crushAttack");
    bonuses.magicAttack = GetOptionalInt(value, "magicAttack");
    bonuses.rangedAttack = GetOptionalInt(value, "rangedAttack");
    bonuses.stabDefence = GetOptionalInt(value, "stabDefence");
    bonuses.slashDefence = GetOptionalInt(value, "slashDefence");
    bonuses.crushDefence = GetOptionalInt(value, "crushDefence");
    bonuses.magicDefence = GetOptionalInt(value, "magicDefence");
    bonuses.rangedDefenceLight =
        GetOptionalInt(value, "rangedDefenceLight");
    bonuses.rangedDefenceStandard =
        GetOptionalInt(value, "rangedDefenceStandard");
    bonuses.rangedDefenceHeavy =
        GetOptionalInt(value, "rangedDefenceHeavy");
    bonuses.meleeStrength = GetOptionalInt(value, "meleeStrength");
    bonuses.rangedStrength = GetOptionalInt(value, "rangedStrength");
    bonuses.magicDamagePercent =
        GetOptionalDouble(value, "magicDamagePercent");

    return bonuses;
}

WeaponId GetRequiredWeaponId(const Json& value, const std::string& key)
{
    if (!value.contains(key) || !value.at(key).is_number_integer())
    {
        throw std::invalid_argument(key + " must be an integer");
    }

    const Json& id = value.at(key);
    if (id.is_number_unsigned())
    {
        return id.get<WeaponId>();
    }

    const long long signedId = id.get<long long>();
    if (signedId < 0)
    {
        throw std::invalid_argument(key + " must be non-negative");
    }

    return static_cast<WeaponId>(signedId);
}

EquipmentPiece ParseEquipmentPiece(const Json& value)
{
    RequireObject(value, "equipment piece");

    if (!HasOnlyKeys(value, {"id", "name", "slot", "bonuses", "weaponId"}))
    {
        throw std::invalid_argument("equipment piece contains an unknown field");
    }

    if (!value.contains("bonuses"))
    {
        throw std::invalid_argument("bonuses is required");
    }

    EquipmentPiece piece;

    piece.id = GetRequiredNonNegativeInt(value, "id");
    piece.name = GetRequiredString(value, "name");
    piece.slot = ParseEquipmentSlot(GetRequiredString(value, "slot"));
    piece.bonuses = ParseEquipmentBonuses(value.at("bonuses"));

    if (value.contains("weaponId"))
    {
        piece.hasWeapon = true;
        piece.weaponId = GetRequiredWeaponId(value, "weaponId");
    }

    if (piece.slot == EquipmentSlot::Weapon && !piece.hasWeapon)
    {
        throw std::invalid_argument(
            "weapon-slot equipment pieces require a weapon ID");
    }

    if (piece.slot == EquipmentSlot::Weapon && piece.weaponId == 0)
    {
        throw std::invalid_argument(
            "weapon-slot equipment pieces require a non-zero weapon ID");
    }

    if (piece.slot != EquipmentSlot::Weapon && piece.hasWeapon)
    {
        throw std::invalid_argument(
            "non-weapon equipment pieces cannot have a weapon ID");
    }

    return piece;
}

}  // namespace

EquipmentDatabase EquipmentDatabase::LoadFromJson(const std::string& json)
{
    Json document;

    try
    {
        document = Json::parse(json);
    }
    catch (const Json::parse_error& error)
    {
        throw std::invalid_argument(error.what());
    }

    RequireObject(document, "equipment document");

    if (!HasOnlyKeys(document, {"version", "equipmentPieces"}))
    {
        throw std::invalid_argument(
            "equipment document contains an unknown field");
    }

    if (GetRequiredInt(document, "version") != SupportedEquipmentJsonVersion)
    {
        throw std::invalid_argument("unsupported equipment document version");
    }

    if (!document.contains("equipmentPieces") ||
        !document.at("equipmentPieces").is_array())
    {
        throw std::invalid_argument("equipmentPieces must be an array");
    }

    EquipmentDatabase database;

    for (const Json& pieceJson : document.at("equipmentPieces"))
    {
        database.AddEquipmentPiece(ParseEquipmentPiece(pieceJson));
    }

    return database;
}

const EquipmentPiece* EquipmentDatabase::TryGetEquipmentPiece(int id) const
{
    const auto iterator = m_PieceIndexById.find(id);

    if (iterator == m_PieceIndexById.end())
    {
        return nullptr;
    }

    return &m_Pieces[iterator->second];
}

bool EquipmentDatabase::HasEquipmentPiece(int id) const
{
    return TryGetEquipmentPiece(id) != nullptr;
}

EquipmentPiece EquipmentDatabase::GetEquipmentPiece(int id) const
{
    const EquipmentPiece* piece = TryGetEquipmentPiece(id);

    if (piece == nullptr)
    {
        throw std::out_of_range("equipment piece ID was not found");
    }

    return *piece;
}

std::vector<EquipmentPiece> EquipmentDatabase::GetAllEquipmentPieces() const
{
    return m_Pieces;
}

std::vector<EquipmentPiece> EquipmentDatabase::GetEquipmentPiecesBySlot(
    EquipmentSlot slot) const
{
    std::vector<EquipmentPiece> pieces;

    for (const EquipmentPiece& piece : m_Pieces)
    {
        if (piece.slot == slot)
        {
            pieces.push_back(piece);
        }
    }

    return pieces;
}

void EquipmentDatabase::AddEquipmentPiece(const EquipmentPiece& piece)
{
    if (m_PieceIndexById.contains(piece.id))
    {
        throw std::invalid_argument("duplicate equipment piece ID");
    }

    m_PieceIndexById.emplace(piece.id, m_Pieces.size());
    m_Pieces.push_back(piece);
}

}  // namespace osrssim
