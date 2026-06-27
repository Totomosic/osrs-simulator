#include "behavior/DefaultNpcBehavior.h"

#include "World.h"

namespace osrssim::behavior
{

bool DefaultNpcBehavior::CanBeShared() const
{
    return true;
}

void DefaultNpcBehavior::Update(
    NpcBehaviorContext& context,
    ActorId actorId)
{
    if (!context.TryHandleActorTargetCombat(actorId))
    {
        const bool moved =
            context.GetWorld().UpdateActorMovement(
                actorId,
                context.GetCurrentTick());

        if (moved)
        {
            context.TryHandleActorTargetCombat(actorId);
        }
    }
}

}  // namespace osrssim::behavior
