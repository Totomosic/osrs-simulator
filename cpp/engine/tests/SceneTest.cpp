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

    osrssim::Engine engine;
    assert(engine.GetScene().Contains({0, 0, 0}));

    return 0;
}
