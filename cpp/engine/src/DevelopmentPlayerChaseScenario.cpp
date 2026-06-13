#include "DevelopmentPlayerChaseScenario.h"

#include "Scene.h"
#include "Tile.h"

namespace osrssim
{

DevelopmentPlayerChaseScenario::DevelopmentPlayerChaseScenario()
{
    World& world = m_Engine.GetWorld();
    Scene* scene = world.TryGetScene(world.GetDefaultSceneId());

    CollisionProfile blockingObject;
    blockingObject.blocksMovement = true;
    blockingObject.blocksLineOfSight = true;

    if (scene != nullptr)
    {
        scene->PlaceGameObject(
            {12, 10, 0},
            200,
            CardinalDirection::North,
            2,
            2,
            blockingObject);
    }

    m_PlayerId = world.CreatePlayer(1, 1);
    m_NpcId = world.CreateNpc(2, 1);

    world.PlaceActor(m_NpcId, world.GetDefaultSceneId(), {9, 10, 0});
    world.PlaceActor(m_PlayerId, world.GetDefaultSceneId(), {15, 10, 0});
    world.SetActorMovementTarget(m_NpcId, m_PlayerId);
}

void DevelopmentPlayerChaseScenario::Step()
{
    m_Engine.Step();
}

bool DevelopmentPlayerChaseScenario::ClickSceneCoordinate(int x, int y, int plane)
{
    SceneCoordinate coordinate{x, y, plane};
    World& world = m_Engine.GetWorld();

    if (!world.CanPlayerUseSceneCoordinateMovementTarget(m_PlayerId, coordinate))
    {
        m_LastClickBlocked = true;
        return false;
    }

    m_LastClickBlocked = false;
    return m_Engine.SetPlayerSceneCoordinateMovementTarget(
        m_PlayerId,
        coordinate);
}

void DevelopmentPlayerChaseScenario::SetRunning(bool running)
{
    m_Running = running;
}

bool DevelopmentPlayerChaseScenario::IsRunning() const
{
    return m_Running;
}

bool DevelopmentPlayerChaseScenario::WasLastClickBlocked() const
{
    return m_LastClickBlocked;
}

Tick DevelopmentPlayerChaseScenario::GetTick() const
{
    return m_Engine.GetCurrentTick();
}

int DevelopmentPlayerChaseScenario::GetSceneWidth() const
{
    return Scene::Width;
}

int DevelopmentPlayerChaseScenario::GetSceneHeight() const
{
    return Scene::Height;
}

int DevelopmentPlayerChaseScenario::GetScenePlaneCount() const
{
    return Scene::PlaneCount;
}

ActorId DevelopmentPlayerChaseScenario::GetPlayerId() const
{
    return m_PlayerId;
}

ActorId DevelopmentPlayerChaseScenario::GetNpcId() const
{
    return m_NpcId;
}

int DevelopmentPlayerChaseScenario::GetPlayerX() const
{
    return GetActorX(m_PlayerId);
}

int DevelopmentPlayerChaseScenario::GetPlayerY() const
{
    return GetActorY(m_PlayerId);
}

int DevelopmentPlayerChaseScenario::GetPlayerPlane() const
{
    return GetActorPlane(m_PlayerId);
}

bool DevelopmentPlayerChaseScenario::HasPlayerMovementTarget() const
{
    return GetPlayerMovementTarget() != nullptr;
}

int DevelopmentPlayerChaseScenario::GetPlayerMovementTargetX() const
{
    const MovementTarget* movementTarget = GetPlayerMovementTarget();
    return movementTarget == nullptr ? 0 : movementTarget->sceneCoordinate.x;
}

int DevelopmentPlayerChaseScenario::GetPlayerMovementTargetY() const
{
    const MovementTarget* movementTarget = GetPlayerMovementTarget();
    return movementTarget == nullptr ? 0 : movementTarget->sceneCoordinate.y;
}

int DevelopmentPlayerChaseScenario::GetPlayerMovementTargetPlane() const
{
    const MovementTarget* movementTarget = GetPlayerMovementTarget();
    return movementTarget == nullptr ? 0 : movementTarget->sceneCoordinate.plane;
}

int DevelopmentPlayerChaseScenario::GetNpcX() const
{
    return GetActorX(m_NpcId);
}

int DevelopmentPlayerChaseScenario::GetNpcY() const
{
    return GetActorY(m_NpcId);
}

int DevelopmentPlayerChaseScenario::GetNpcPlane() const
{
    return GetActorPlane(m_NpcId);
}

int DevelopmentPlayerChaseScenario::GetNpcSize() const
{
    const ActorCore* actor = GetActor(m_NpcId);
    return actor == nullptr ? 0 : actor->size;
}

bool DevelopmentPlayerChaseScenario::HasNpcMovementTarget() const
{
    return GetNpcMovementTarget() != nullptr;
}

ActorId DevelopmentPlayerChaseScenario::GetNpcMovementTargetActorId() const
{
    const MovementTarget* movementTarget = GetNpcMovementTarget();
    return movementTarget == nullptr ? 0 : movementTarget->actorId;
}

std::string DevelopmentPlayerChaseScenario::GetNpcMovementTargetLabel() const
{
    const MovementTarget* movementTarget = GetNpcMovementTarget();

    if (movementTarget == nullptr)
    {
        return "None";
    }

    if (movementTarget->kind == MovementTargetKind::Actor &&
        movementTarget->actorId == m_PlayerId)
    {
        return "Player #" + std::to_string(m_PlayerId);
    }

    return "Actor #" + std::to_string(movementTarget->actorId);
}

bool DevelopmentPlayerChaseScenario::IsGameObjectTile(
    int x,
    int y,
    int plane) const
{
    const Scene* scene = GetWorld().TryGetScene(GetWorld().GetDefaultSceneId());

    if (scene == nullptr)
    {
        return false;
    }

    const Tile* tile = scene->TryGetTile({x, y, plane});
    return tile != nullptr && tile->gameObject.has_value();
}

bool DevelopmentPlayerChaseScenario::IsPlayerTile(int x, int y, int plane) const
{
    return IsActorTile(m_PlayerId, x, y, plane);
}

bool DevelopmentPlayerChaseScenario::IsNpcTile(int x, int y, int plane) const
{
    return IsActorTile(m_NpcId, x, y, plane);
}

const World& DevelopmentPlayerChaseScenario::GetWorld() const
{
    return m_Engine.GetWorld();
}

const SceneMembership* DevelopmentPlayerChaseScenario::GetMembership(
    ActorId actorId) const
{
    return GetWorld().GetSceneMembership(actorId);
}

const ActorCore* DevelopmentPlayerChaseScenario::GetActor(ActorId actorId) const
{
    return GetWorld().GetActorCore(actorId);
}

const MovementTarget*
DevelopmentPlayerChaseScenario::GetPlayerMovementTarget() const
{
    const Player* player = GetWorld().GetPlayer(m_PlayerId);

    if (player == nullptr || !player->movementTarget.has_value())
    {
        return nullptr;
    }

    return &player->movementTarget.value();
}

const MovementTarget* DevelopmentPlayerChaseScenario::GetNpcMovementTarget() const
{
    const Npc* npc = GetWorld().GetNpc(m_NpcId);

    if (npc == nullptr || !npc->movementTarget.has_value())
    {
        return nullptr;
    }

    return &npc->movementTarget.value();
}

int DevelopmentPlayerChaseScenario::GetActorX(ActorId actorId) const
{
    const SceneMembership* membership = GetMembership(actorId);
    return membership == nullptr ? 0 : membership->coordinate.x;
}

int DevelopmentPlayerChaseScenario::GetActorY(ActorId actorId) const
{
    const SceneMembership* membership = GetMembership(actorId);
    return membership == nullptr ? 0 : membership->coordinate.y;
}

int DevelopmentPlayerChaseScenario::GetActorPlane(ActorId actorId) const
{
    const SceneMembership* membership = GetMembership(actorId);
    return membership == nullptr ? 0 : membership->coordinate.plane;
}

bool DevelopmentPlayerChaseScenario::IsActorTile(
    ActorId actorId,
    int x,
    int y,
    int plane) const
{
    const ActorCore* actor = GetActor(actorId);
    const SceneMembership* membership = GetMembership(actorId);

    if (actor == nullptr || membership == nullptr ||
        membership->coordinate.plane != plane)
    {
        return false;
    }

    return x >= membership->coordinate.x &&
           x < membership->coordinate.x + actor->size &&
           y >= membership->coordinate.y &&
           y < membership->coordinate.y + actor->size;
}

}  // namespace osrssim
