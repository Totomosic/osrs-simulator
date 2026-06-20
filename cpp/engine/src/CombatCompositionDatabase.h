#pragma once

#include "DpsService.h"
#include "Types.h"
#include "WeaponDatabase.h"

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

namespace osrssim
{

enum class CombatCompositionSource
{
    BuiltIn,
    Saved
};

struct CombatCompositionRecord
{
    CombatCompositionId id = 0;
    std::string name;
    CombatCompositionSource source = CombatCompositionSource::BuiltIn;
    CombatComposition composition;
};

class CombatCompositionDatabase
{
private:
    std::vector<CombatCompositionRecord> m_BuiltInRecords;
    std::vector<CombatCompositionRecord> m_SavedRecords;
    std::unordered_map<CombatCompositionId, std::size_t>
        m_BuiltInRecordIndexById;
    std::unordered_map<CombatCompositionId, std::size_t>
        m_SavedRecordIndexById;

public:
    static constexpr CombatCompositionId FirstSavedCombatCompositionId =
        9'000'000'000'000'000'000ULL;

    static CombatCompositionDatabase LoadFromJson(
        const std::string& json,
        const WeaponDatabase& weaponDatabase);

    void LoadSavedCombatCompositionRecordsFromJson(
        const std::string& json,
        const WeaponDatabase& weaponDatabase);
    std::string ExportSavedCombatCompositionRecordsToJson() const;
    CombatCompositionId CreateSavedCombatCompositionRecord(
        const std::string& name,
        const CombatComposition& composition);
    void UpdateSavedCombatCompositionRecord(
        CombatCompositionId id,
        const std::string& name,
        const CombatComposition& composition);
    bool DeleteSavedCombatCompositionRecord(CombatCompositionId id);

    const CombatCompositionRecord* TryGetCombatCompositionRecord(
        CombatCompositionId id) const;
    bool HasCombatCompositionRecord(CombatCompositionId id) const;
    CombatCompositionRecord GetCombatCompositionRecord(
        CombatCompositionId id) const;
    std::vector<CombatCompositionRecord> GetAllCombatCompositionRecords()
        const;
    std::vector<CombatCompositionRecord> GetCombatCompositionRecordsBySource(
        CombatCompositionSource source) const;

private:
    void AddBuiltInCombatCompositionRecord(
        const CombatCompositionRecord& record);
    void AddSavedCombatCompositionRecord(
        const CombatCompositionRecord& record);
};

}  // namespace osrssim
