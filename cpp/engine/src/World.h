#pragma once

#include "Pathing.h"
#include "Scene.h"
#include "Types.h"

#include <unordered_map>

namespace osrssim
{

struct ActorCore
{
    ActorId id = 0;
    int size = 1;
    int speed = 0;
};

struct Player
{
    ActorCore actor;
};

struct Npc
{
    ActorCore actor;
};

struct SceneMembership
{
    SceneId sceneId = 0;
    SceneCoordinate coordinate;
};

class World
{
private:
    SceneId m_DefaultSceneId = 1;
    ActorId m_NextActorId = 1;
    Scene m_Scene;
    std::unordered_map<ActorId, Player> m_Players;
    std::unordered_map<ActorId, Npc> m_Npcs;
    std::unordered_map<ActorId, SceneMembership> m_SceneMemberships;

public:
    SceneId GetDefaultSceneId() const;
    Scene* TryGetScene(SceneId sceneId);
    const Scene* TryGetScene(SceneId sceneId) const;
    ActorId CreatePlayer(int size, int speed);
    ActorId CreateNpc(int size, int speed);
    const Player* GetPlayer(ActorId actorId) const;
    const Npc* GetNpc(ActorId actorId) const;
    const ActorCore* GetActorCore(ActorId actorId) const;
    const SceneMembership* GetSceneMembership(ActorId actorId) const;
    bool PlaceActor(ActorId actorId, SceneId sceneId, SceneCoordinate coordinate);
    bool MoveActorByDelta(ActorId actorId, int dx, int dy);

private:
    enum class ActorKind
    {
        Player,
        Npc,
    };

    static int ClampSize(int size);
    static int ClampSpeed(int speed);
    static bool IsWholeTileMovementBlocked(const Tile& tile);

    ActorCore* TryGetActorCore(ActorId actorId);
    const ActorCore* TryGetActorCore(ActorId actorId) const;
    ActorKind GetActorKind(ActorId actorId) const;
    bool HasActor(ActorId actorId) const;
    bool CanStandOnMovementBlockers(
        const Scene& scene,
        SceneCoordinate coordinate,
        int actorSize) const;
    bool HasFinalNpcOccupancyConflict(
        const Scene& scene,
        SceneCoordinate current,
        SceneCoordinate destination,
        int actorSize) const;
    void AddActorOccupancy(
        Scene& scene,
        SceneCoordinate coordinate,
        int actorSize);
    void RemoveActorOccupancy(
        Scene& scene,
        SceneCoordinate coordinate,
        int actorSize);
};

}  // namespace osrssim
