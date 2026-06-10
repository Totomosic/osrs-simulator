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

private:
    static bool IsAdjacentStep(int dx, int dy);
    static bool IsDiagonalStep(int dx, int dy);
    static bool IsDestinationBlocked(const Tile& tile);
    static TileFlag GetSourceMovementFlag(int dx, int dy);
    static TileFlag GetDestinationMovementFlag(int dx, int dy);

    bool CanMoveCardinal(SceneCoordinate from, SceneCoordinate to) const;
    bool CanMoveDiagonal(SceneCoordinate from, SceneCoordinate to) const;
};

}  // namespace osrssim
