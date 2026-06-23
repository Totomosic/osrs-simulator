#pragma once

#include "behavior/NpcBehavior.h"

namespace osrssim::behavior
{

class DefaultNpcBehavior : public NpcBehavior
{
public:
    bool CanBeShared() const override;
    void Update(NpcBehaviorContext& context, ActorId actorId) override;
};

}  // namespace osrssim::behavior
