#include "World.h"

namespace osrssim
{
namespace
{
int Abs(int value)
{
    return value < 0 ? -value : value;
}

int Max(int lhs, int rhs)
{
    return lhs > rhs ? lhs : rhs;
}

int Sign(int value)
{
    if (value < 0)
    {
        return -1;
    }

    if (value > 0)
    {
        return 1;
    }

    return 0;
}

int CardinalPriority(int dx, int dy)
{
    if (dx < 0 && dy == 0)
    {
        return 0;
    }

    if (dx > 0 && dy == 0)
    {
        return 1;
    }

    if (dx == 0 && dy < 0)
    {
        return 2;
    }

    if (dx == 0 && dy > 0)
    {
        return 3;
    }

    return 4;
}

int DeterministicCardinalStart(
    ActorId actorId,
    ActorId targetActorId,
    Tick currentTick)
{
    return static_cast<int>(
        (actorId * 31 + targetActorId * 17 + currentTick) % 4);
}

int RemainingChebyshev(int requestedDx, int requestedDy, int dx, int dy)
{
    return Max(Abs(requestedDx - dx), Abs(requestedDy - dy));
}

int RemainingManhattan(int requestedDx, int requestedDy, int dx, int dy)
{
    return Abs(requestedDx - dx) + Abs(requestedDy - dy);
}

int RatioError(int requestedDx, int requestedDy, int dx, int dy)
{
    return Abs(Abs(dx) * Abs(requestedDy) - Abs(dy) * Abs(requestedDx));
}

bool IsBetterResolvedDelta(
    int requestedDx,
    int requestedDy,
    int candidateDx,
    int candidateDy,
    int bestDx,
    int bestDy)
{
    const int candidateRemaining =
        RemainingChebyshev(requestedDx, requestedDy, candidateDx, candidateDy);
    const int bestRemaining =
        RemainingChebyshev(requestedDx, requestedDy, bestDx, bestDy);

    if (candidateRemaining != bestRemaining)
    {
        return candidateRemaining < bestRemaining;
    }

    const bool requestUsesBothAxes = requestedDx != 0 && requestedDy != 0;
    const bool candidateUsesBothAxes = candidateDx != 0 && candidateDy != 0;
    const bool bestUsesBothAxes = bestDx != 0 && bestDy != 0;

    if (requestUsesBothAxes && candidateUsesBothAxes != bestUsesBothAxes)
    {
        return candidateUsesBothAxes;
    }

    if (requestUsesBothAxes && candidateUsesBothAxes && bestUsesBothAxes)
    {
        const int candidateRatioError =
            RatioError(requestedDx, requestedDy, candidateDx, candidateDy);
        const int bestRatioError =
            RatioError(requestedDx, requestedDy, bestDx, bestDy);

        if (candidateRatioError != bestRatioError)
        {
            return candidateRatioError < bestRatioError;
        }
    }

    const int candidateManhattan =
        RemainingManhattan(requestedDx, requestedDy, candidateDx, candidateDy);
    const int bestManhattan =
        RemainingManhattan(requestedDx, requestedDy, bestDx, bestDy);

    if (candidateManhattan != bestManhattan)
    {
        return candidateManhattan < bestManhattan;
    }

    const int candidateCardinalPriority =
        CardinalPriority(candidateDx, candidateDy);
    const int bestCardinalPriority = CardinalPriority(bestDx, bestDy);

    if (candidateCardinalPriority != bestCardinalPriority)
    {
        return candidateCardinalPriority < bestCardinalPriority;
    }

    return Abs(candidateDx) + Abs(candidateDy) > Abs(bestDx) + Abs(bestDy);
}
}  // namespace

SceneId World::GetDefaultSceneId() const
{
    return m_DefaultSceneId;
}

Scene* World::TryGetScene(SceneId sceneId)
{
    if (sceneId != m_DefaultSceneId)
    {
        return nullptr;
    }

    return &m_Scene;
}

const Scene* World::TryGetScene(SceneId sceneId) const
{
    if (sceneId != m_DefaultSceneId)
    {
        return nullptr;
    }

    return &m_Scene;
}

ActorId World::CreatePlayer(int size, int speed)
{
    const ActorId actorId = m_NextActorId++;
    m_Players.emplace(
        actorId,
        Player{{actorId, ClampSize(size), ClampSpeed(speed)}, std::nullopt});
    return actorId;
}

ActorId World::CreateNpc(int size, int speed)
{
    const ActorId actorId = m_NextActorId++;
    m_Npcs.emplace(
        actorId,
        Npc{{actorId, ClampSize(size), ClampSpeed(speed)}, std::nullopt});
    return actorId;
}

const std::unordered_map<ActorId, Player>& World::GetPlayers() const
{
    return m_Players;
}

const std::unordered_map<ActorId, Npc>& World::GetNpcs() const
{
    return m_Npcs;
}

const std::unordered_map<ActorId, SceneMembership>&
World::GetSceneMemberships() const
{
    return m_SceneMemberships;
}

const Player* World::GetPlayer(ActorId actorId) const
{
    const auto found = m_Players.find(actorId);
    return found == m_Players.end() ? nullptr : &found->second;
}

const Npc* World::GetNpc(ActorId actorId) const
{
    const auto found = m_Npcs.find(actorId);
    return found == m_Npcs.end() ? nullptr : &found->second;
}

const ActorCore* World::GetActorCore(ActorId actorId) const
{
    return TryGetActorCore(actorId);
}

const SceneMembership* World::GetSceneMembership(ActorId actorId) const
{
    const auto found = m_SceneMemberships.find(actorId);
    return found == m_SceneMemberships.end() ? nullptr : &found->second;
}

bool World::AreActorFootprintsOverlapping(
    ActorId firstActorId,
    ActorId secondActorId) const
{
    const ActorCore* firstActor = TryGetActorCore(firstActorId);
    const ActorCore* secondActor = TryGetActorCore(secondActorId);
    const SceneMembership* firstMembership = GetSceneMembership(firstActorId);
    const SceneMembership* secondMembership = GetSceneMembership(secondActorId);

    return firstActor != nullptr && secondActor != nullptr &&
           firstMembership != nullptr && secondMembership != nullptr &&
           firstMembership->sceneId == secondMembership->sceneId &&
           AreActorFootprintsOverlapping(
               *firstActor,
               firstMembership->coordinate,
               *secondActor,
               secondMembership->coordinate);
}

const WeaponDefinition* World::GetActorWeaponDefinition(ActorId actorId) const
{
    const ActorCore* actor = TryGetActorCore(actorId);
    return actor == nullptr ? nullptr : &actor->weapon;
}

bool World::SetActorWeaponDefinition(
    ActorId actorId,
    WeaponDefinition weaponDefinition)
{
    ActorCore* actor = TryGetActorCore(actorId);

    if (actor == nullptr)
    {
        return false;
    }

    actor->weapon = weaponDefinition;

    return true;
}

int World::GetActorAttackTimer(ActorId actorId) const
{
    const ActorCore* actor = TryGetActorCore(actorId);
    return actor == nullptr ? 0 : actor->attackTimer;
}

bool World::SetActorAttackTimer(ActorId actorId, int attackTimer)
{
    ActorCore* actor = TryGetActorCore(actorId);

    if (actor == nullptr)
    {
        return false;
    }

    actor->attackTimer = attackTimer;

    return true;
}

bool World::SetActorSpeed(ActorId actorId, int speed)
{
    ActorCore* actor = TryGetActorCore(actorId);

    if (actor == nullptr)
    {
        return false;
    }

    actor->speed = ClampSpeed(speed);

    return true;
}

bool World::PlaceActor(
    ActorId actorId,
    SceneId sceneId,
    SceneCoordinate coordinate)
{
    ActorCore* actor = TryGetActorCore(actorId);
    Scene* scene = TryGetScene(sceneId);

    if (actor == nullptr || scene == nullptr ||
        !CanStandOnMovementBlockers(*scene, coordinate, actor->size))
    {
        return false;
    }

    const SceneMembership* oldMembership = GetSceneMembership(actorId);

    if (oldMembership != nullptr)
    {
        Scene* oldScene = TryGetScene(oldMembership->sceneId);

        if (oldScene != nullptr)
        {
            RemoveActorOccupancy(*oldScene, oldMembership->coordinate, actor->size);
        }
    }

    m_SceneMemberships[actorId] = SceneMembership{sceneId, coordinate};
    AddActorOccupancy(*scene, coordinate, actor->size);

    return true;
}

bool World::RemoveActorSceneMembership(ActorId actorId)
{
    ActorCore* actor = TryGetActorCore(actorId);
    auto membership = m_SceneMemberships.find(actorId);

    if (actor == nullptr || membership == m_SceneMemberships.end())
    {
        return false;
    }

    Scene* scene = TryGetScene(membership->second.sceneId);

    if (scene != nullptr)
    {
        RemoveActorOccupancy(*scene, membership->second.coordinate, actor->size);
    }

    m_SceneMemberships.erase(membership);

    return true;
}

bool World::RemoveActor(ActorId actorId)
{
    if (!HasActor(actorId))
    {
        return false;
    }

    RemoveActorSceneMembership(actorId);
    m_Players.erase(actorId);
    m_Npcs.erase(actorId);

    return true;
}

bool World::MoveActorByDelta(ActorId actorId, int dx, int dy)
{
    ActorCore* actor = TryGetActorCore(actorId);
    SceneMembership* membership = nullptr;
    auto membershipIterator = m_SceneMemberships.find(actorId);

    if (membershipIterator != m_SceneMemberships.end())
    {
        membership = &membershipIterator->second;
    }

    if (actor == nullptr || membership == nullptr || (dx == 0 && dy == 0))
    {
        return false;
    }

    Scene* scene = TryGetScene(membership->sceneId);

    if (scene == nullptr)
    {
        return false;
    }

    int resolvedDx = 0;
    int resolvedDy = 0;
    const ActorKind actorKind = GetActorKind(actorId);

    if (!TryResolveMovementDelta(
            *scene,
            actorKind,
            *actor,
            membership->coordinate,
            dx,
            dy,
            resolvedDx,
            resolvedDy))
    {
        return false;
    }

    SceneCoordinate destination{
        membership->coordinate.x + resolvedDx,
        membership->coordinate.y + resolvedDy,
        membership->coordinate.plane};

    RemoveActorOccupancy(*scene, membership->coordinate, actor->size);
    membership->coordinate = destination;
    AddActorOccupancy(*scene, membership->coordinate, actor->size);

    return true;
}

bool World::CanActorUseSceneCoordinateMovementTarget(
    ActorId actorId,
    SceneCoordinate coordinate) const
{
    const ActorCore* actor = TryGetActorCore(actorId);
    const SceneMembership* membership = GetSceneMembership(actorId);

    if (actor == nullptr || membership == nullptr)
    {
        return false;
    }

    const Scene* scene = TryGetScene(membership->sceneId);

    return scene != nullptr && CanUseSceneCoordinateMovementTarget(
        *scene,
        coordinate,
        actor->size);
}

bool World::CanPlayerUseSceneCoordinateMovementTarget(
    ActorId actorId,
    SceneCoordinate coordinate) const
{
    return TryGetPlayer(actorId) != nullptr &&
           CanActorUseSceneCoordinateMovementTarget(actorId, coordinate);
}

bool World::SetActorSceneCoordinateMovementTarget(
    ActorId actorId,
    SceneCoordinate coordinate)
{
    std::optional<MovementTarget>* movementTarget =
        TryGetMovementTarget(actorId);

    if (movementTarget == nullptr)
    {
        return false;
    }

    *movementTarget = MovementTarget{
        MovementTargetKind::SceneCoordinate,
        coordinate,
        0};

    return true;
}

bool World::SetPlayerSceneCoordinateMovementTarget(
    ActorId actorId,
    SceneCoordinate coordinate)
{
    return TryGetPlayer(actorId) != nullptr &&
           SetActorSceneCoordinateMovementTarget(actorId, coordinate);
}

bool World::ClearActorMovementTarget(ActorId actorId)
{
    std::optional<MovementTarget>* movementTarget =
        TryGetMovementTarget(actorId);

    if (movementTarget == nullptr)
    {
        return false;
    }

    *movementTarget = std::nullopt;

    return true;
}

bool World::SetActorMovementTarget(ActorId actorId, ActorId targetActorId)
{
    std::optional<MovementTarget>* movementTarget =
        TryGetMovementTarget(actorId);

    if (movementTarget == nullptr || actorId == targetActorId ||
        TryGetActorCore(targetActorId) == nullptr)
    {
        return false;
    }

    *movementTarget = MovementTarget{
        MovementTargetKind::Actor,
        {},
        targetActorId};

    return true;
}

bool World::UpdateActorMovement(ActorId actorId, Tick currentTick)
{
    ActorCore* actor = TryGetActorCore(actorId);
    std::optional<MovementTarget>* movementTarget =
        TryGetMovementTarget(actorId);
    SceneMembership* membership = nullptr;
    auto membershipIterator = m_SceneMemberships.find(actorId);

    if (membershipIterator != m_SceneMemberships.end())
    {
        membership = &membershipIterator->second;
    }

    if (actor == nullptr || movementTarget == nullptr ||
        !movementTarget->has_value())
    {
        return false;
    }

    if (membership == nullptr)
    {
        *movementTarget = std::nullopt;
        return false;
    }

    const Scene* scene = TryGetScene(membership->sceneId);
    SceneCoordinate destination;
    bool actorTargetOverlaps = false;

    if (movementTarget->value().kind == MovementTargetKind::SceneCoordinate)
    {
        destination = movementTarget->value().sceneCoordinate;

        if (scene == nullptr || !scene->Contains(destination) ||
            destination.plane != membership->coordinate.plane)
        {
            *movementTarget = std::nullopt;
            return false;
        }

        if (IsActorAtSceneCoordinateMovementTarget(
                membership->coordinate,
                destination))
        {
            *movementTarget = std::nullopt;
            return false;
        }
    }
    else
    {
        const ActorCore* targetActor =
            TryGetActorCore(movementTarget->value().actorId);
        const SceneMembership* targetMembership =
            GetSceneMembership(movementTarget->value().actorId);

        if (scene == nullptr || targetActor == nullptr ||
            targetMembership == nullptr ||
            targetMembership->sceneId != membership->sceneId ||
            targetMembership->coordinate.plane != membership->coordinate.plane)
        {
            *movementTarget = std::nullopt;
            return false;
        }

        if (AreActorFootprintsEdgeAdjacent(
                *actor,
                membership->coordinate,
                *targetActor,
                targetMembership->coordinate))
        {
            return false;
        }

        if (AreActorFootprintsOverlapping(
                *actor,
                membership->coordinate,
                *targetActor,
                targetMembership->coordinate))
        {
            actorTargetOverlaps = true;
        }

        destination = targetMembership->coordinate;
    }

    int dx = 0;
    int dy = 0;

    if (movementTarget->value().kind == MovementTargetKind::SceneCoordinate)
    {
        dx = GetMovementDeltaForAxis(
            membership->coordinate.x,
            1,
            destination.x,
            actor->speed);
        dy = GetMovementDeltaForAxis(
            membership->coordinate.y,
            1,
            destination.y,
            actor->speed);
    }
    else
    {
        if (actorTargetOverlaps)
        {
            if (scene == nullptr ||
                !TryGetActorTargetOverlapEscapeMovementDelta(
                    *scene,
                    GetActorKind(actorId),
                    *actor,
                    membership->coordinate,
                    movementTarget->value().actorId,
                    currentTick,
                    dx,
                    dy))
            {
                return false;
            }
        }
        else
        {
            dx = GetMovementDeltaForAxis(
                membership->coordinate.x,
                1,
                destination.x,
                actor->speed);
            dy = GetMovementDeltaForAxis(
                membership->coordinate.y,
                1,
                destination.y,
                actor->speed);

            int edgeAdjacentDx = 0;
            int edgeAdjacentDy = 0;
            const ActorCore* targetActor =
                TryGetActorCore(movementTarget->value().actorId);
            const SceneMembership* targetMembership =
                GetSceneMembership(movementTarget->value().actorId);
            const bool cornerContact =
                targetActor != nullptr && targetMembership != nullptr &&
                AreActorFootprintsCornerContact(
                    *actor,
                    membership->coordinate,
                    *targetActor,
                    targetMembership->coordinate);

            if (scene != nullptr && targetActor != nullptr &&
                targetMembership != nullptr &&
                TryGetActorTargetEdgeAdjacentMovementDelta(
                    *scene,
                    GetActorKind(actorId),
                    *actor,
                    membership->coordinate,
                    dx,
                    dy,
                    *targetActor,
                    targetMembership->coordinate,
                    edgeAdjacentDx,
                    edgeAdjacentDy))
            {
                dx = edgeAdjacentDx;
                dy = edgeAdjacentDy;
            }
            else if (cornerContact)
            {
                return false;
            }
        }
    }

    const bool moved = MoveActorByDelta(actorId, dx, dy);

    if (!moved)
    {
        const SceneCoordinate attemptedDestination{
            membership->coordinate.x + dx,
            membership->coordinate.y + dy,
            membership->coordinate.plane};
        const bool shouldKeepDynamicSceneCoordinateTarget =
            movementTarget->value().kind == MovementTargetKind::SceneCoordinate &&
            GetActorKind(actorId) == ActorKind::Npc && scene != nullptr &&
            IsFinalNpcOccupancyOnlyBlock(
                *scene,
                *actor,
                membership->coordinate,
                attemptedDestination);

        if (movementTarget->value().kind == MovementTargetKind::SceneCoordinate &&
            !shouldKeepDynamicSceneCoordinateTarget)
        {
            *movementTarget = std::nullopt;
        }

        return false;
    }

    const SceneMembership* updatedMembership = GetSceneMembership(actorId);

    if (updatedMembership != nullptr)
    {
        if (movementTarget->value().kind == MovementTargetKind::SceneCoordinate)
        {
            if (IsActorAtSceneCoordinateMovementTarget(
                    updatedMembership->coordinate,
                    destination))
            {
                *movementTarget = std::nullopt;
            }
        }
    }

    return moved;
}

bool World::UpdatePlayerMovement(ActorId actorId, Tick currentTick)
{
    return TryGetPlayer(actorId) != nullptr &&
           UpdateActorMovement(actorId, currentTick);
}

int World::ClampSize(int size)
{
    return size < 1 ? 1 : size;
}

int World::ClampSpeed(int speed)
{
    return speed < 0 ? 0 : speed;
}

int World::ClampDelta(int delta, int speed)
{
    if (delta > speed)
    {
        return speed;
    }

    if (delta < -speed)
    {
        return -speed;
    }

    return delta;
}

bool World::IsWholeTileMovementBlocked(const Tile& tile)
{
    return tile.HasFlag(TileFlag::BlockMovementFull) ||
           tile.HasFlag(TileFlag::BlockMovementObject) ||
           tile.HasFlag(TileFlag::BlockMovementFloor) ||
           tile.HasFlag(TileFlag::BlockMovementFloorDecoration);
}

bool World::CanUseLargeNpcDiagonalSqueeze(
    ActorKind actorKind,
    const ActorCore& actor)
{
    return actorKind == ActorKind::Npc && actor.size > 1;
}

DiagonalSideFootprintRule World::GetDiagonalSideFootprintRule(
    ActorKind actorKind,
    const ActorCore& actor)
{
    return CanUseLargeNpcDiagonalSqueeze(actorKind, actor)
               ? DiagonalSideFootprintRule::AllowBlocked
               : DiagonalSideFootprintRule::RequireClear;
}

Player* World::TryGetPlayer(ActorId actorId)
{
    auto player = m_Players.find(actorId);
    return player == m_Players.end() ? nullptr : &player->second;
}

const Player* World::TryGetPlayer(ActorId actorId) const
{
    const auto player = m_Players.find(actorId);
    return player == m_Players.end() ? nullptr : &player->second;
}

ActorCore* World::TryGetActorCore(ActorId actorId)
{
    auto player = m_Players.find(actorId);

    if (player != m_Players.end())
    {
        return &player->second.actor;
    }

    auto npc = m_Npcs.find(actorId);

    if (npc != m_Npcs.end())
    {
        return &npc->second.actor;
    }

    return nullptr;
}

const ActorCore* World::TryGetActorCore(ActorId actorId) const
{
    const auto player = m_Players.find(actorId);

    if (player != m_Players.end())
    {
        return &player->second.actor;
    }

    const auto npc = m_Npcs.find(actorId);

    if (npc != m_Npcs.end())
    {
        return &npc->second.actor;
    }

    return nullptr;
}

std::optional<MovementTarget>* World::TryGetMovementTarget(ActorId actorId)
{
    auto player = m_Players.find(actorId);

    if (player != m_Players.end())
    {
        return &player->second.movementTarget;
    }

    auto npc = m_Npcs.find(actorId);

    if (npc != m_Npcs.end())
    {
        return &npc->second.movementTarget;
    }

    return nullptr;
}

const std::optional<MovementTarget>* World::TryGetMovementTarget(
    ActorId actorId) const
{
    const auto player = m_Players.find(actorId);

    if (player != m_Players.end())
    {
        return &player->second.movementTarget;
    }

    const auto npc = m_Npcs.find(actorId);

    if (npc != m_Npcs.end())
    {
        return &npc->second.movementTarget;
    }

    return nullptr;
}

World::ActorKind World::GetActorKind(ActorId actorId) const
{
    return m_Players.contains(actorId) ? ActorKind::Player : ActorKind::Npc;
}

bool World::HasActor(ActorId actorId) const
{
    return m_Players.contains(actorId) || m_Npcs.contains(actorId);
}

bool World::CanUseSceneCoordinateMovementTarget(
    const Scene& scene,
    SceneCoordinate coordinate,
    int actorSize) const
{
    return CanStandOnMovementBlockers(scene, coordinate, actorSize);
}

bool World::IsActorAtSceneCoordinateMovementTarget(
    SceneCoordinate actorCoordinate,
    SceneCoordinate target) const
{
    return target.plane == actorCoordinate.plane &&
           target.x == actorCoordinate.x &&
           target.y == actorCoordinate.y;
}

bool World::AreActorFootprintsEdgeAdjacent(
    const ActorCore& mover,
    SceneCoordinate moverCoordinate,
    const ActorCore& target,
    SceneCoordinate targetCoordinate) const
{
    if (moverCoordinate.plane != targetCoordinate.plane)
    {
        return false;
    }

    const int moverMinX = moverCoordinate.x;
    const int moverMaxX = moverCoordinate.x + mover.size - 1;
    const int moverMinY = moverCoordinate.y;
    const int moverMaxY = moverCoordinate.y + mover.size - 1;
    const int targetMinX = targetCoordinate.x;
    const int targetMaxX = targetCoordinate.x + target.size - 1;
    const int targetMinY = targetCoordinate.y;
    const int targetMaxY = targetCoordinate.y + target.size - 1;

    const bool overlapsOnX =
        moverMinX <= targetMaxX && targetMinX <= moverMaxX;
    const bool overlapsOnY =
        moverMinY <= targetMaxY && targetMinY <= moverMaxY;
    const bool touchesWestOrEast =
        moverMaxX + 1 == targetMinX || targetMaxX + 1 == moverMinX;
    const bool touchesSouthOrNorth =
        moverMaxY + 1 == targetMinY || targetMaxY + 1 == moverMinY;

    return (touchesWestOrEast && overlapsOnY) ||
           (touchesSouthOrNorth && overlapsOnX);
}

bool World::AreActorFootprintsOverlapping(
    const ActorCore& mover,
    SceneCoordinate moverCoordinate,
    const ActorCore& target,
    SceneCoordinate targetCoordinate) const
{
    if (moverCoordinate.plane != targetCoordinate.plane)
    {
        return false;
    }

    const int moverMinX = moverCoordinate.x;
    const int moverMaxX = moverCoordinate.x + mover.size - 1;
    const int moverMinY = moverCoordinate.y;
    const int moverMaxY = moverCoordinate.y + mover.size - 1;
    const int targetMinX = targetCoordinate.x;
    const int targetMaxX = targetCoordinate.x + target.size - 1;
    const int targetMinY = targetCoordinate.y;
    const int targetMaxY = targetCoordinate.y + target.size - 1;

    return moverMinX <= targetMaxX && targetMinX <= moverMaxX &&
           moverMinY <= targetMaxY && targetMinY <= moverMaxY;
}

bool World::AreActorFootprintsCornerContact(
    const ActorCore& mover,
    SceneCoordinate moverCoordinate,
    const ActorCore& target,
    SceneCoordinate targetCoordinate) const
{
    if (moverCoordinate.plane != targetCoordinate.plane)
    {
        return false;
    }

    const int moverMinX = moverCoordinate.x;
    const int moverMaxX = moverCoordinate.x + mover.size - 1;
    const int moverMinY = moverCoordinate.y;
    const int moverMaxY = moverCoordinate.y + mover.size - 1;
    const int targetMinX = targetCoordinate.x;
    const int targetMaxX = targetCoordinate.x + target.size - 1;
    const int targetMinY = targetCoordinate.y;
    const int targetMaxY = targetCoordinate.y + target.size - 1;

    const bool touchesWestOrEast =
        moverMaxX + 1 == targetMinX || targetMaxX + 1 == moverMinX;
    const bool touchesSouthOrNorth =
        moverMaxY + 1 == targetMinY || targetMaxY + 1 == moverMinY;

    return touchesWestOrEast && touchesSouthOrNorth;
}

int World::GetMovementDeltaForAxis(
    int anchor,
    int size,
    int target,
    int speed) const
{
    if (target < anchor)
    {
        return ClampDelta(target - anchor, speed);
    }

    if (target >= anchor + size)
    {
        return ClampDelta(target - (anchor + size - 1), speed);
    }

    return 0;
}

bool World::HasNpcDiagonalSideOccupancyConflict(
    const Scene& scene,
    SceneCoordinate current,
    SceneCoordinate destination,
    int actorSize) const
{
    const int dx = Sign(destination.x - current.x);
    const int dy = Sign(destination.y - current.y);

    if (dx == 0 || dy == 0)
    {
        return false;
    }

    SceneCoordinate horizontal{current.x + dx, current.y, current.plane};
    SceneCoordinate vertical{current.x, current.y + dy, current.plane};

    return HasFinalNpcOccupancyConflict(scene, current, horizontal, actorSize) ||
           HasFinalNpcOccupancyConflict(scene, current, vertical, actorSize);
}

bool World::TryGetActorTargetEdgeAdjacentMovementDelta(
    const Scene& scene,
    ActorKind actorKind,
    const ActorCore& actor,
    SceneCoordinate current,
    int requestedDx,
    int requestedDy,
    const ActorCore& target,
    SceneCoordinate targetCoordinate,
    int& edgeAdjacentDx,
    int& edgeAdjacentDy) const
{
    const int stepX = Sign(requestedDx);
    const int stepY = Sign(requestedDy);
    const int absRequestedDx = Abs(requestedDx);
    const int absRequestedDy = Abs(requestedDy);
    Pathing pathing(scene);
    const DiagonalSideFootprintRule diagonalSideFootprintRule =
        GetDiagonalSideFootprintRule(actorKind, actor);
    const bool canUseLargeNpcDiagonalSqueeze =
        CanUseLargeNpcDiagonalSqueeze(actorKind, actor);
    const bool cornerContact = AreActorFootprintsCornerContact(
        actor,
        current,
        target,
        targetCoordinate);
    bool found = false;
    int bestDx = 0;
    int bestDy = 0;

    for (int candidateAbsDx = 0; candidateAbsDx <= absRequestedDx;
         ++candidateAbsDx)
    {
        for (int candidateAbsDy = 0; candidateAbsDy <= absRequestedDy;
             ++candidateAbsDy)
        {
            const int candidateDx = candidateAbsDx * stepX;
            const int candidateDy = candidateAbsDy * stepY;

            if (candidateDx == 0 && candidateDy == 0)
            {
                continue;
            }

            if (cornerContact && candidateDy != 0)
            {
                continue;
            }

            SceneCoordinate candidate{
                current.x + candidateDx,
                current.y + candidateDy,
                current.plane};

            if (!AreActorFootprintsEdgeAdjacent(
                    actor,
                    candidate,
                    target,
                    targetCoordinate))
            {
                continue;
            }

            if (!pathing.CanMoveIgnoringActorOccupancy(
                    current,
                    candidate,
                    actor.speed,
                    actor.size,
                    diagonalSideFootprintRule))
            {
                continue;
            }

            if (actorKind == ActorKind::Npc &&
                !canUseLargeNpcDiagonalSqueeze &&
                HasNpcDiagonalSideOccupancyConflict(
                    scene,
                    current,
                    candidate,
                    actor.size))
            {
                continue;
            }

            if (actorKind == ActorKind::Npc &&
                HasFinalNpcOccupancyConflict(
                    scene,
                    current,
                    candidate,
                    actor.size))
            {
                continue;
            }

            const int candidateDistance = Max(Abs(candidateDx), Abs(candidateDy));
            const int bestDistance = Max(Abs(bestDx), Abs(bestDy));
            if (!found || candidateDistance < bestDistance ||
                (candidateDistance == bestDistance &&
                 IsBetterResolvedDelta(
                     requestedDx,
                     requestedDy,
                     candidateDx,
                     candidateDy,
                     bestDx,
                     bestDy)))
            {
                found = true;
                bestDx = candidateDx;
                bestDy = candidateDy;
            }
        }
    }

    if (!found)
    {
        return false;
    }

    edgeAdjacentDx = bestDx;
    edgeAdjacentDy = bestDy;

    return true;
}

bool World::TryGetActorTargetOverlapEscapeMovementDelta(
    const Scene& scene,
    ActorKind actorKind,
    const ActorCore& actor,
    SceneCoordinate current,
    ActorId targetActorId,
    Tick currentTick,
    int& escapeDx,
    int& escapeDy) const
{
    if (actorKind != ActorKind::Npc || actor.speed <= 0)
    {
        return false;
    }

    struct MovementDelta
    {
        int dx = 0;
        int dy = 0;
    };

    const MovementDelta cardinalDeltas[4]{
        {-1, 0},
        {1, 0},
        {0, -1},
        {0, 1}};
    const int start =
        DeterministicCardinalStart(actor.id, targetActorId, currentTick);
    Pathing pathing(scene);

    for (int offset = 0; offset < 4; ++offset)
    {
        const MovementDelta delta = cardinalDeltas[(start + offset) % 4];
        SceneCoordinate candidate{
            current.x + delta.dx,
            current.y + delta.dy,
            current.plane};

        if (!pathing.CanMoveIgnoringActorOccupancy(
                current,
                candidate,
                1,
                actor.size))
        {
            continue;
        }

        if (HasFinalNpcOccupancyConflict(
                scene,
                current,
                candidate,
                actor.size))
        {
            continue;
        }

        escapeDx = delta.dx;
        escapeDy = delta.dy;

        return true;
    }

    return false;
}

bool World::CanStandOnMovementBlockers(
    const Scene& scene,
    SceneCoordinate coordinate,
    int actorSize) const
{
    for (int offsetX = 0; offsetX < actorSize; ++offsetX)
    {
        for (int offsetY = 0; offsetY < actorSize; ++offsetY)
        {
            const Tile* tile = scene.TryGetTile(
                {coordinate.x + offsetX,
                 coordinate.y + offsetY,
                 coordinate.plane});

            if (tile == nullptr || IsWholeTileMovementBlocked(*tile))
            {
                return false;
            }
        }
    }

    return true;
}

bool World::HasFinalNpcOccupancyConflict(
    const Scene& scene,
    SceneCoordinate current,
    SceneCoordinate destination,
    int actorSize) const
{
    for (int offsetX = 0; offsetX < actorSize; ++offsetX)
    {
        for (int offsetY = 0; offsetY < actorSize; ++offsetY)
        {
            SceneCoordinate coordinate{
                destination.x + offsetX,
                destination.y + offsetY,
                destination.plane};
            const Tile* tile = scene.TryGetTile(coordinate);

            if (tile == nullptr)
            {
                return true;
            }

            const bool isCurrentFootprint =
                coordinate.plane == current.plane &&
                coordinate.x >= current.x &&
                coordinate.x < current.x + actorSize &&
                coordinate.y >= current.y &&
                coordinate.y < current.y + actorSize;

            if (!isCurrentFootprint && tile->IsOccupied())
            {
                return true;
            }
        }
    }

    return false;
}

bool World::IsFinalNpcOccupancyOnlyBlock(
    const Scene& scene,
    const ActorCore& actor,
    SceneCoordinate current,
    SceneCoordinate destination) const
{
    Pathing pathing(scene);
    const DiagonalSideFootprintRule diagonalSideFootprintRule =
        GetDiagonalSideFootprintRule(ActorKind::Npc, actor);

    return pathing.CanMoveIgnoringActorOccupancy(
               current,
               destination,
               actor.speed,
               actor.size,
               diagonalSideFootprintRule) &&
           HasFinalNpcOccupancyConflict(
               scene,
               current,
               destination,
               actor.size);
}

bool World::TryResolveMovementDelta(
    const Scene& scene,
    ActorKind actorKind,
    const ActorCore& actor,
    SceneCoordinate current,
    int requestedDx,
    int requestedDy,
    int& resolvedDx,
    int& resolvedDy) const
{
    if ((requestedDx == 0 && requestedDy == 0) ||
        Abs(requestedDx) > actor.speed || Abs(requestedDy) > actor.speed)
    {
        return false;
    }

    const int stepX = Sign(requestedDx);
    const int stepY = Sign(requestedDy);
    const int absRequestedDx = Abs(requestedDx);
    const int absRequestedDy = Abs(requestedDy);
    Pathing pathing(scene);
    const DiagonalSideFootprintRule diagonalSideFootprintRule =
        GetDiagonalSideFootprintRule(actorKind, actor);
    const bool canUseLargeNpcDiagonalSqueeze =
        CanUseLargeNpcDiagonalSqueeze(actorKind, actor);
    bool found = false;
    int bestDx = 0;
    int bestDy = 0;

    for (int candidateAbsDx = 0; candidateAbsDx <= absRequestedDx;
         ++candidateAbsDx)
    {
        for (int candidateAbsDy = 0; candidateAbsDy <= absRequestedDy;
             ++candidateAbsDy)
        {
            const int candidateDx = candidateAbsDx * stepX;
            const int candidateDy = candidateAbsDy * stepY;

            if (candidateDx == 0 && candidateDy == 0)
            {
                continue;
            }

            SceneCoordinate candidate{
                current.x + candidateDx,
                current.y + candidateDy,
                current.plane};

            if (!pathing.CanMoveIgnoringActorOccupancy(
                    current,
                    candidate,
                    actor.speed,
                    actor.size,
                    diagonalSideFootprintRule))
            {
                continue;
            }

            if (actorKind == ActorKind::Npc &&
                !canUseLargeNpcDiagonalSqueeze &&
                HasNpcDiagonalSideOccupancyConflict(
                    scene,
                    current,
                    candidate,
                    actor.size))
            {
                continue;
            }

            if (actorKind == ActorKind::Npc &&
                HasFinalNpcOccupancyConflict(
                    scene,
                    current,
                    candidate,
                    actor.size))
            {
                continue;
            }

            if (!found || IsBetterResolvedDelta(
                              requestedDx,
                              requestedDy,
                              candidateDx,
                              candidateDy,
                              bestDx,
                              bestDy))
            {
                found = true;
                bestDx = candidateDx;
                bestDy = candidateDy;
            }
        }
    }

    if (!found)
    {
        return false;
    }

    resolvedDx = bestDx;
    resolvedDy = bestDy;

    return true;
}

void World::AddActorOccupancy(
    Scene& scene,
    SceneCoordinate coordinate,
    int actorSize)
{
    for (int offsetX = 0; offsetX < actorSize; ++offsetX)
    {
        for (int offsetY = 0; offsetY < actorSize; ++offsetY)
        {
            Tile* tile = scene.TryGetTile(
                {coordinate.x + offsetX,
                 coordinate.y + offsetY,
                 coordinate.plane});

            if (tile != nullptr)
            {
                tile->AddFlag(TileFlag::Occupied);
            }
        }
    }
}

void World::RemoveActorOccupancy(
    Scene& scene,
    SceneCoordinate coordinate,
    int actorSize)
{
    for (int offsetX = 0; offsetX < actorSize; ++offsetX)
    {
        for (int offsetY = 0; offsetY < actorSize; ++offsetY)
        {
            Tile* tile = scene.TryGetTile(
                {coordinate.x + offsetX,
                 coordinate.y + offsetY,
                 coordinate.plane});

            if (tile != nullptr)
            {
                tile->RemoveFlag(TileFlag::Occupied);
            }
        }
    }
}

}  // namespace osrssim
