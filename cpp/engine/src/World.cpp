#include "World.h"

namespace osrssim
{

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

    SceneCoordinate destination{
        membership->coordinate.x + dx,
        membership->coordinate.y + dy,
        membership->coordinate.plane};
    Pathing pathing(*scene);

    if (!pathing.CanMoveIgnoringActorOccupancy(
            membership->coordinate,
            destination,
            actor->speed,
            actor->size))
    {
        return false;
    }

    if (GetActorKind(actorId) == ActorKind::Npc &&
        HasFinalNpcOccupancyConflict(
            *scene,
            membership->coordinate,
            destination,
            actor->size))
    {
        return false;
    }

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
        coordinate);
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

bool World::UpdateActorMovement(ActorId actorId)
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

    if (movementTarget->value().kind == MovementTargetKind::SceneCoordinate)
    {
        destination = movementTarget->value().sceneCoordinate;

        if (scene == nullptr || !scene->Contains(destination) ||
            destination.plane != membership->coordinate.plane)
        {
            *movementTarget = std::nullopt;
            return false;
        }

        if (DoesActorFootprintCover(*actor, membership->coordinate, destination))
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
            *movementTarget = std::nullopt;
            return false;
        }

        destination = targetMembership->coordinate;
    }

    int dx = 0;
    int dy = 0;

    if (movementTarget->value().kind == MovementTargetKind::SceneCoordinate)
    {
        dx = GetMovementDeltaForAxis(
            membership->coordinate.x,
            actor->size,
            destination.x,
            actor->speed);
        dy = GetMovementDeltaForAxis(
            membership->coordinate.y,
            actor->size,
            destination.y,
            actor->speed);
    }
    else
    {
        const ActorCore* targetActor =
            TryGetActorCore(movementTarget->value().actorId);

        if (AreActorFootprintsDiagonallyAdjacent(
                *actor,
                membership->coordinate,
                *targetActor,
                destination))
        {
            const SceneCoordinate cardinalDestination =
                GetDiagonalAdjacencyCardinalDestination(
                    *actor,
                    membership->coordinate,
                    *targetActor,
                    destination);
            dx = cardinalDestination.x - membership->coordinate.x;
            dy = cardinalDestination.y - membership->coordinate.y;
        }
        else
        {
            dx = GetMovementDeltaTowardFootprintForAxis(
                membership->coordinate.x,
                actor->size,
                destination.x,
                targetActor->size,
                actor->speed);
            dy = GetMovementDeltaTowardFootprintForAxis(
                membership->coordinate.y,
                actor->size,
                destination.y,
                targetActor->size,
                actor->speed);
        }
    }

    const bool moved = MoveActorByDelta(actorId, dx, dy);

    if (moved)
    {
        const SceneMembership* updatedMembership = GetSceneMembership(actorId);

        if (updatedMembership != nullptr)
        {
            if (movementTarget->value().kind == MovementTargetKind::SceneCoordinate)
            {
                if (DoesActorFootprintCover(
                        *actor,
                        updatedMembership->coordinate,
                        destination))
                {
                    *movementTarget = std::nullopt;
                }
            }
            else
            {
                const ActorCore* targetActor =
                    TryGetActorCore(movementTarget->value().actorId);
                const SceneMembership* targetMembership =
                    GetSceneMembership(movementTarget->value().actorId);

                if (targetActor != nullptr && targetMembership != nullptr &&
                    AreActorFootprintsEdgeAdjacent(
                        *actor,
                        updatedMembership->coordinate,
                        *targetActor,
                        targetMembership->coordinate))
                {
                    *movementTarget = std::nullopt;
                }
            }
        }
    }

    return moved;
}

bool World::UpdatePlayerMovement(ActorId actorId)
{
    return TryGetPlayer(actorId) != nullptr && UpdateActorMovement(actorId);
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
    SceneCoordinate coordinate) const
{
    const Tile* tile = scene.TryGetTile(coordinate);
    return tile != nullptr && !IsWholeTileMovementBlocked(*tile);
}

bool World::DoesActorFootprintCover(
    const ActorCore& actor,
    SceneCoordinate actorCoordinate,
    SceneCoordinate target) const
{
    return target.plane == actorCoordinate.plane &&
           target.x >= actorCoordinate.x &&
           target.x < actorCoordinate.x + actor.size &&
           target.y >= actorCoordinate.y &&
           target.y < actorCoordinate.y + actor.size;
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

bool World::AreActorFootprintsDiagonallyAdjacent(
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

int World::GetMovementDeltaTowardFootprintForAxis(
    int anchor,
    int size,
    int targetAnchor,
    int targetSize,
    int speed) const
{
    const int max = anchor + size - 1;
    const int targetMax = targetAnchor + targetSize - 1;

    if (max + 1 < targetAnchor)
    {
        return ClampDelta(targetAnchor - max - 1, speed);
    }

    if (targetMax + 1 < anchor)
    {
        return ClampDelta(targetMax + 1 - anchor, speed);
    }

    return 0;
}

SceneCoordinate World::GetDiagonalAdjacencyCardinalDestination(
    const ActorCore& mover,
    SceneCoordinate moverCoordinate,
    const ActorCore& target,
    SceneCoordinate targetCoordinate) const
{
    const int moverMaxX = moverCoordinate.x + mover.size - 1;
    const int moverMaxY = moverCoordinate.y + mover.size - 1;
    const int targetMaxX = targetCoordinate.x + target.size - 1;
    const int targetMaxY = targetCoordinate.y + target.size - 1;
    const int westDx = -1;
    const int eastDx = 1;
    const int southDy = -1;
    const int northDy = 1;

    if (targetMaxX + 1 == moverCoordinate.x)
    {
        return {
            moverCoordinate.x + westDx,
            moverCoordinate.y,
            moverCoordinate.plane};
    }

    if (moverMaxX + 1 == targetCoordinate.x)
    {
        return {
            moverCoordinate.x + eastDx,
            moverCoordinate.y,
            moverCoordinate.plane};
    }

    if (targetMaxY + 1 == moverCoordinate.y)
    {
        return {
            moverCoordinate.x,
            moverCoordinate.y + southDy,
            moverCoordinate.plane};
    }

    if (moverMaxY + 1 == targetCoordinate.y)
    {
        return {
            moverCoordinate.x,
            moverCoordinate.y + northDy,
            moverCoordinate.plane};
    }

    return moverCoordinate;
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
