#pragma once

#include "EncounterContext.h"

namespace osrssim::encounter
{

class ActiveEncounter
{
public:
    virtual ~ActiveEncounter() = default;

    virtual void Initialize(EncounterContext& context) = 0;
    virtual void BeforeEngineTick(EncounterContext& context);
    virtual void AfterEngineTick(EncounterContext& context);
    virtual bool IsComplete(const EncounterContext& context) const = 0;
};

}  // namespace osrssim::encounter
