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
    assert(wallTile->wallObject->direction == osrssim::CardinalDirection::East);
    assert(wallEastTile->wallObject == std::nullopt);
    assert(wallTile->HasFlag(osrssim::TileFlag::BlockMovementEast));
    assert(wallEastTile->HasFlag(osrssim::TileFlag::BlockMovementWest));

    osrssim::Engine engine;
    assert(engine.GetScene().Contains({0, 0, 0}));

    return 0;
}
