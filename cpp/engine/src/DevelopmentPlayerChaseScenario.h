#pragma once

#include "Engine.h"
#include "Types.h"

#include <string>

namespace osrssim
{

class DevelopmentPlayerChaseScenario
{
private:
    Engine m_Engine;
    ActorId m_PlayerId = 0;
    ActorId m_NpcId = 0;
    bool m_Running = false;
    bool m_LastClickBlocked = false;

public:
    DevelopmentPlayerChaseScenario();

    void Step();
    bool ClickSceneCoordinate(int x, int y, int plane);
    void SetRunning(bool running);
    bool IsRunning() const;
    bool WasLastClickBlocked() const;
    std::string GetSnapshotJson() const;
    Tick GetTick() const;
    int GetSceneWidth() const;
    int GetSceneHeight() const;
    int GetScenePlaneCount() const;
    ActorId GetPlayerId() const;
    ActorId GetNpcId() const;
    int GetPlayerX() const;
    int GetPlayerY() const;
    int GetPlayerPlane() const;
    bool HasPlayerMovementTarget() const;
    int GetPlayerMovementTargetX() const;
    int GetPlayerMovementTargetY() const;
    int GetPlayerMovementTargetPlane() const;
    int GetNpcX() const;
    int GetNpcY() const;
    int GetNpcPlane() const;
    int GetNpcSize() const;
    bool HasNpcMovementTarget() const;
    ActorId GetNpcMovementTargetActorId() const;
    std::string GetNpcMovementTargetLabel() const;
    bool IsGameObjectTile(int x, int y, int plane) const;
    bool IsPlayerTile(int x, int y, int plane) const;
    bool IsNpcTile(int x, int y, int plane) const;

private:
    const World& GetWorld() const;
    const SceneMembership* GetMembership(ActorId actorId) const;
    const ActorCore* GetActor(ActorId actorId) const;
    const MovementTarget* GetPlayerMovementTarget() const;
    const MovementTarget* GetNpcMovementTarget() const;
    int GetActorX(ActorId actorId) const;
    int GetActorY(ActorId actorId) const;
    int GetActorPlane(ActorId actorId) const;
    bool IsActorTile(ActorId actorId, int x, int y, int plane) const;
};

}  // namespace osrssim
