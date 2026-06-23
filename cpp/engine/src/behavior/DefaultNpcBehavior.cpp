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
        const bool startedFromOverlap =
            context.IsOverlappingActorMovementTarget(actorId);
        const bool moved =
            context.GetWorld().UpdateActorMovement(
                actorId,
                context.GetCurrentTick());

        if (moved && !startedFromOverlap)
        {
            context.TryHandleActorTargetCombat(actorId);
        }
    }
}

}  // namespace osrssim::behavior
