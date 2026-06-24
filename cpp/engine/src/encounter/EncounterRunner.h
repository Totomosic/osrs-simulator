#pragma once

#include "Engine.h"
#include "encounter/ActiveEncounter.h"
#include "recording/EncounterRecorder.h"

#include <memory>

namespace osrssim::encounter
{

class EncounterRunner
{
private:
    Engine m_Engine;
    std::unique_ptr<ActiveEncounter> m_ActiveEncounter;
    recording::EncounterRecorder* m_Recorder = nullptr;
    bool m_Started = false;

public:
    explicit EncounterRunner(std::unique_ptr<ActiveEncounter> activeEncounter);

    bool AttachRecorder(recording::EncounterRecorder& recorder);
    void Start();
    bool Step();
    Engine& GetEngine();
    const Engine& GetEngine() const;
};

}  // namespace osrssim::encounter
