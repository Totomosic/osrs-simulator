#include "Types.h"

namespace osrssim
{
namespace
{
static_assert(sizeof(Tick) == 8);
static_assert(sizeof(EntityId) == 8);
static_assert(sizeof(ActorId) == 8);
static_assert(sizeof(SceneId) == 8);
static_assert(sizeof(Seed) == 8);
}  // namespace
}  // namespace osrssim
