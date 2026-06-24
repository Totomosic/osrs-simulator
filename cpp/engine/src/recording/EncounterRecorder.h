#pragma once

#include "Engine.h"

#include <nlohmann/json.hpp>

#include <string>

namespace osrssim::recording
{

class EncounterRecorder
{
private:
    std::string m_EncounterName;
    double m_SecondsPerTick = 0.6;
    nlohmann::json m_Recording;

    static nlohmann::json CreateEmptyTick(int tick);

public:
    EncounterRecorder(std::string encounterName, double secondsPerTick);

    void RecordInitialState(const Engine& engine);
    void RecordCompletedTick(const Engine& engine);
    std::string ExportJson() const;
};

}  // namespace osrssim::recording
