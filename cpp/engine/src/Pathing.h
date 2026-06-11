#pragma once

#include "Scene.h"
#include "Types.h"

namespace osrssim
{

class Pathing
{
private:
    const Scene& m_Scene;

public:
    explicit Pathing(const Scene& scene);

    bool CanMove(SceneCoordinate from, SceneCoordinate to) const;
    bool CanMove(
        SceneCoordinate from,
        SceneCoordinate to,
        int actorSpeed,
        int actorSize) const;

private:
    static bool IsAdjacentStep(int dx, int dy);
    static bool IsDiagonalStep(int dx, int dy);
    static bool IsDestinationBlocked(const Tile& tile);
    static int Sign(int value);
    static TileFlag GetSourceMovementFlag(int dx, int dy);
    static TileFlag GetDestinationMovementFlag(int dx, int dy);

    bool CanStand(SceneCoordinate anchor, int actorSize) const;
    bool CanMoveFootprintStep(
        SceneCoordinate from,
        SceneCoordinate to,
        int actorSize) const;
    bool CanMoveFootprintCardinal(
        SceneCoordinate from,
        SceneCoordinate to,
        int actorSize) const;
    bool CanMoveFootprintDiagonal(
        SceneCoordinate from,
        SceneCoordinate to,
        int actorSize) const;
    bool CanMoveMonotonicRoute(
        SceneCoordinate current,
        SceneCoordinate to,
        int remainingSteps,
        int actorSize) const;
    bool CanMoveCardinal(SceneCoordinate from, SceneCoordinate to) const;
    bool CanMoveDiagonal(SceneCoordinate from, SceneCoordinate to) const;
};

}  // namespace osrssim
