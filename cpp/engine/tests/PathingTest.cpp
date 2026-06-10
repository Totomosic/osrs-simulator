#include "Pathing.h"
#include "Scene.h"
#include "Tile.h"

#include <cassert>

int main()
{
    {
        osrssim::Scene scene;
        osrssim::Pathing pathing(scene);

        osrssim::SceneCoordinate origin{10, 10, 0};
        osrssim::SceneCoordinate east{11, 10, 0};
        osrssim::SceneCoordinate north{10, 11, 0};
        osrssim::SceneCoordinate northEast{11, 11, 0};
        osrssim::SceneCoordinate same{10, 10, 0};
        osrssim::SceneCoordinate far{12, 10, 0};
        osrssim::SceneCoordinate otherPlane{11, 10, 1};
        osrssim::SceneCoordinate outside{104, 10, 0};

        assert(pathing.CanMove(origin, east));
        assert(pathing.CanMove(origin, north));
        assert(pathing.CanMove(origin, northEast));
        assert(!pathing.CanMove(origin, same));
        assert(!pathing.CanMove(origin, far));
        assert(!pathing.CanMove(origin, otherPlane));
        assert(!pathing.CanMove(origin, outside));
    }

    {
        osrssim::Scene scene;
        osrssim::Pathing pathing(scene);

        osrssim::SceneCoordinate origin{10, 10, 0};
        osrssim::SceneCoordinate east{11, 10, 0};

        scene.TryGetTile(origin)->AddFlag(osrssim::TileFlag::BlockMovementEast);
        assert(!pathing.CanMove(origin, east));

        scene.TryGetTile(origin)->RemoveFlag(osrssim::TileFlag::BlockMovementEast);
        scene.TryGetTile(east)->AddFlag(osrssim::TileFlag::BlockMovementWest);
        assert(!pathing.CanMove(origin, east));
    }

    {
        osrssim::Scene scene;
        osrssim::Pathing pathing(scene);

        osrssim::SceneCoordinate origin{10, 10, 0};
        osrssim::SceneCoordinate east{11, 10, 0};

        scene.TryGetTile(east)->AddFlag(osrssim::TileFlag::Occupied);
        assert(!pathing.CanMove(origin, east));
    }

    {
        osrssim::Scene scene;
        osrssim::Pathing pathing(scene);

        osrssim::SceneCoordinate origin{10, 10, 0};
        osrssim::SceneCoordinate east{11, 10, 0};

        scene.TryGetTile(east)->AddFlag(osrssim::TileFlag::BlockLineOfSightFull);
        assert(pathing.CanMove(origin, east));
    }

    {
        osrssim::Scene scene;
        osrssim::Pathing pathing(scene);

        osrssim::SceneCoordinate origin{10, 10, 0};
        osrssim::SceneCoordinate east{11, 10, 0};
        osrssim::SceneCoordinate north{10, 11, 0};
        osrssim::SceneCoordinate northEast{11, 11, 0};

        scene.TryGetTile(origin)->AddFlag(osrssim::TileFlag::BlockMovementNorthEast);
        assert(!pathing.CanMove(origin, northEast));

        scene.TryGetTile(origin)->RemoveFlag(osrssim::TileFlag::BlockMovementNorthEast);
        scene.TryGetTile(northEast)->AddFlag(osrssim::TileFlag::BlockMovementSouthWest);
        assert(!pathing.CanMove(origin, northEast));

        scene.TryGetTile(northEast)->RemoveFlag(osrssim::TileFlag::BlockMovementSouthWest);
        scene.TryGetTile(east)->AddFlag(osrssim::TileFlag::Occupied);
        assert(!pathing.CanMove(origin, northEast));

        scene.TryGetTile(east)->RemoveFlag(osrssim::TileFlag::Occupied);
        scene.TryGetTile(north)->AddFlag(osrssim::TileFlag::BlockMovementFull);
        assert(!pathing.CanMove(origin, northEast));
    }

    {
        osrssim::Scene scene;
        osrssim::Pathing pathing(scene);

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

        assert(!pathing.CanMove(origin, east));
        assert(!pathing.CanMove(east, origin));
        assert(pathing.CanMove(origin, north));
        assert(pathing.CanMove(origin, south));
    }

    {
        osrssim::Scene scene;
        osrssim::Pathing pathing(scene);

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

        assert(!pathing.CanMove(origin, east));
        assert(!pathing.CanMove(east, origin));
        assert(!pathing.CanMove(origin, north));
        assert(!pathing.CanMove(north, origin));
        assert(pathing.CanMove(origin, south));
    }

    {
        osrssim::Scene scene;
        osrssim::Pathing pathing(scene);

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

        assert(pathing.CanMove(origin, east));
        assert(pathing.CanMove(east, origin));
        assert(!pathing.CanMove(origin, north));
        assert(!pathing.CanMove(north, origin));
    }

    {
        osrssim::Scene scene;
        osrssim::Pathing pathing(scene);

        osrssim::SceneCoordinate origin{10, 10, 0};
        osrssim::SceneCoordinate east{11, 10, 0};

        osrssim::CollisionProfile objectCollision;
        objectCollision.blocksMovement = true;

        assert(scene.PlaceGameObject(
            east,
            200,
            osrssim::CardinalDirection::North,
            objectCollision));

        assert(!pathing.CanMove(origin, east));
        assert(pathing.CanMove(east, origin));
    }

    {
        osrssim::Scene scene;
        osrssim::Pathing pathing(scene);

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
        assert(pathing.CanMove(origin, east));
    }

    {
        osrssim::Scene scene;
        osrssim::Pathing pathing(scene);

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

        assert(!pathing.CanMove(origin, coveredSouthWest));
        assert(!pathing.CanMove({13, 12, 0}, coveredNorthEast));
        assert(pathing.CanMove(coveredSouthWest, origin));
    }

    {
        osrssim::Scene scene;
        osrssim::Pathing pathing(scene);

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
        assert(pathing.CanMove(origin, covered));
    }

    return 0;
}
