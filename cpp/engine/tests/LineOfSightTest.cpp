#include "LineOfSight.h"
#include "Scene.h"
#include "Tile.h"

#include <cassert>

int main()
{
    {
        osrssim::Scene scene;
        osrssim::LineOfSight lineOfSight(scene);

        assert(lineOfSight.HasLineOfSight({10, 10, 0}, 1, {15, 12, 0}, 5));
        assert(!lineOfSight.HasLineOfSight({10, 10, 0}, 1, {16, 12, 0}, 5));
        assert(!lineOfSight.HasLineOfSight({10, 10, 0}, 1, {15, 12, 1}, 5));
        assert(!lineOfSight.HasLineOfSight({10, 10, 0}, 1, {104, 12, 0}, 5));
        assert(!lineOfSight.HasLineOfSight({10, 10, 0}, 0, {15, 12, 0}, 5));
        assert(!lineOfSight.HasLineOfSight({10, 10, 0}, 1, {15, 12, 0}, 0));
    }

    {
        osrssim::Scene scene;
        osrssim::LineOfSight lineOfSight(scene);

        scene.TryGetTile({10, 10, 0})
            ->AddFlag(osrssim::TileFlag::BlockLineOfSightFull);
        assert(!lineOfSight.HasLineOfSight({10, 10, 0}, 1, {15, 10, 0}, 5));

        scene.TryGetTile({10, 10, 0})
            ->RemoveFlag(osrssim::TileFlag::BlockLineOfSightFull);
        scene.TryGetTile({15, 10, 0})
            ->AddFlag(osrssim::TileFlag::BlockLineOfSightFull);
        assert(!lineOfSight.HasLineOfSight({10, 10, 0}, 1, {15, 10, 0}, 5));

        scene.TryGetTile({15, 10, 0})
            ->RemoveFlag(osrssim::TileFlag::BlockLineOfSightFull);
        scene.TryGetTile({12, 10, 0})
            ->AddFlag(osrssim::TileFlag::BlockLineOfSightFull);
        assert(!lineOfSight.HasLineOfSight({10, 10, 0}, 1, {15, 10, 0}, 5));
    }

    {
        osrssim::Scene scene;
        osrssim::LineOfSight lineOfSight(scene);

        scene.TryGetTile({11, 10, 0})
            ->AddFlag(osrssim::TileFlag::BlockMovementFull);
        assert(lineOfSight.HasLineOfSight({10, 10, 0}, 1, {12, 10, 0}, 2));

        scene.TryGetTile({10, 10, 0})
            ->AddFlag(osrssim::TileFlag::BlockLineOfSightEast);
        assert(!lineOfSight.HasLineOfSight({10, 10, 0}, 1, {12, 10, 0}, 2));

        scene.TryGetTile({10, 10, 0})
            ->RemoveFlag(osrssim::TileFlag::BlockLineOfSightEast);
        scene.TryGetTile({11, 10, 0})
            ->AddFlag(osrssim::TileFlag::BlockLineOfSightWest);
        assert(!lineOfSight.HasLineOfSight({10, 10, 0}, 1, {12, 10, 0}, 2));
    }

    {
        osrssim::Scene scene;
        osrssim::LineOfSight lineOfSight(scene);

        osrssim::CollisionProfile lineOfSightWall;
        lineOfSightWall.blocksLineOfSight = true;

        assert(scene.PlaceWallObject(
            {10, 10, 0},
            100,
            osrssim::CardinalDirection::East,
            lineOfSightWall));
        assert(!lineOfSight.HasLineOfSight({10, 10, 0}, 1, {11, 10, 0}, 1));
        assert(!lineOfSight.HasLineOfSight({11, 10, 0}, 1, {10, 10, 0}, 1));

        assert(scene.RemoveWallObject({10, 10, 0}));

        osrssim::CollisionProfile movementWall;
        movementWall.blocksMovement = true;

        assert(scene.PlaceWallObject(
            {10, 10, 0},
            101,
            osrssim::CardinalDirection::East,
            movementWall));
        assert(lineOfSight.HasLineOfSight({10, 10, 0}, 1, {11, 10, 0}, 1));
    }

    {
        osrssim::Scene scene;
        osrssim::LineOfSight lineOfSight(scene);

        assert(lineOfSight.HasLineOfSight({10, 10, 0}, 3, {13, 11, 0}, 1));
        assert(lineOfSight.HasLineOfSight({10, 10, 0}, 3, {11, 13, 0}, 1));
        assert(!lineOfSight.HasLineOfSight({10, 10, 0}, 3, {13, 13, 0}, 1));
        assert(lineOfSight.HasLineOfSight({10, 10, 0}, 3, {15, 11, 0}, 3));
        assert(!lineOfSight.HasLineOfSight({10, 10, 0}, 3, {15, 11, 0}, 2));
    }

    {
        osrssim::Scene scene;
        osrssim::LineOfSight lineOfSight(scene);

        scene.TryGetTile({12, 10, 0})
            ->AddFlag(osrssim::TileFlag::BlockLineOfSightFull);
        assert(lineOfSight.HasLineOfSight(
            {10, 10, 0},
            1,
            {12, 10, 0},
            2,
            5));

        scene.TryGetTile({12, 11, 0})
            ->AddFlag(osrssim::TileFlag::BlockLineOfSightFull);
        scene.TryGetTile({13, 10, 0})
            ->AddFlag(osrssim::TileFlag::BlockLineOfSightFull);
        scene.TryGetTile({13, 11, 0})
            ->AddFlag(osrssim::TileFlag::BlockLineOfSightFull);
        assert(!lineOfSight.HasLineOfSight(
            {10, 10, 0},
            1,
            {12, 10, 0},
            2,
            5));
    }

    {
        osrssim::Scene scene;
        osrssim::LineOfSight lineOfSight(scene);

        assert(lineOfSight.HasLineOfSight(
            {10, 10, 0},
            2,
            {12, 10, 0},
            2,
            1));
        assert(!lineOfSight.HasLineOfSight(
            {10, 10, 0},
            2,
            {12, 12, 0},
            2,
            1));
    }
}
