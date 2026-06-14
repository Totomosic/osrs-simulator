#pragma once

#include "Scene.h"
#include "Types.h"

namespace osrssim
{

class LineOfSight
{
private:
    const Scene& m_Scene;

public:
    explicit LineOfSight(const Scene& scene);

    bool HasLineOfSight(
        SceneCoordinate sourceAnchor,
        int sourceActorSize,
        SceneCoordinate target,
        int range) const;
    bool HasLineOfSight(
        SceneCoordinate sourceAnchor,
        int sourceActorSize,
        SceneCoordinate targetAnchor,
        int targetActorSize,
        int range) const;

private:
    static int Abs(int value);
    static int Clamp(int value, int min, int max);
    static int Sign(int value);
    static bool IsCardinalStep(SceneCoordinate from, SceneCoordinate to);
    static bool IsEdgeAdjacentToFootprint(
        SceneCoordinate sourceAnchor,
        int sourceActorSize,
        SceneCoordinate target);
    static TileFlag GetSourceLineOfSightFlag(int dx, int dy);
    static TileFlag GetDestinationLineOfSightFlag(int dx, int dy);

    bool ContainsFootprint(SceneCoordinate anchor, int actorSize) const;
    bool IsLineOfSightFullBlocked(SceneCoordinate coordinate) const;
    bool IsLineOfSightStepBlocked(
        SceneCoordinate from,
        SceneCoordinate to) const;
    bool HasLineOfSightFromTile(
        SceneCoordinate source,
        SceneCoordinate target,
        int range) const;
    bool TryStepLineOfSight(
        SceneCoordinate& current,
        SceneCoordinate next) const;
    SceneCoordinate GetClosestSourceTile(
        SceneCoordinate sourceAnchor,
        int sourceActorSize,
        SceneCoordinate target) const;
};

}  // namespace osrssim
