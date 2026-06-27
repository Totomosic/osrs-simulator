#include "ActorMovement.h"
#include "Scene.h"
#include "Tile.h"

#include <cassert>

int main()
{
    {
        osrssim::Scene scene;
        osrssim::ActorMovement actorMovement(scene);

        osrssim::SceneCoordinate origin{10, 10, 0};
        osrssim::SceneCoordinate east{11, 10, 0};
        osrssim::SceneCoordinate north{10, 11, 0};
        osrssim::SceneCoordinate northEast{11, 11, 0};
        osrssim::SceneCoordinate same{10, 10, 0};
        osrssim::SceneCoordinate far{12, 10, 0};
        osrssim::SceneCoordinate otherPlane{11, 10, 1};
        osrssim::SceneCoordinate outside{104, 10, 0};

        assert(actorMovement.CanMove(origin, east));
        assert(actorMovement.CanMove(origin, north));
        assert(actorMovement.CanMove(origin, northEast));
        assert(!actorMovement.CanMove(origin, same));
        assert(!actorMovement.CanMove(origin, far));
        assert(!actorMovement.CanMove(origin, otherPlane));
        assert(!actorMovement.CanMove(origin, outside));
    }

    {
        osrssim::Scene scene;
        osrssim::ActorMovement actorMovement(scene);

        osrssim::SceneCoordinate origin{10, 10, 0};
        osrssim::SceneCoordinate east{11, 10, 0};

        scene.TryGetTile(origin)->AddFlag(osrssim::TileFlag::BlockMovementEast);
        assert(!actorMovement.CanMove(origin, east));

        scene.TryGetTile(origin)->RemoveFlag(osrssim::TileFlag::BlockMovementEast);
        scene.TryGetTile(east)->AddFlag(osrssim::TileFlag::BlockMovementWest);
        assert(!actorMovement.CanMove(origin, east));
    }

    {
        osrssim::Scene scene;
        osrssim::ActorMovement actorMovement(scene);

        osrssim::SceneCoordinate origin{10, 10, 0};
        osrssim::SceneCoordinate east{11, 10, 0};

        scene.TryGetTile(east)->AddFlag(osrssim::TileFlag::Occupied);
        assert(!actorMovement.CanMove(origin, east));
    }

    {
        osrssim::Scene scene;
        osrssim::ActorMovement actorMovement(scene);

        osrssim::SceneCoordinate origin{10, 10, 0};
        osrssim::SceneCoordinate east{11, 10, 0};

        scene.TryGetTile(east)->AddFlag(osrssim::TileFlag::BlockLineOfSightFull);
        assert(actorMovement.CanMove(origin, east));
    }

    {
        osrssim::Scene scene;
        osrssim::ActorMovement actorMovement(scene);

        osrssim::SceneCoordinate origin{10, 10, 0};
        osrssim::SceneCoordinate east{11, 10, 0};
        osrssim::SceneCoordinate north{10, 11, 0};
        osrssim::SceneCoordinate northEast{11, 11, 0};

        scene.TryGetTile(origin)->AddFlag(osrssim::TileFlag::BlockMovementNorthEast);
        assert(!actorMovement.CanMove(origin, northEast));

        scene.TryGetTile(origin)->RemoveFlag(osrssim::TileFlag::BlockMovementNorthEast);
        scene.TryGetTile(northEast)->AddFlag(osrssim::TileFlag::BlockMovementSouthWest);
        assert(!actorMovement.CanMove(origin, northEast));

        scene.TryGetTile(northEast)->RemoveFlag(osrssim::TileFlag::BlockMovementSouthWest);
        scene.TryGetTile(east)->AddFlag(osrssim::TileFlag::Occupied);
        assert(!actorMovement.CanMove(origin, northEast));

        scene.TryGetTile(east)->RemoveFlag(osrssim::TileFlag::Occupied);
        scene.TryGetTile(north)->AddFlag(osrssim::TileFlag::BlockMovementFull);
        assert(!actorMovement.CanMove(origin, northEast));
    }

    {
        osrssim::Scene scene;
        osrssim::ActorMovement actorMovement(scene);

        osrssim::SceneCoordinate origin{10, 10, 0};
        osrssim::SceneCoordinate east{11, 10, 0};
        osrssim::SceneCoordinate north{10, 11, 0};
        osrssim::SceneCoordinate south{10, 9, 0};

        osrssim::CollisionProfile wallCollision;
        wallCollision.blocksMovement = true;

        assert(scene.PlaceWallObject(
            origin,
            100,
            osrssim::CardinalDirection::East,
            wallCollision));

        assert(!actorMovement.CanMove(origin, east));
        assert(!actorMovement.CanMove(east, origin));
        assert(actorMovement.CanMove(origin, north));
        assert(actorMovement.CanMove(origin, south));

        assert(scene.RemoveWallObject(origin));
        assert(actorMovement.CanMove(origin, east));
        assert(actorMovement.CanMove(east, origin));
    }

    {
        osrssim::Scene scene;
        osrssim::ActorMovement actorMovement(scene);

        osrssim::SceneCoordinate origin{10, 10, 0};
        osrssim::SceneCoordinate east{11, 10, 0};
        osrssim::SceneCoordinate north{10, 11, 0};
        osrssim::SceneCoordinate south{10, 9, 0};

        osrssim::CollisionProfile eastCollision;
        eastCollision.blocksMovement = true;

        osrssim::CollisionProfile northCollision;
        northCollision.blocksMovement = true;

        assert(scene.PlaceWallObject(
            origin,
            101,
            osrssim::CardinalDirection::East,
            eastCollision,
            osrssim::CardinalDirection::North,
            northCollision));

        assert(!actorMovement.CanMove(origin, east));
        assert(!actorMovement.CanMove(east, origin));
        assert(!actorMovement.CanMove(origin, north));
        assert(!actorMovement.CanMove(north, origin));
        assert(actorMovement.CanMove(origin, south));

        assert(scene.RemoveWallObject(origin));
        assert(actorMovement.CanMove(origin, east));
        assert(actorMovement.CanMove(east, origin));
        assert(actorMovement.CanMove(origin, north));
        assert(actorMovement.CanMove(north, origin));
    }

    {
        osrssim::Scene scene;
        osrssim::ActorMovement actorMovement(scene);

        osrssim::SceneCoordinate origin{10, 10, 0};
        osrssim::SceneCoordinate east{11, 10, 0};
        osrssim::SceneCoordinate north{10, 11, 0};

        osrssim::CollisionProfile eastCollision;
        eastCollision.blocksLineOfSight = true;

        osrssim::CollisionProfile northCollision;
        northCollision.blocksMovement = true;

        assert(scene.PlaceWallObject(
            origin,
            102,
            osrssim::CardinalDirection::East,
            eastCollision,
            osrssim::CardinalDirection::North,
            northCollision));

        assert(actorMovement.CanMove(origin, east));
        assert(actorMovement.CanMove(east, origin));
        assert(!actorMovement.CanMove(origin, north));
        assert(!actorMovement.CanMove(north, origin));
    }

    {
        osrssim::Scene scene;
        osrssim::ActorMovement actorMovement(scene);

        osrssim::SceneCoordinate origin{10, 10, 0};
        osrssim::SceneCoordinate east{11, 10, 0};

        osrssim::CollisionProfile objectCollision;
        objectCollision.blocksMovement = true;

        assert(scene.PlaceGameObject(
            east,
            200,
            osrssim::CardinalDirection::North,
            objectCollision));

        assert(!actorMovement.CanMove(origin, east));
        assert(actorMovement.CanMove(east, origin));

        assert(scene.RemoveGameObject(east));
        assert(actorMovement.CanMove(origin, east));
        assert(actorMovement.CanMove(east, origin));
    }

    {
        osrssim::Scene scene;
        osrssim::ActorMovement actorMovement(scene);

        osrssim::SceneCoordinate origin{10, 10, 0};
        osrssim::SceneCoordinate east{11, 10, 0};

        osrssim::CollisionProfile objectCollision;
        objectCollision.blocksLineOfSight = true;

        assert(scene.PlaceGameObject(
            east,
            201,
            osrssim::CardinalDirection::West,
            objectCollision));

        const osrssim::Tile* tile = scene.TryGetTile(east);

        assert(tile != nullptr);
        assert(tile->HasFlag(osrssim::TileFlag::BlockLineOfSightFull));
        assert(!tile->HasFlag(osrssim::TileFlag::BlockMovementObject));
        assert(actorMovement.CanMove(origin, east));
    }

    {
        osrssim::Scene scene;
        osrssim::ActorMovement actorMovement(scene);

        osrssim::SceneCoordinate origin{10, 10, 0};
        osrssim::SceneCoordinate coveredSouthWest{11, 10, 0};
        osrssim::SceneCoordinate coveredNorthEast{13, 11, 0};

        osrssim::CollisionProfile objectCollision;
        objectCollision.blocksMovement = true;

        assert(scene.PlaceGameObject(
            coveredSouthWest,
            300,
            osrssim::CardinalDirection::East,
            2,
            3,
            objectCollision));

        assert(!actorMovement.CanMove(origin, coveredSouthWest));
        assert(!actorMovement.CanMove({13, 12, 0}, coveredNorthEast));
        assert(actorMovement.CanMove(coveredSouthWest, origin));

        assert(scene.RemoveGameObject(coveredNorthEast));
        assert(actorMovement.CanMove(origin, coveredSouthWest));
        assert(actorMovement.CanMove({13, 12, 0}, coveredNorthEast));
        assert(actorMovement.CanMove(coveredSouthWest, origin));
    }

    {
        osrssim::Scene scene;
        osrssim::ActorMovement actorMovement(scene);

        osrssim::SceneCoordinate origin{10, 10, 0};
        osrssim::SceneCoordinate covered{11, 10, 0};

        osrssim::CollisionProfile objectCollision;
        objectCollision.blocksLineOfSight = true;

        assert(scene.PlaceGameObject(
            covered,
            301,
            osrssim::CardinalDirection::West,
            2,
            3,
            objectCollision));

        const osrssim::Tile* tile = scene.TryGetTile({13, 11, 0});

        assert(tile != nullptr);
        assert(tile->HasFlag(osrssim::TileFlag::BlockLineOfSightFull));
        assert(!tile->HasFlag(osrssim::TileFlag::BlockMovementObject));
        assert(actorMovement.CanMove(origin, covered));
    }

    {
        osrssim::Scene scene;
        osrssim::ActorMovement actorMovement(scene);

        osrssim::SceneCoordinate origin{10, 10, 0};
        osrssim::SceneCoordinate east{11, 10, 0};
        osrssim::SceneCoordinate invalidWall{103, 10, 0};
        osrssim::SceneCoordinate invalidObject{11, 10, 0};

        osrssim::CollisionProfile wallCollision;
        wallCollision.blocksMovement = true;

        osrssim::CollisionProfile objectCollision;
        objectCollision.blocksMovement = true;

        assert(actorMovement.CanMove(origin, east));
        assert(!scene.PlaceWallObject(
            invalidWall,
            400,
            osrssim::CardinalDirection::East,
            wallCollision));
        assert(actorMovement.CanMove(origin, east));

        assert(!scene.PlaceGameObject(
            invalidObject,
            401,
            osrssim::CardinalDirection::North,
            104,
            1,
            objectCollision));
        assert(actorMovement.CanMove(origin, east));
    }

    {
        osrssim::Scene scene;
        osrssim::ActorMovement actorMovement(scene);

        osrssim::SceneCoordinate origin{10, 10, 0};
        osrssim::SceneCoordinate destination{12, 11, 0};

        assert(actorMovement.CanMove(origin, destination, 2, 1));
        assert(!actorMovement.CanMove(origin, destination, 1, 1));
        assert(!actorMovement.CanMove(origin, origin, 2, 1));
    }

    {
        osrssim::Scene scene;
        osrssim::ActorMovement actorMovement(scene);

        osrssim::SceneCoordinate origin{10, 10, 0};
        osrssim::SceneCoordinate destination{12, 11, 0};

        scene.TryGetTile(origin)->AddFlag(osrssim::TileFlag::BlockMovementNorthEast);

        assert(actorMovement.CanMove(origin, destination, 2, 1));
    }

    {
        osrssim::Scene scene;
        osrssim::ActorMovement actorMovement(scene);

        osrssim::SceneCoordinate origin{10, 10, 0};
        osrssim::SceneCoordinate destination{12, 11, 0};

        scene.TryGetTile(origin)->AddFlag(osrssim::TileFlag::BlockMovementEast);

        assert(!actorMovement.CanMove(origin, destination, 2, 1));
    }

    {
        osrssim::Scene scene;
        osrssim::ActorMovement actorMovement(scene);

        osrssim::SceneCoordinate origin{10, 10, 0};
        osrssim::SceneCoordinate east{11, 10, 0};
        osrssim::SceneCoordinate farEast{12, 10, 0};

        osrssim::CollisionProfile wallCollision;
        wallCollision.blocksMovement = true;

        assert(scene.PlaceWallObject(
            {11, 11, 0},
            500,
            osrssim::CardinalDirection::East,
            wallCollision));

        assert(actorMovement.CanMove(origin, east, 1, 1));
        assert(!actorMovement.CanMove(origin, east, 1, 2));
        assert(!actorMovement.CanMove(origin, farEast, 2, 2));
        assert(scene.TryGetTile({11, 11, 0})
                   ->HasFlag(osrssim::TileFlag::BlockMovementEast));
        assert(scene.TryGetTile({12, 11, 0})
                   ->HasFlag(osrssim::TileFlag::BlockMovementWest));
    }

    {
        osrssim::Scene scene;
        osrssim::ActorMovement actorMovement(scene);
        osrssim::SceneCoordinate coordinate{10, 10, 0};

        scene.TryGetTile(coordinate)->AddFlag(osrssim::TileFlag::Occupied);

        osrssim::ActorMovementAccess access(
            scene,
            coordinate,
            osrssim::ActorMovementKind::Player,
            1,
            2);

        const osrssim::ActorMovementResult result =
            actorMovement.MoveByDelta(access, 2, 0);

        assert(result.moved);
        assert(coordinate == (osrssim::SceneCoordinate{12, 10, 0}));
        assert(!scene.TryGetTile({10, 10, 0})->IsOccupied());
        assert(scene.TryGetTile({12, 10, 0})->IsOccupied());
    }

    {
        osrssim::Scene scene;
        osrssim::ActorMovement actorMovement(scene);
        osrssim::SceneCoordinate coordinate{10, 10, 0};

        scene.TryGetTile(coordinate)->AddFlag(osrssim::TileFlag::Occupied);
        scene.TryGetTile({11, 10, 0})
            ->AddFlag(osrssim::TileFlag::BlockMovementObject);

        osrssim::ActorMovementAccess access(
            scene,
            coordinate,
            osrssim::ActorMovementKind::Player,
            1,
            1);

        assert(!actorMovement.MoveByDelta(access, 1, 0).moved);
        assert(!actorMovement.MoveByDelta(access, 0, 0).moved);
        assert(!actorMovement.MoveByDelta(access, 2, 0).moved);
        assert(coordinate == (osrssim::SceneCoordinate{10, 10, 0}));
        assert(scene.TryGetTile({10, 10, 0})->IsOccupied());
        assert(!scene.TryGetTile({11, 10, 0})->IsOccupied());
    }

    {
        osrssim::Scene scene;
        osrssim::ActorMovement actorMovement(scene);
        osrssim::SceneCoordinate coordinate{10, 10, 0};

        scene.TryGetTile(coordinate)->AddFlag(osrssim::TileFlag::Occupied);

        osrssim::ActorMovementAccess access(
            scene,
            coordinate,
            osrssim::ActorMovementKind::Player,
            1,
            1);

        const osrssim::ActorMovementTargetResult result =
            actorMovement.PursueSceneCoordinateTarget(access, {104, 10, 0});

        assert(!result.moved);
        assert(result.clearTarget);
        assert(coordinate == (osrssim::SceneCoordinate{10, 10, 0}));
        assert(scene.TryGetTile({10, 10, 0})->IsOccupied());
    }

    {
        osrssim::Scene scene;
        osrssim::ActorMovement actorMovement(scene);
        osrssim::SceneCoordinate coordinate{10, 10, 0};

        scene.TryGetTile(coordinate)->AddFlag(osrssim::TileFlag::Occupied);
        scene.TryGetTile({12, 10, 0})
            ->AddFlag(osrssim::TileFlag::BlockMovementObject);

        osrssim::ActorMovementAccess access(
            scene,
            coordinate,
            osrssim::ActorMovementKind::Player,
            1,
            2);
        const osrssim::ActorMovementResult result =
            actorMovement.MoveByDelta(access, 2, 0);

        assert(result.moved);
        assert(coordinate == (osrssim::SceneCoordinate{11, 10, 0}));
        assert(!scene.TryGetTile({10, 10, 0})->IsOccupied());
        assert(scene.TryGetTile({11, 10, 0})->IsOccupied());
        assert(!scene.TryGetTile({12, 10, 0})->IsOccupied());
    }

    {
        osrssim::Scene scene;
        osrssim::ActorMovement actorMovement(scene);
        osrssim::SceneCoordinate playerCoordinate{10, 10, 0};
        osrssim::SceneCoordinate npcCoordinate{20, 20, 0};

        scene.TryGetTile(playerCoordinate)->AddFlag(osrssim::TileFlag::Occupied);
        scene.TryGetTile(npcCoordinate)->AddFlag(osrssim::TileFlag::Occupied);
        scene.TryGetTile({11, 10, 0})->AddFlag(osrssim::TileFlag::Occupied);
        scene.TryGetTile({21, 20, 0})->AddFlag(osrssim::TileFlag::Occupied);

        osrssim::ActorMovementAccess playerAccess(
            scene,
            playerCoordinate,
            osrssim::ActorMovementKind::Player,
            1,
            1);
        osrssim::ActorMovementAccess npcAccess(
            scene,
            npcCoordinate,
            osrssim::ActorMovementKind::Npc,
            1,
            1);

        assert(actorMovement.MoveByDelta(playerAccess, 1, 0).moved);
        assert(!actorMovement.MoveByDelta(npcAccess, 1, 0).moved);
        assert(playerCoordinate == (osrssim::SceneCoordinate{11, 10, 0}));
        assert(npcCoordinate == (osrssim::SceneCoordinate{20, 20, 0}));
        assert(scene.TryGetTile({11, 10, 0})->IsOccupied());
        assert(scene.TryGetTile({20, 20, 0})->IsOccupied());
        assert(scene.TryGetTile({21, 20, 0})->IsOccupied());
    }

    {
        osrssim::Scene scene;
        osrssim::ActorMovement actorMovement(scene);
        osrssim::SceneCoordinate coordinate{10, 10, 0};

        scene.TryGetTile(coordinate)->AddFlag(osrssim::TileFlag::Occupied);

        osrssim::ActorMovementAccess access(
            scene,
            coordinate,
            osrssim::ActorMovementKind::Player,
            1,
            2);

        const osrssim::SceneCoordinate target{12, 10, 0};
        const osrssim::ActorMovementTargetResult result =
            actorMovement.PursueSceneCoordinateTarget(access, target);

        assert(result.moved);
        assert(result.clearTarget);
        assert(coordinate == target);
        assert(!scene.TryGetTile({10, 10, 0})->IsOccupied());
        assert(scene.TryGetTile(target)->IsOccupied());
    }

    {
        osrssim::Scene scene;
        osrssim::ActorMovement actorMovement(scene);
        osrssim::SceneCoordinate coordinate{10, 10, 0};

        scene.TryGetTile(coordinate)->AddFlag(osrssim::TileFlag::Occupied);
        scene.TryGetTile({12, 10, 0})
            ->AddFlag(osrssim::TileFlag::BlockMovementObject);

        osrssim::ActorMovementAccess access(
            scene,
            coordinate,
            osrssim::ActorMovementKind::Player,
            1,
            2);

        const osrssim::ActorMovementTargetResult result =
            actorMovement.PursueSceneCoordinateTarget(access, {12, 10, 0});

        assert(result.moved);
        assert(!result.clearTarget);
        assert(coordinate == (osrssim::SceneCoordinate{11, 10, 0}));
        assert(!scene.TryGetTile({10, 10, 0})->IsOccupied());
        assert(scene.TryGetTile({11, 10, 0})->IsOccupied());
        assert(!scene.TryGetTile({12, 10, 0})->IsOccupied());
    }

    {
        osrssim::Scene scene;
        osrssim::ActorMovement actorMovement(scene);
        osrssim::SceneCoordinate coordinate{10, 10, 0};

        scene.TryGetTile(coordinate)->AddFlag(osrssim::TileFlag::Occupied);
        scene.TryGetTile({11, 10, 0})
            ->AddFlag(osrssim::TileFlag::BlockMovementObject);

        osrssim::ActorMovementAccess access(
            scene,
            coordinate,
            osrssim::ActorMovementKind::Player,
            1,
            1);

        const osrssim::ActorMovementTargetResult result =
            actorMovement.PursueSceneCoordinateTarget(access, {12, 10, 0});

        assert(!result.moved);
        assert(result.clearTarget);
        assert(coordinate == (osrssim::SceneCoordinate{10, 10, 0}));
        assert(scene.TryGetTile({10, 10, 0})->IsOccupied());
        assert(!scene.TryGetTile({11, 10, 0})->IsOccupied());
    }

    {
        osrssim::Scene scene;
        osrssim::ActorMovement actorMovement(scene);
        osrssim::SceneCoordinate coordinate{10, 10, 0};

        scene.TryGetTile(coordinate)->AddFlag(osrssim::TileFlag::Occupied);
        scene.TryGetTile({11, 10, 0})->AddFlag(osrssim::TileFlag::Occupied);

        osrssim::ActorMovementAccess access(
            scene,
            coordinate,
            osrssim::ActorMovementKind::Npc,
            1,
            1);

        const osrssim::ActorMovementTargetResult result =
            actorMovement.PursueSceneCoordinateTarget(access, {11, 10, 0});

        assert(!result.moved);
        assert(!result.clearTarget);
        assert(coordinate == (osrssim::SceneCoordinate{10, 10, 0}));
        assert(scene.TryGetTile({10, 10, 0})->IsOccupied());
        assert(scene.TryGetTile({11, 10, 0})->IsOccupied());
    }

    return 0;
}
