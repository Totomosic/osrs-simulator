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
    m_Npcs.emplace(actorId, Npc{{actorId, ClampSize(size), ClampSpeed(speed)}});
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

bool World::CanPlayerUseSceneCoordinateMovementTarget(
    ActorId actorId,
    SceneCoordinate coordinate) const
{
    const Player* player = TryGetPlayer(actorId);
    const SceneMembership* membership = GetSceneMembership(actorId);

    if (player == nullptr || membership == nullptr)
    {
        return false;
    }

    const Scene* scene = TryGetScene(membership->sceneId);

    return scene != nullptr && CanUseSceneCoordinateMovementTarget(
        *scene,
        coordinate);
}

bool World::SetPlayerSceneCoordinateMovementTarget(
    ActorId actorId,
    SceneCoordinate coordinate)
{
    Player* player = TryGetPlayer(actorId);

    if (player == nullptr ||
        !CanPlayerUseSceneCoordinateMovementTarget(actorId, coordinate))
    {
        return false;
    }

    player->movementTarget = coordinate;

    return true;
}

bool World::UpdatePlayerMovement(ActorId actorId)
{
    Player* player = TryGetPlayer(actorId);
    SceneMembership* membership = nullptr;
    auto membershipIterator = m_SceneMemberships.find(actorId);

    if (membershipIterator != m_SceneMemberships.end())
    {
        membership = &membershipIterator->second;
    }

    if (player == nullptr || membership == nullptr ||
        !player->movementTarget.has_value())
    {
        return false;
    }

    if (DoesActorFootprintCover(
            player->actor,
            membership->coordinate,
            *player->movementTarget))
    {
        player->movementTarget = std::nullopt;
        return false;
    }

    const int dx = GetMovementDeltaForAxis(
        membership->coordinate.x,
        player->actor.size,
        player->movementTarget->x,
        player->actor.speed);
    const int dy = GetMovementDeltaForAxis(
        membership->coordinate.y,
        player->actor.size,
        player->movementTarget->y,
        player->actor.speed);

    const bool moved = MoveActorByDelta(actorId, dx, dy);

    if (moved)
    {
        const SceneMembership* updatedMembership = GetSceneMembership(actorId);

        if (updatedMembership != nullptr &&
            DoesActorFootprintCover(
                player->actor,
                updatedMembership->coordinate,
                *player->movementTarget))
        {
            player->movementTarget = std::nullopt;
        }
    }

    return moved;
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
