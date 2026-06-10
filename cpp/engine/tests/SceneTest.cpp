#include "Engine.h"
#include "Scene.h"
#include "Tile.h"

#include <cassert>

int main()
{
    osrssim::Scene scene(3200, 3200);

    assert(scene.GetBaseX() == 3200);
    assert(scene.GetBaseY() == 3200);
    assert(scene.Contains({0, 0, 0}));
    assert(scene.Contains({103, 103, 3}));
    assert(!scene.Contains({104, 0, 0}));
    assert(!scene.Contains({0, 104, 0}));
    assert(!scene.Contains({0, 0, 4}));

    osrssim::Tile* tile = scene.TryGetTile({12, 34, 2});
    assert(tile != nullptr);
    osrssim::SceneCoordinate expectedCoordinate{12, 34, 2};
    assert(tile->coordinate == expectedCoordinate);
    assert(!tile->IsOccupied());

    tile->AddFlag(osrssim::TileFlag::Occupied);
    tile->AddFlag(osrssim::TileFlag::BlockMovementWest);
    tile->AddFlag(osrssim::TileFlag::BlockLineOfSightWest);

    assert(tile->IsOccupied());
    assert(tile->HasFlag(osrssim::TileFlag::BlockMovementWest));
    assert(tile->HasFlag(osrssim::TileFlag::BlockLineOfSightWest));

    tile->RemoveFlag(osrssim::TileFlag::Occupied);
    assert(!tile->IsOccupied());

    assert(scene.TryGetTile({104, 0, 0}) == nullptr);

    osrssim::CollisionProfile wallCollision;
    wallCollision.blocksMovement = true;

    osrssim::SceneCoordinate wallCoordinate{20, 20, 0};
    osrssim::SceneCoordinate wallEast{21, 20, 0};

    assert(scene.PlaceWallObject(
        wallCoordinate,
        42,
        osrssim::CardinalDirection::East,
        wallCollision));

    const osrssim::Tile* wallTile = scene.TryGetTile(wallCoordinate);
    const osrssim::Tile* wallEastTile = scene.TryGetTile(wallEast);

    assert(wallTile != nullptr);
    assert(wallEastTile != nullptr);
    assert(wallTile->wallObject.has_value());
    assert(wallTile->wallObject->id == 42);
    assert(wallTile->wallObject->directionCount == 1);
    assert(wallTile->wallObject->HasDirection(osrssim::CardinalDirection::East));
    assert(wallEastTile->wallObject == std::nullopt);
    assert(wallTile->HasFlag(osrssim::TileFlag::BlockMovementEast));
    assert(wallEastTile->HasFlag(osrssim::TileFlag::BlockMovementWest));

    osrssim::CollisionProfile movementCollision;
    movementCollision.blocksMovement = true;

    osrssim::CollisionProfile lineOfSightCollision;
    lineOfSightCollision.blocksLineOfSight = true;

    osrssim::SceneCoordinate cornerCoordinate{30, 30, 0};
    osrssim::SceneCoordinate cornerEast{31, 30, 0};
    osrssim::SceneCoordinate cornerNorth{30, 31, 0};

    assert(scene.PlaceWallObject(
        cornerCoordinate,
        43,
        osrssim::CardinalDirection::East,
        movementCollision,
        osrssim::CardinalDirection::North,
        lineOfSightCollision));

    const osrssim::Tile* cornerTile = scene.TryGetTile(cornerCoordinate);
    const osrssim::Tile* cornerEastTile = scene.TryGetTile(cornerEast);
    const osrssim::Tile* cornerNorthTile = scene.TryGetTile(cornerNorth);

    assert(cornerTile != nullptr);
    assert(cornerEastTile != nullptr);
    assert(cornerNorthTile != nullptr);
    assert(cornerTile->wallObject.has_value());
    assert(cornerTile->wallObject->id == 43);
    assert(cornerTile->wallObject->directionCount == 2);
    assert(cornerTile->wallObject->HasDirection(osrssim::CardinalDirection::East));
    assert(cornerTile->wallObject->HasDirection(osrssim::CardinalDirection::North));
    assert(cornerEastTile->wallObject == std::nullopt);
    assert(cornerNorthTile->wallObject == std::nullopt);
    assert(cornerTile->HasFlag(osrssim::TileFlag::BlockMovementEast));
    assert(cornerEastTile->HasFlag(osrssim::TileFlag::BlockMovementWest));
    assert(!cornerTile->HasFlag(osrssim::TileFlag::BlockMovementNorth));
    assert(!cornerNorthTile->HasFlag(osrssim::TileFlag::BlockMovementSouth));
    assert(cornerTile->HasFlag(osrssim::TileFlag::BlockLineOfSightNorth));
    assert(cornerNorthTile->HasFlag(osrssim::TileFlag::BlockLineOfSightSouth));
    assert(!cornerTile->HasFlag(osrssim::TileFlag::BlockLineOfSightEast));
    assert(!cornerEastTile->HasFlag(osrssim::TileFlag::BlockLineOfSightWest));

    assert(!scene.PlaceWallObject(
        {40, 40, 0},
        44,
        osrssim::CardinalDirection::East,
        movementCollision,
        osrssim::CardinalDirection::East,
        lineOfSightCollision));

    osrssim::CollisionProfile gameObjectCollision;
    gameObjectCollision.blocksMovement = true;
    gameObjectCollision.blocksLineOfSight = true;

    osrssim::SceneCoordinate gameObjectCoordinate{50, 50, 0};

    assert(scene.PlaceGameObject(
        gameObjectCoordinate,
        200,
        osrssim::CardinalDirection::South,
        gameObjectCollision));

    const osrssim::Tile* gameObjectTile = scene.TryGetTile(gameObjectCoordinate);

    assert(gameObjectTile != nullptr);
    assert(gameObjectTile->gameObject.has_value());
    assert(gameObjectTile->gameObject->id == 200);
    assert(gameObjectTile->gameObject->direction == osrssim::CardinalDirection::South);
    assert(gameObjectTile->gameObject->sizeX == 1);
    assert(gameObjectTile->gameObject->sizeY == 1);
    assert(gameObjectTile->HasFlag(osrssim::TileFlag::BlockMovementObject));
    assert(gameObjectTile->HasFlag(osrssim::TileFlag::BlockLineOfSightFull));
    assert(!gameObjectTile->HasFlag(osrssim::TileFlag::Occupied));
    assert(!scene.PlaceGameObject(
        gameObjectCoordinate,
        201,
        osrssim::CardinalDirection::North,
        gameObjectCollision));

    osrssim::SceneCoordinate largeObjectCoordinate{60, 60, 0};

    assert(scene.PlaceGameObject(
        largeObjectCoordinate,
        300,
        osrssim::CardinalDirection::East,
        2,
        3,
        gameObjectCollision));

    for (int dx = 0; dx < 3; ++dx)
    {
        for (int dy = 0; dy < 2; ++dy)
        {
            const osrssim::Tile* coveredTile = scene.TryGetTile(
                {largeObjectCoordinate.x + dx, largeObjectCoordinate.y + dy, 0});

            assert(coveredTile != nullptr);
            assert(coveredTile->gameObject.has_value());
            assert(coveredTile->gameObject->id == 300);
            assert(
                coveredTile->gameObject->direction ==
                osrssim::CardinalDirection::East);
            assert(coveredTile->gameObject->sizeX == 2);
            assert(coveredTile->gameObject->sizeY == 3);
            assert(coveredTile->HasFlag(osrssim::TileFlag::BlockMovementObject));
            assert(coveredTile->HasFlag(osrssim::TileFlag::BlockLineOfSightFull));
        }
    }

    const osrssim::Tile* outsideRotatedFootprint = scene.TryGetTile({62, 62, 0});
    assert(outsideRotatedFootprint != nullptr);
    assert(outsideRotatedFootprint->gameObject == std::nullopt);

    assert(scene.RemoveWallObject(wallCoordinate));
    assert(!wallTile->wallObject.has_value());
    assert(!wallTile->HasFlag(osrssim::TileFlag::BlockMovementEast));
    assert(!wallEastTile->HasFlag(osrssim::TileFlag::BlockMovementWest));
    assert(!scene.RemoveWallObject(wallCoordinate));

    assert(scene.RemoveWallObject(cornerCoordinate));
    assert(!cornerTile->wallObject.has_value());
    assert(!cornerTile->HasFlag(osrssim::TileFlag::BlockMovementEast));
    assert(!cornerEastTile->HasFlag(osrssim::TileFlag::BlockMovementWest));
    assert(!cornerTile->HasFlag(osrssim::TileFlag::BlockLineOfSightNorth));
    assert(!cornerNorthTile->HasFlag(osrssim::TileFlag::BlockLineOfSightSouth));

    assert(scene.RemoveGameObject(gameObjectCoordinate));
    assert(!gameObjectTile->gameObject.has_value());
    assert(!gameObjectTile->HasFlag(osrssim::TileFlag::BlockMovementObject));
    assert(!gameObjectTile->HasFlag(osrssim::TileFlag::BlockLineOfSightFull));
    assert(!scene.RemoveGameObject(gameObjectCoordinate));

    assert(scene.RemoveGameObject({61, 61, 0}));

    for (int dx = 0; dx < 3; ++dx)
    {
        for (int dy = 0; dy < 2; ++dy)
        {
            const osrssim::Tile* coveredTile = scene.TryGetTile(
                {largeObjectCoordinate.x + dx, largeObjectCoordinate.y + dy, 0});

            assert(coveredTile != nullptr);
            assert(coveredTile->gameObject == std::nullopt);
            assert(!coveredTile->HasFlag(osrssim::TileFlag::BlockMovementObject));
            assert(!coveredTile->HasFlag(osrssim::TileFlag::BlockLineOfSightFull));
        }
    }

    osrssim::Scene overlapScene;
    assert(overlapScene.PlaceWallObject(
        {10, 10, 0},
        400,
        osrssim::CardinalDirection::East,
        movementCollision));
    assert(!overlapScene.PlaceWallObject(
        {11, 10, 0},
        401,
        osrssim::CardinalDirection::West,
        movementCollision));
    assert(overlapScene.TryGetTile({11, 10, 0})->wallObject == std::nullopt);
    assert(overlapScene.TryGetTile({10, 10, 0})
               ->HasFlag(osrssim::TileFlag::BlockMovementEast));
    assert(overlapScene.TryGetTile({11, 10, 0})
               ->HasFlag(osrssim::TileFlag::BlockMovementWest));

    osrssim::Scene objectOverlapScene;
    assert(objectOverlapScene.PlaceGameObject(
        {20, 20, 0},
        500,
        osrssim::CardinalDirection::North,
        gameObjectCollision));
    assert(!objectOverlapScene.PlaceGameObject(
        {20, 20, 0},
        501,
        osrssim::CardinalDirection::North,
        gameObjectCollision));
    assert(objectOverlapScene.TryGetTile({20, 20, 0})->gameObject->id == 500);
    assert(objectOverlapScene.TryGetTile({20, 20, 0})
               ->HasFlag(osrssim::TileFlag::BlockMovementObject));
    assert(objectOverlapScene.TryGetTile({20, 20, 0})
               ->HasFlag(osrssim::TileFlag::BlockLineOfSightFull));

    osrssim::Scene invalidWallScene;
    osrssim::SceneCoordinate invalidWallCoordinate{103, 50, 0};
    const osrssim::Tile* invalidWallTile =
        invalidWallScene.TryGetTile(invalidWallCoordinate);

    assert(invalidWallTile != nullptr);
    assert(!invalidWallScene.PlaceWallObject(
        invalidWallCoordinate,
        600,
        osrssim::CardinalDirection::East,
        movementCollision));
    assert(invalidWallTile->wallObject == std::nullopt);
    assert(!invalidWallTile->HasFlag(osrssim::TileFlag::BlockMovementEast));

    osrssim::Scene invalidCornerWallScene;
    osrssim::SceneCoordinate invalidCornerWallCoordinate{10, 103, 0};
    osrssim::SceneCoordinate invalidCornerWallEast{11, 103, 0};
    const osrssim::Tile* invalidCornerWallTile =
        invalidCornerWallScene.TryGetTile(invalidCornerWallCoordinate);
    const osrssim::Tile* invalidCornerWallEastTile =
        invalidCornerWallScene.TryGetTile(invalidCornerWallEast);

    assert(invalidCornerWallTile != nullptr);
    assert(invalidCornerWallEastTile != nullptr);
    assert(!invalidCornerWallScene.PlaceWallObject(
        invalidCornerWallCoordinate,
        601,
        osrssim::CardinalDirection::East,
        movementCollision,
        osrssim::CardinalDirection::North,
        lineOfSightCollision));
    assert(invalidCornerWallTile->wallObject == std::nullopt);
    assert(!invalidCornerWallTile->HasFlag(osrssim::TileFlag::BlockMovementEast));
    assert(!invalidCornerWallEastTile->HasFlag(osrssim::TileFlag::BlockMovementWest));
    assert(!invalidCornerWallTile->HasFlag(osrssim::TileFlag::BlockLineOfSightNorth));

    osrssim::Scene invalidGameObjectScene;
    osrssim::SceneCoordinate invalidGameObjectCoordinate{103, 103, 0};
    const osrssim::Tile* invalidGameObjectTile =
        invalidGameObjectScene.TryGetTile(invalidGameObjectCoordinate);

    assert(invalidGameObjectTile != nullptr);
    assert(!invalidGameObjectScene.PlaceGameObject(
        invalidGameObjectCoordinate,
        700,
        osrssim::CardinalDirection::North,
        2,
        1,
        gameObjectCollision));
    assert(invalidGameObjectTile->gameObject == std::nullopt);
    assert(!invalidGameObjectTile->HasFlag(osrssim::TileFlag::BlockMovementObject));
    assert(!invalidGameObjectTile->HasFlag(osrssim::TileFlag::BlockLineOfSightFull));

    osrssim::Scene invalidCoverageScene;
    osrssim::SceneCoordinate invalidCoverageCoordinate{101, 101, 0};

    assert(!invalidCoverageScene.PlaceGameObject(
        invalidCoverageCoordinate,
        701,
        osrssim::CardinalDirection::North,
        4,
        4,
        gameObjectCollision));

    for (int dx = 0; dx < 3; ++dx)
    {
        for (int dy = 0; dy < 3; ++dy)
        {
            const osrssim::Tile* coveredTile = invalidCoverageScene.TryGetTile(
                {invalidCoverageCoordinate.x + dx,
                 invalidCoverageCoordinate.y + dy,
                 invalidCoverageCoordinate.plane});

            assert(coveredTile != nullptr);
            assert(coveredTile->gameObject == std::nullopt);
            assert(!coveredTile->HasFlag(osrssim::TileFlag::BlockMovementObject));
            assert(!coveredTile->HasFlag(osrssim::TileFlag::BlockLineOfSightFull));
        }
    }

    osrssim::Scene placementConflictScene;
    osrssim::SceneCoordinate conflictOrigin{70, 70, 0};
    osrssim::SceneCoordinate conflictTileCoordinate{71, 70, 0};
    osrssim::Tile* conflictTile =
        placementConflictScene.TryGetTile(conflictTileCoordinate);

    assert(conflictTile != nullptr);
    conflictTile->AddFlag(osrssim::TileFlag::BlockMovementObject);
    assert(!placementConflictScene.PlaceGameObject(
        conflictOrigin,
        800,
        osrssim::CardinalDirection::North,
        2,
        1,
        gameObjectCollision));

    const osrssim::Tile* conflictOriginTile =
        placementConflictScene.TryGetTile(conflictOrigin);

    assert(conflictOriginTile != nullptr);
    assert(conflictOriginTile->gameObject == std::nullopt);
    assert(!conflictOriginTile->HasFlag(osrssim::TileFlag::BlockMovementObject));
    assert(!conflictOriginTile->HasFlag(osrssim::TileFlag::BlockLineOfSightFull));
    assert(conflictTile->gameObject == std::nullopt);
    assert(conflictTile->HasFlag(osrssim::TileFlag::BlockMovementObject));
    assert(!conflictTile->HasFlag(osrssim::TileFlag::BlockLineOfSightFull));

    osrssim::Engine engine;
    assert(engine.GetScene().Contains({0, 0, 0}));

    return 0;
}
