#pragma once

#include "Types.h"

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

namespace osrssim
{

struct WeaponRecord
{
    WeaponDefinition weapon;
    std::string name;
    std::string attackCallbackName;
};

class WeaponDatabase
{
private:
    std::vector<WeaponRecord> m_Records;
    std::unordered_map<WeaponId, std::size_t> m_RecordIndexById;

public:
    static WeaponDatabase LoadFromJson(const std::string& json);

    const WeaponRecord* TryGetWeaponRecord(WeaponId id) const;
    bool HasWeaponRecord(WeaponId id) const;
    WeaponRecord GetWeaponRecord(WeaponId id) const;
    std::vector<WeaponRecord> GetAllWeaponRecords() const;

private:
    void AddWeaponRecord(const WeaponRecord& record);
};

}  // namespace osrssim
