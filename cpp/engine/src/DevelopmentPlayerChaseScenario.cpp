#include "DevelopmentPlayerChaseScenario.h"

#include "Scene.h"
#include "Tile.h"

#include <sstream>

namespace osrssim
{
namespace
{
void AppendCoordinateJson(std::ostringstream& output, SceneCoordinate coordinate)
{
    output << "{\"x\":" << coordinate.x << ",\"y\":" << coordinate.y
           << ",\"plane\":" << coordinate.plane << "}";
}

void AppendMovementTargetJson(
    std::ostringstream& output,
    const MovementTarget* movementTarget,
    ActorId playerId)
{
    if (movementTarget == nullptr)
    {
        output << "null";
        return;
    }

    if (movementTarget->kind == MovementTargetKind::SceneCoordinate)
    {
        output << "{\"kind\":\"SceneCoordinate\",\"coordinate\":";
        AppendCoordinateJson(output, movementTarget->sceneCoordinate);
        output << "}";
        return;
    }

    output << "{\"kind\":\"Actor\",\"actorId\":" << movementTarget->actorId
           << ",\"label\":\""
           << (movementTarget->actorId == playerId ? "Player #" : "Actor #")
           << movementTarget->actorId << "\"}";
}

void AppendTileFlagsJson(std::ostringstream& output, const Tile& tile)
{
    bool hasPrevious = false;

    auto appendFlag = [&](TileFlag flag, const char* label)
    {
        if (!tile.HasFlag(flag))
        {
            return;
        }

        if (hasPrevious)
        {
            output << ",";
        }

        output << "\"" << label << "\"";
        hasPrevious = true;
    };

    appendFlag(TileFlag::Occupied, "Occupied");
    appendFlag(TileFlag::BlockMovementNorthWest, "BlockMovementNorthWest");
    appendFlag(TileFlag::BlockMovementNorth, "BlockMovementNorth");
    appendFlag(TileFlag::BlockMovementNorthEast, "BlockMovementNorthEast");
    appendFlag(TileFlag::BlockMovementEast, "BlockMovementEast");
    appendFlag(TileFlag::BlockMovementSouthEast, "BlockMovementSouthEast");
    appendFlag(TileFlag::BlockMovementSouth, "BlockMovementSouth");
    appendFlag(TileFlag::BlockMovementSouthWest, "BlockMovementSouthWest");
    appendFlag(TileFlag::BlockMovementWest, "BlockMovementWest");
    appendFlag(TileFlag::BlockMovementFull, "BlockMovementFull");
    appendFlag(TileFlag::BlockMovementObject, "BlockMovementObject");
    appendFlag(TileFlag::BlockMovementFloor, "BlockMovementFloor");
    appendFlag(
        TileFlag::BlockMovementFloorDecoration,
        "BlockMovementFloorDecoration");
    appendFlag(TileFlag::BlockLineOfSightNorth, "BlockLineOfSightNorth");
    appendFlag(TileFlag::BlockLineOfSightEast, "BlockLineOfSightEast");
    appendFlag(TileFlag::BlockLineOfSightSouth, "BlockLineOfSightSouth");
    appendFlag(TileFlag::BlockLineOfSightWest, "BlockLineOfSightWest");
    appendFlag(TileFlag::BlockLineOfSightFull, "BlockLineOfSightFull");
}
}  // namespace

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
            {12, 4, 0},
            200,
            CardinalDirection::North,
            3,
            3,
            blockingObject);
    }

    m_PlayerId = world.CreatePlayer(1, 2);
    m_NpcId = world.CreateNpc(4, 1);

    world.PlaceActor(m_PlayerId, world.GetDefaultSceneId(), {8, 11, 0});
    world.PlaceActor(m_NpcId, world.GetDefaultSceneId(), {18, 20, 0});
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

std::string DevelopmentPlayerChaseScenario::GetSnapshotJson() const
{
    const World& world = GetWorld();
    const Scene* scene = world.TryGetScene(world.GetDefaultSceneId());
    const ActorCore* player = GetActor(m_PlayerId);
    const ActorCore* npc = GetActor(m_NpcId);
    const SceneMembership* playerMembership = GetMembership(m_PlayerId);
    const SceneMembership* npcMembership = GetMembership(m_NpcId);
    std::ostringstream output;

    output << "{\"name\":\"Player Chase\",\"tick\":" << GetTick()
           << ",\"running\":" << (IsRunning() ? "true" : "false")
           << ",\"blockedClick\":"
           << (WasLastClickBlocked() ? "true" : "false")
           << ",\"scene\":{\"id\":" << world.GetDefaultSceneId()
           << ",\"width\":" << Scene::Width << ",\"height\":" << Scene::Height
           << ",\"planeCount\":" << Scene::PlaneCount << "}";

    output << ",\"player\":{\"id\":" << m_PlayerId << ",\"kind\":\"Player\""
           << ",\"size\":" << (player == nullptr ? 0 : player->size)
           << ",\"speed\":" << (player == nullptr ? 0 : player->speed)
           << ",\"coordinate\":";
    AppendCoordinateJson(
        output,
        playerMembership == nullptr ? SceneCoordinate{} :
                                      playerMembership->coordinate);
    output << ",\"movementTarget\":";
    AppendMovementTargetJson(output, GetPlayerMovementTarget(), m_PlayerId);
    output << "}";

    output << ",\"npc\":{\"id\":" << m_NpcId << ",\"kind\":\"NPC\""
           << ",\"size\":" << (npc == nullptr ? 0 : npc->size)
           << ",\"speed\":" << (npc == nullptr ? 0 : npc->speed)
           << ",\"coordinate\":";
    AppendCoordinateJson(
        output,
        npcMembership == nullptr ? SceneCoordinate{} : npcMembership->coordinate);
    output << ",\"movementTarget\":";
    AppendMovementTargetJson(output, GetNpcMovementTarget(), m_PlayerId);
    output << "}";

    output << ",\"tiles\":[";

    bool hasPreviousTile = false;
    if (scene != nullptr)
    {
        for (int plane = 0; plane < Scene::PlaneCount; ++plane)
        {
            for (int x = 0; x < Scene::Width; ++x)
            {
                for (int y = 0; y < Scene::Height; ++y)
                {
                    const Tile* tile = scene->TryGetTile({x, y, plane});

                    if (tile == nullptr ||
                        (tile->flags == ToTileFlags(TileFlag::None) &&
                         !tile->gameObject.has_value()))
                    {
                        continue;
                    }

                    if (hasPreviousTile)
                    {
                        output << ",";
                    }

                    output << "{\"coordinate\":";
                    AppendCoordinateJson(output, tile->coordinate);
                    output << ",\"flags\":[";
                    AppendTileFlagsJson(output, *tile);
                    output << "]";

                    if (tile->gameObject.has_value())
                    {
                        output << ",\"gameObject\":{\"id\":"
                               << tile->gameObject->id << ",\"origin\":";
                        AppendCoordinateJson(output, tile->gameObject->origin);
                        output << ",\"sizeX\":" << tile->gameObject->sizeX
                               << ",\"sizeY\":" << tile->gameObject->sizeY
                               << "}";
                    }

                    output << "}";
                    hasPreviousTile = true;
                }
            }
        }
    }

    output << "]}";

    return output.str();
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
