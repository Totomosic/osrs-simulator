#pragma once

#include "DpsService.h"
#include "LineOfSight.h"
#include "Scene.h"
#include "Types.h"
#include "World.h"
#include "encounter/ActiveEncounter.h"
#include "encounter/EncounterRunner.h"

#include <memory>
#include <string>
#include <vector>

namespace osrssim::debug
{

class PlayerChaseEncounter : public encounter::ActiveEncounter
{
private:
    ActorId m_PlayerId = 0;
    std::vector<ActorId> m_NpcIds;
    ActorId m_SelectedNpcId = 0;

public:
    void Initialize(EncounterContext& context) override;
    bool IsComplete(const EncounterContext& context) const override;

    ActorId GetPlayerId() const;
    const std::vector<ActorId>& GetNpcIds() const;
    ActorId GetSelectedNpcId() const;
    void AddNpc(ActorId npcId);
    void RemoveNpc(ActorId npcId);
};

class DevelopmentPlayerChaseScenario
{
private:
    std::unique_ptr<encounter::EncounterRunner> m_Runner;
    PlayerChaseEncounter* m_Encounter = nullptr;
    bool m_Running = false;
    bool m_LastClickBlocked = false;
    EntityId m_NextGameObjectId = 201;

public:
    DevelopmentPlayerChaseScenario();

    void Step();
    void Reset();
    bool ClickSceneCoordinate(int x, int y, int plane);
    bool PlaceNpc(int size, int speed, int x, int y, int plane);
    bool RemoveNpc(int x, int y, int plane);
    bool PlaceGameObject(
        int x,
        int y,
        int plane,
        int sizeX,
        int sizeY,
        CardinalDirection direction,
        bool blocksMovement,
        bool blocksLineOfSight);
    bool RemoveGameObject(int x, int y, int plane);
    bool HasLineOfSight(
        ActorId actorId,
        int x,
        int y,
        int plane,
        int range) const;
    bool HasActorLineOfSight(
        ActorId sourceActorId,
        ActorId targetActorId,
        int range) const;
    void SetRunning(bool running);
    bool IsRunning() const;
    bool WasLastClickBlocked() const;
    Tick GetCurrentTick() const;
    World& GetWorld();
    const World& GetWorld() const;
    ActorId GetPlayerId() const;
    std::string GetNpcIdsJson() const;
    ActorId GetSelectedNpcId() const;

private:
    void CreateRunner();
    Scene& GetDefaultScene();
    const Scene& GetDefaultScene() const;
    ActorId FindNpcIdAtCoordinate(SceneCoordinate coordinate) const;
};

}  // namespace osrssim::debug
