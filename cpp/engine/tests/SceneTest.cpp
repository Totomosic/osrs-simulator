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

    osrssim::Engine engine;
    assert(engine.GetScene().Contains({0, 0, 0}));

    return 0;
}
