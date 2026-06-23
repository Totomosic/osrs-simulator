#include "debug/DevelopmentPlayerChaseScenario.h"

#include <cassert>

int main()
{
    osrssim::debug::DevelopmentPlayerChaseScenario scenario;
    osrssim::World& world = scenario.GetWorld();

    const osrssim::ActorId playerId = scenario.GetPlayerId();
    const osrssim::ActorId npcId = scenario.GetSelectedNpcId();

    assert(scenario.GetCurrentTick() == 0);
    assert(world.GetPlayer(playerId) != nullptr);
    assert(world.GetNpc(npcId) != nullptr);
    assert(world.GetSceneMembership(playerId)->coordinate == (osrssim::SceneCoordinate{8, 11, 0}));
    assert(world.GetSceneMembership(npcId)->coordinate == (osrssim::SceneCoordinate{18, 20, 0}));
    assert(world.GetActorCombatComposition(playerId)->weapon.range == 5);
    assert(world.GetActorCombatComposition(npcId)->weapon.range == 8);
    assert(scenario.GetNpcIdsJson() == "[2]");

    scenario.Step();
    assert(scenario.GetCurrentTick() == 1);

    scenario.SetRunning(true);
    assert(scenario.IsRunning());

    assert(scenario.PlaceNpc(1, 1, 6, 7, 0));
    assert(scenario.GetSelectedNpcId() == 3);
    assert(scenario.GetNpcIdsJson() == "[2,3]");
    assert(scenario.RemoveNpc(6, 7, 0));
    assert(scenario.GetSelectedNpcId() == 2);
    assert(scenario.GetNpcIdsJson() == "[2]");

    assert(!scenario.ClickSceneCoordinate(12, 4, 0));
    assert(scenario.WasLastClickBlocked());
    assert(scenario.PlaceGameObject(30, 30, 0, 1, 1, osrssim::CardinalDirection::North, true, true));
    assert(!scenario.ClickSceneCoordinate(30, 30, 0));
    assert(scenario.RemoveGameObject(30, 30, 0));

    scenario.Reset();
    assert(scenario.GetCurrentTick() == 0);
    assert(!scenario.IsRunning());
    assert(scenario.GetPlayerId() == 1);
    assert(scenario.GetSelectedNpcId() == 2);
    assert(scenario.GetNpcIdsJson() == "[2]");
}
