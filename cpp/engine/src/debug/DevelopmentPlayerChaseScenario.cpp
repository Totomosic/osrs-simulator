#include "debug/DevelopmentPlayerChaseScenario.h"

#include "EncounterContext.h"

#include <algorithm>
#include <sstream>
#include <stdexcept>

namespace osrssim::debug
{
namespace
{
constexpr int PlayerWeaponRange = 5;
constexpr int NpcWeaponRange = 8;
constexpr SceneCoordinate InitialPlayerCoordinate{8, 11, 0};
constexpr SceneCoordinate InitialNpcCoordinate{18, 20, 0};
constexpr SceneCoordinate InitialGameObjectCoordinate{12, 4, 0};

CombatComposition CreatePlayerChaseCombatComposition(WeaponDefinition weapon)
{
    CombatComposition composition;
    composition.weapon = weapon;
    composition.attackType = AttackType::Slash;

    return composition;
}

bool IsCoordinateInActorFootprint(
    const World& world,
    ActorId actorId,
    SceneCoordinate coordinate)
{
    const ActorCore* actor = world.GetActorCore(actorId);
    const SceneMembership* membership = world.GetSceneMembership(actorId);

    if (actor == nullptr || membership == nullptr)
    {
        return false;
    }

    return coordinate.plane == membership->coordinate.plane &&
           coordinate.x >= membership->coordinate.x &&
           coordinate.x < membership->coordinate.x + actor->size &&
           coordinate.y >= membership->coordinate.y &&
           coordinate.y < membership->coordinate.y + actor->size;
}
}  // namespace

void PlayerChaseEncounter::Initialize(EncounterContext& context)
{
    World& world = context.GetWorld();
    const SceneId sceneId = world.GetDefaultSceneId();

    const std::optional<ActorId> playerId = context.CreatePlayer(
        1,
        2,
        CreatePlayerChaseCombatComposition({2, PlayerWeaponRange, 4, 61}));
    const std::optional<ActorId> npcId = context.CreateNpc(
        4,
        1,
        CreatePlayerChaseCombatComposition({0, NpcWeaponRange, 4, 1}));

    if (!playerId.has_value() || !npcId.has_value())
    {
        throw std::runtime_error("Player Chase actor creation failed");
    }

    m_PlayerId = playerId.value();
    m_NpcIds = {npcId.value()};
    m_SelectedNpcId = npcId.value();

    world.PlaceActor(m_PlayerId, sceneId, InitialPlayerCoordinate);
    world.PlaceActor(npcId.value(), sceneId, InitialNpcCoordinate);
    world.SetActorMovementTarget(npcId.value(), m_PlayerId);

    Scene* scene = world.TryGetScene(sceneId);

    if (scene == nullptr)
    {
        throw std::runtime_error("Player Chase requires the default scene");
    }

    scene->PlaceGameObject(
        InitialGameObjectCoordinate,
        200,
        CardinalDirection::North,
        3,
        3,
        {true, true});
}

bool PlayerChaseEncounter::IsComplete(const EncounterContext&) const
{
    return false;
}

ActorId PlayerChaseEncounter::GetPlayerId() const
{
    return m_PlayerId;
}

const std::vector<ActorId>& PlayerChaseEncounter::GetNpcIds() const
{
    return m_NpcIds;
}

ActorId PlayerChaseEncounter::GetSelectedNpcId() const
{
    return m_SelectedNpcId;
}

void PlayerChaseEncounter::AddNpc(ActorId npcId)
{
    m_NpcIds.push_back(npcId);
    m_SelectedNpcId = npcId;
}

void PlayerChaseEncounter::RemoveNpc(ActorId npcId)
{
    m_NpcIds.erase(
        std::remove(m_NpcIds.begin(), m_NpcIds.end(), npcId),
        m_NpcIds.end());

    if (m_SelectedNpcId == npcId)
    {
        m_SelectedNpcId = m_NpcIds.empty() ? 0 : m_NpcIds.front();
    }
}

DevelopmentPlayerChaseScenario::DevelopmentPlayerChaseScenario()
{
    CreateRunner();
}

void DevelopmentPlayerChaseScenario::Step()
{
    m_Runner->Step();
}

void DevelopmentPlayerChaseScenario::Reset()
{
    CreateRunner();
    m_Running = false;
    m_LastClickBlocked = false;
    m_NextGameObjectId = 201;
}

bool DevelopmentPlayerChaseScenario::ClickSceneCoordinate(
    int x,
    int y,
    int plane)
{
    const SceneCoordinate coordinate{x, y, plane};

    if (!GetWorld().CanPlayerUseSceneCoordinateMovementTarget(
            m_Encounter->GetPlayerId(),
            coordinate))
    {
        m_LastClickBlocked = true;
        return false;
    }

    m_LastClickBlocked = false;
    return m_Runner->GetEngine().SetPlayerSceneCoordinateMovementTarget(
        m_Encounter->GetPlayerId(),
        coordinate);
}

bool DevelopmentPlayerChaseScenario::PlaceNpc(
    int size,
    int speed,
    int x,
    int y,
    int plane)
{
    const std::optional<ActorId> npcId = m_Runner->GetEngine().CreateNpc(
        size,
        speed,
        CreatePlayerChaseCombatComposition({0, NpcWeaponRange, 4, 1}));

    if (!npcId.has_value())
    {
        return false;
    }

    const bool placed = GetWorld().PlaceActor(
        npcId.value(),
        GetWorld().GetDefaultSceneId(),
        {x, y, plane});

    if (!placed)
    {
        m_Runner->GetEngine().RemoveNpc(npcId.value());
        return false;
    }

    GetWorld().SetActorMovementTarget(npcId.value(), m_Encounter->GetPlayerId());
    m_Encounter->AddNpc(npcId.value());
    m_LastClickBlocked = false;

    return true;
}

bool DevelopmentPlayerChaseScenario::RemoveNpc(int x, int y, int plane)
{
    const ActorId npcId = FindNpcIdAtCoordinate({x, y, plane});

    if (npcId == 0)
    {
        return false;
    }

    if (!m_Runner->GetEngine().RemoveNpc(npcId))
    {
        return false;
    }

    m_Encounter->RemoveNpc(npcId);
    m_LastClickBlocked = false;

    return true;
}

bool DevelopmentPlayerChaseScenario::PlaceGameObject(
    int x,
    int y,
    int plane,
    int sizeX,
    int sizeY,
    CardinalDirection direction,
    bool blocksMovement,
    bool blocksLineOfSight)
{
    const bool placed = GetDefaultScene().PlaceGameObject(
        {x, y, plane},
        m_NextGameObjectId,
        direction,
        sizeX,
        sizeY,
        {blocksMovement, blocksLineOfSight});

    if (placed)
    {
        ++m_NextGameObjectId;
        m_LastClickBlocked = false;
    }

    return placed;
}

bool DevelopmentPlayerChaseScenario::RemoveGameObject(int x, int y, int plane)
{
    const bool removed = GetDefaultScene().RemoveGameObject({x, y, plane});

    if (removed)
    {
        m_LastClickBlocked = false;
    }

    return removed;
}

bool DevelopmentPlayerChaseScenario::HasLineOfSight(
    ActorId actorId,
    int x,
    int y,
    int plane,
    int range) const
{
    const ActorCore* actor = GetWorld().GetActorCore(actorId);
    const SceneMembership* membership = GetWorld().GetSceneMembership(actorId);

    if (actor == nullptr || membership == nullptr)
    {
        return false;
    }

    LineOfSight lineOfSight(GetDefaultScene());

    return lineOfSight.HasLineOfSight(
        membership->coordinate,
        actor->size,
        {x, y, plane},
        range);
}

bool DevelopmentPlayerChaseScenario::HasActorLineOfSight(
    ActorId sourceActorId,
    ActorId targetActorId,
    int range) const
{
    const ActorCore* source = GetWorld().GetActorCore(sourceActorId);
    const ActorCore* target = GetWorld().GetActorCore(targetActorId);
    const SceneMembership* sourceMembership =
        GetWorld().GetSceneMembership(sourceActorId);
    const SceneMembership* targetMembership =
        GetWorld().GetSceneMembership(targetActorId);

    if (
        source == nullptr ||
        target == nullptr ||
        sourceMembership == nullptr ||
        targetMembership == nullptr)
    {
        return false;
    }

    LineOfSight lineOfSight(GetDefaultScene());

    return lineOfSight.HasLineOfSight(
        sourceMembership->coordinate,
        source->size,
        targetMembership->coordinate,
        target->size,
        range);
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

Tick DevelopmentPlayerChaseScenario::GetCurrentTick() const
{
    return m_Runner->GetEngine().GetCurrentTick();
}

World& DevelopmentPlayerChaseScenario::GetWorld()
{
    return m_Runner->GetEngine().GetWorld();
}

const World& DevelopmentPlayerChaseScenario::GetWorld() const
{
    return m_Runner->GetEngine().GetWorld();
}

ActorId DevelopmentPlayerChaseScenario::GetPlayerId() const
{
    return m_Encounter->GetPlayerId();
}

std::string DevelopmentPlayerChaseScenario::GetNpcIdsJson() const
{
    std::ostringstream output;
    output << "[";

    const std::vector<ActorId>& npcIds = m_Encounter->GetNpcIds();

    for (std::size_t index = 0; index < npcIds.size(); ++index)
    {
        if (index > 0)
        {
            output << ",";
        }

        output << npcIds[index];
    }

    output << "]";
    return output.str();
}

ActorId DevelopmentPlayerChaseScenario::GetSelectedNpcId() const
{
    return m_Encounter->GetSelectedNpcId();
}

void DevelopmentPlayerChaseScenario::CreateRunner()
{
    auto encounter = std::make_unique<PlayerChaseEncounter>();
    m_Encounter = encounter.get();
    m_Runner = std::make_unique<encounter::EncounterRunner>(std::move(encounter));
    m_Runner->Start();
}

Scene& DevelopmentPlayerChaseScenario::GetDefaultScene()
{
    Scene* scene = GetWorld().TryGetScene(GetWorld().GetDefaultSceneId());

    if (scene == nullptr)
    {
        throw std::runtime_error("Player Chase requires the default scene");
    }

    return *scene;
}

const Scene& DevelopmentPlayerChaseScenario::GetDefaultScene() const
{
    const Scene* scene = GetWorld().TryGetScene(GetWorld().GetDefaultSceneId());

    if (scene == nullptr)
    {
        throw std::runtime_error("Player Chase requires the default scene");
    }

    return *scene;
}

ActorId DevelopmentPlayerChaseScenario::FindNpcIdAtCoordinate(
    SceneCoordinate coordinate) const
{
    for (ActorId npcId : m_Encounter->GetNpcIds())
    {
        if (IsCoordinateInActorFootprint(GetWorld(), npcId, coordinate))
        {
            return npcId;
        }
    }

    return 0;
}

}  // namespace osrssim::debug
