#pragma once

#include "Pathing.h"
#include "Scene.h"
#include "Types.h"

#include <optional>
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
    std::optional<SceneCoordinate> movementTarget;
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
    const std::unordered_map<ActorId, Player>& GetPlayers() const;
    const std::unordered_map<ActorId, Npc>& GetNpcs() const;
    const std::unordered_map<ActorId, SceneMembership>& GetSceneMemberships() const;
    const Player* GetPlayer(ActorId actorId) const;
    const Npc* GetNpc(ActorId actorId) const;
    const ActorCore* GetActorCore(ActorId actorId) const;
    const SceneMembership* GetSceneMembership(ActorId actorId) const;
    bool SetActorSpeed(ActorId actorId, int speed);
    bool PlaceActor(ActorId actorId, SceneId sceneId, SceneCoordinate coordinate);
    bool RemoveActorSceneMembership(ActorId actorId);
    bool RemoveActor(ActorId actorId);
    bool MoveActorByDelta(ActorId actorId, int dx, int dy);
    bool CanPlayerUseSceneCoordinateMovementTarget(
        ActorId actorId,
        SceneCoordinate coordinate) const;
    bool SetPlayerSceneCoordinateMovementTarget(
        ActorId actorId,
        SceneCoordinate coordinate);
    bool UpdatePlayerMovement(ActorId actorId);

private:
    enum class ActorKind
    {
        Player,
        Npc,
    };

    static int ClampSize(int size);
    static int ClampSpeed(int speed);
    static int ClampDelta(int delta, int speed);
    static bool IsWholeTileMovementBlocked(const Tile& tile);

    Player* TryGetPlayer(ActorId actorId);
    const Player* TryGetPlayer(ActorId actorId) const;
    ActorCore* TryGetActorCore(ActorId actorId);
    const ActorCore* TryGetActorCore(ActorId actorId) const;
    ActorKind GetActorKind(ActorId actorId) const;
    bool HasActor(ActorId actorId) const;
    bool CanUseSceneCoordinateMovementTarget(
        const Scene& scene,
        SceneCoordinate coordinate) const;
    bool DoesActorFootprintCover(
        const ActorCore& actor,
        SceneCoordinate actorCoordinate,
        SceneCoordinate target) const;
    int GetMovementDeltaForAxis(int anchor, int size, int target, int speed) const;
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
