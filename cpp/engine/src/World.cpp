#include "World.h"

#include "ActorMovement.h"

#include <algorithm>
#include <utility>

namespace osrssim
{
namespace
{
constexpr int MaxLiveActorIndex = 65535;

}  // namespace

void World::SetCurrentTick(Tick currentTick)
{
    m_CurrentTick = currentTick;
}

std::optional<Tick> World::GetCurrentTick() const
{
    return m_CurrentTick;
}

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

std::optional<ActorId> World::CreatePlayer(
    int size,
    int speed,
    CombatComposition combatComposition)
{
    std::optional<PlayerIndex> playerIndex = AllocatePlayerIndex();

    if (!playerIndex.has_value())
    {
        return std::nullopt;
    }

    const ActorId actorId = m_NextActorId++;
    m_Players.emplace(
        actorId,
        Player{
            {actorId, ClampSize(size), ClampSpeed(speed), combatComposition},
            playerIndex.value(),
            std::nullopt});
    m_PlayerActorsByIndex.emplace(playerIndex.value(), actorId);
    return actorId;
}

std::optional<ActorId> World::CreateNpc(
    int size,
    int speed,
    CombatComposition combatComposition)
{
    std::optional<NpcIndex> npcIndex = AllocateNpcIndex();

    if (!npcIndex.has_value())
    {
        return std::nullopt;
    }

    const ActorId actorId = m_NextActorId++;
    m_Npcs.emplace(
        actorId,
        Npc{
            {actorId, ClampSize(size), ClampSpeed(speed), combatComposition},
            npcIndex.value(),
            0,
            std::nullopt});
    m_NpcActorsByIndex.emplace(npcIndex.value(), actorId);
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

std::optional<ActorId> World::GetNextPlayerActorIdAfterIndex(
    int playerIndex) const
{
    const auto nextIndex =
        playerIndex < 0
            ? m_LivePlayerIndices.begin()
            : m_LivePlayerIndices.upper_bound(
                  static_cast<PlayerIndex>(playerIndex));

    if (nextIndex == m_LivePlayerIndices.end())
    {
        return std::nullopt;
    }

    const auto actor = m_PlayerActorsByIndex.find(*nextIndex);
    return actor == m_PlayerActorsByIndex.end()
        ? std::nullopt
        : std::optional<ActorId>(actor->second);
}

std::optional<ActorId> World::GetNextNpcActorIdAfterIndex(int npcIndex) const
{
    const auto nextIndex =
        npcIndex < 0
            ? m_LiveNpcIndices.begin()
            : m_LiveNpcIndices.upper_bound(static_cast<NpcIndex>(npcIndex));

    if (nextIndex == m_LiveNpcIndices.end())
    {
        return std::nullopt;
    }

    const auto actor = m_NpcActorsByIndex.find(*nextIndex);
    return actor == m_NpcActorsByIndex.end()
        ? std::nullopt
        : std::optional<ActorId>(actor->second);
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

const CombatComposition* World::GetActorCombatComposition(
    ActorId actorId) const
{
    const ActorCore* actor = TryGetActorCore(actorId);
    return actor == nullptr ? nullptr : &actor->combatComposition;
}

CombatQueue* World::GetActorCombatQueue(ActorId actorId)
{
    ActorCore* actor = TryGetActorCore(actorId);
    return actor == nullptr ? nullptr : &actor->combatQueue;
}

const CombatQueue* World::GetActorCombatQueue(ActorId actorId) const
{
    const ActorCore* actor = TryGetActorCore(actorId);
    return actor == nullptr ? nullptr : &actor->combatQueue;
}

bool World::QueueActorCombatEvent(
    ActorId actorId,
    int ticksRemaining,
    std::function<void()> callback)
{
    CombatQueue* queue = GetActorCombatQueue(actorId);

    if (queue == nullptr)
    {
        return false;
    }

    return queue->AddEvent(ticksRemaining, std::move(callback), m_CurrentTick);
}

bool World::QueueActorCombatEvent(
    ActorId actorId,
    int ticksRemaining,
    std::function<void()> callback,
    ProjectileMetadata projectile)
{
    CombatQueue* queue = GetActorCombatQueue(actorId);

    if (queue == nullptr)
    {
        return false;
    }

    return queue->AddEvent(
        ticksRemaining,
        std::move(callback),
        projectile,
        m_CurrentTick);
}

std::vector<ProjectileSnapshot> World::GetProjectileSnapshots() const
{
    std::vector<ProjectileSnapshot> snapshots;

    auto appendActorProjectiles = [this, &snapshots](const ActorCore& actor)
    {
        std::vector<ProjectileSnapshot> actorProjectiles =
            actor.combatQueue.GetProjectileSnapshots();

        for (ProjectileSnapshot& projectile : actorProjectiles)
        {
            if (GetActorCore(projectile.targetActorId) != nullptr &&
                GetSceneMembership(projectile.targetActorId) != nullptr)
            {
                projectile.lastKnownTargetCenter =
                    GetActorFootprintCenter(projectile.targetActorId);
            }
        }

        snapshots.insert(
            snapshots.end(),
            actorProjectiles.begin(),
            actorProjectiles.end());
    };

    std::vector<ActorId> actorIds;
    actorIds.reserve(m_Players.size() + m_Npcs.size());

    for (const auto& [actorId, player] : m_Players)
    {
        actorIds.push_back(actorId);
    }

    for (const auto& [actorId, npc] : m_Npcs)
    {
        actorIds.push_back(actorId);
    }

    std::sort(actorIds.begin(), actorIds.end());

    for (ActorId actorId : actorIds)
    {
        const ActorCore* actor = TryGetActorCore(actorId);

        if (actor != nullptr)
        {
            appendActorProjectiles(*actor);
        }
    }


    return snapshots;
}

ScenePosition World::GetActorFootprintCenter(ActorId actorId) const
{
    const ActorCore* actor = TryGetActorCore(actorId);
    const SceneMembership* membership = GetSceneMembership(actorId);

    if (actor == nullptr || membership == nullptr)
    {
        return {};
    }

    const double halfSize = static_cast<double>(actor->size) / 2.0;
    return ScenePosition{
        static_cast<double>(membership->coordinate.x) + halfSize,
        static_cast<double>(membership->coordinate.y) + halfSize,
        membership->coordinate.plane};
}

bool World::SetActorCombatComposition(
    ActorId actorId,
    CombatComposition combatComposition)
{
    ActorCore* actor = TryGetActorCore(actorId);

    if (actor == nullptr)
    {
        return false;
    }

    actor->combatComposition = combatComposition;

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

bool World::SetNpcBehaviorId(ActorId actorId, NpcBehaviorId behaviorId)
{
    auto npc = m_Npcs.find(actorId);

    if (npc == m_Npcs.end())
    {
        return false;
    }

    npc->second.behaviorId = behaviorId;

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
    UpdateProjectilesTargetingActor(actorId);

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

    const auto player = m_Players.find(actorId);

    if (player != m_Players.end())
    {
        m_LivePlayerIndices.erase(player->second.playerIndex);
        m_PlayerActorsByIndex.erase(player->second.playerIndex);
        m_Players.erase(player);
    }

    const auto npc = m_Npcs.find(actorId);

    if (npc != m_Npcs.end())
    {
        m_LiveNpcIndices.erase(npc->second.npcIndex);
        m_NpcActorsByIndex.erase(npc->second.npcIndex);
        m_Npcs.erase(npc);
    }

    return true;
}

bool World::QueueActorRemoval(ActorId actorId)
{
    if (!HasActor(actorId))
    {
        return false;
    }

    for (ActorId queuedActorId : m_QueuedActorRemovals)
    {
        if (queuedActorId == actorId)
        {
            return true;
        }
    }

    m_QueuedActorRemovals.push_back(actorId);

    return true;
}

std::vector<ActorId> World::TakeQueuedActorRemovals()
{
    std::vector<ActorId> queuedActorRemovals;
    queuedActorRemovals.swap(m_QueuedActorRemovals);
    return queuedActorRemovals;
}

void World::FlushQueuedActorRemovals()
{
    for (ActorId actorId : TakeQueuedActorRemovals())
    {
        RemoveActor(actorId);
    }
}

bool World::MoveActorByDelta(ActorId actorId, int dx, int dy)
{
    ActorCore* actor = TryGetActorCore(actorId);
    auto membershipIterator = m_SceneMemberships.find(actorId);

    if (actor == nullptr || membershipIterator == m_SceneMemberships.end())
    {
        return false;
    }

    SceneMembership& membership = membershipIterator->second;
    Scene* scene = TryGetScene(membership.sceneId);

    if (scene == nullptr)
    {
        return false;
    }

    const ActorKind actorKind = GetActorKind(actorId);
    ActorMovementAccess access(
        *scene,
        membership.coordinate,
        actorKind == ActorKind::Player ? ActorMovementKind::Player
                                       : ActorMovementKind::Npc,
        actor->size,
        actor->speed);
    ActorMovement actorMovement(*scene);
    const ActorMovementResult result = actorMovement.MoveByDelta(access, dx, dy);

    if (!result.moved)
    {
        return false;
    }

    UpdateProjectilesTargetingActor(actorId);

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

    Scene* scene = TryGetScene(membership->sceneId);

    if (movementTarget->value().kind == MovementTargetKind::SceneCoordinate)
    {
        if (scene == nullptr)
        {
            *movementTarget = std::nullopt;
            return false;
        }

        const ActorKind actorKind = GetActorKind(actorId);
        ActorMovementAccess access(
            *scene,
            membership->coordinate,
            actorKind == ActorKind::Player ? ActorMovementKind::Player
                                           : ActorMovementKind::Npc,
            actor->size,
            actor->speed);
        ActorMovement actorMovement(*scene);
        const ActorMovementTargetResult result =
            actorMovement.PursueSceneCoordinateTarget(
                access,
                movementTarget->value().sceneCoordinate);

        if (result.clearTarget)
        {
            *movementTarget = std::nullopt;
        }

        if (result.moved)
        {
            UpdateProjectilesTargetingActor(actorId);
        }

        return result.moved;
    }

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

    const ActorKind actorKind = GetActorKind(actorId);
    ActorMovementAccess access(
        *scene,
        membership->coordinate,
        actorKind == ActorKind::Player ? ActorMovementKind::Player
                                       : ActorMovementKind::Npc,
        actor->size,
        actor->speed);
    ActorMovement actorMovement(*scene);
    const ActorMovementTargetActor target{
        movementTarget->value().actorId,
        targetMembership->coordinate,
        targetActor->size};
    const ActorMovementTargetResult result =
        actorMovement.PursueActorTarget(access, target, actorId, currentTick);

    if (result.clearTarget)
    {
        *movementTarget = std::nullopt;
    }

    if (result.moved)
    {
        UpdateProjectilesTargetingActor(actorId);
    }

    return result.moved;
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

std::optional<PlayerIndex> World::AllocatePlayerIndex()
{
    if (m_LivePlayerIndices.size() > MaxLiveActorIndex)
    {
        return std::nullopt;
    }

    for (int attempt = 0; attempt <= MaxLiveActorIndex; ++attempt)
    {
        const PlayerIndex playerIndex =
            static_cast<PlayerIndex>(m_NextPlayerIndex);

        ++m_NextPlayerIndex;

        if (m_NextPlayerIndex > MaxLiveActorIndex)
        {
            m_NextPlayerIndex = 0;
        }

        if (!m_LivePlayerIndices.contains(playerIndex))
        {
            m_LivePlayerIndices.insert(playerIndex);
            return playerIndex;
        }
    }

    return std::nullopt;
}

std::optional<NpcIndex> World::AllocateNpcIndex()
{
    if (m_LiveNpcIndices.size() > MaxLiveActorIndex)
    {
        return std::nullopt;
    }

    for (int attempt = 0; attempt <= MaxLiveActorIndex; ++attempt)
    {
        const NpcIndex npcIndex = static_cast<NpcIndex>(m_NextNpcIndex);

        ++m_NextNpcIndex;

        if (m_NextNpcIndex > MaxLiveActorIndex)
        {
            m_NextNpcIndex = 0;
        }

        if (!m_LiveNpcIndices.contains(npcIndex))
        {
            m_LiveNpcIndices.insert(npcIndex);
            return npcIndex;
        }
    }

    return std::nullopt;
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

void World::UpdateProjectilesTargetingActor(ActorId actorId)
{
    const ActorCore* targetActor = TryGetActorCore(actorId);
    const SceneMembership* targetMembership = GetSceneMembership(actorId);

    if (targetActor == nullptr || targetMembership == nullptr)
    {
        return;
    }

    const ScenePosition targetCenter = GetActorFootprintCenter(actorId);

    for (auto& [_, player] : m_Players)
    {
        player.actor.combatQueue.UpdateProjectileTargetCenter(
            actorId,
            targetCenter);
    }

    for (auto& [_, npc] : m_Npcs)
    {
        npc.actor.combatQueue.UpdateProjectileTargetCenter(
            actorId,
            targetCenter);
    }
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
