#include "LineOfSight.h"

namespace osrssim
{

LineOfSight::LineOfSight(const Scene& scene)
    : m_Scene(scene)
{
}

bool LineOfSight::HasLineOfSight(
    SceneCoordinate sourceAnchor,
    int sourceActorSize,
    SceneCoordinate target,
    int range) const
{
    if (sourceActorSize <= 0 || range <= 0 ||
        sourceAnchor.plane != target.plane ||
        !ContainsFootprint(sourceAnchor, sourceActorSize) ||
        !m_Scene.Contains(target))
    {
        return false;
    }

    SceneCoordinate source =
        GetClosestSourceTile(sourceAnchor, sourceActorSize, target);

    if (range == 1 && !IsEdgeAdjacentToFootprint(
                          sourceAnchor,
                          sourceActorSize,
                          target))
    {
        return false;
    }

    return HasLineOfSightFromTile(source, target, range);
}

bool LineOfSight::HasLineOfSight(
    SceneCoordinate sourceAnchor,
    int sourceActorSize,
    SceneCoordinate targetAnchor,
    int targetActorSize,
    int range) const
{
    if (targetActorSize <= 0 || sourceAnchor.plane != targetAnchor.plane ||
        !ContainsFootprint(targetAnchor, targetActorSize))
    {
        return false;
    }

    for (int dx = 0; dx < targetActorSize; ++dx)
    {
        for (int dy = 0; dy < targetActorSize; ++dy)
        {
            SceneCoordinate target{
                targetAnchor.x + dx,
                targetAnchor.y + dy,
                targetAnchor.plane};

            if (HasLineOfSight(sourceAnchor, sourceActorSize, target, range))
            {
                return true;
            }
        }
    }

    return false;
}

int LineOfSight::Abs(int value)
{
    return value < 0 ? -value : value;
}

int LineOfSight::Clamp(int value, int min, int max)
{
    if (value < min)
    {
        return min;
    }

    if (value > max)
    {
        return max;
    }

    return value;
}

int LineOfSight::Sign(int value)
{
    if (value < 0)
    {
        return -1;
    }

    if (value > 0)
    {
        return 1;
    }

    return 0;
}

bool LineOfSight::IsCardinalStep(
    SceneCoordinate from,
    SceneCoordinate to)
{
    const int dx = to.x - from.x;
    const int dy = to.y - from.y;

    return from.plane == to.plane && Abs(dx) + Abs(dy) == 1;
}

bool LineOfSight::IsEdgeAdjacentToFootprint(
    SceneCoordinate sourceAnchor,
    int sourceActorSize,
    SceneCoordinate target)
{
    if (sourceAnchor.plane != target.plane)
    {
        return false;
    }

    const bool overlapsX = target.x >= sourceAnchor.x &&
                           target.x < sourceAnchor.x + sourceActorSize;
    const bool overlapsY = target.y >= sourceAnchor.y &&
                           target.y < sourceAnchor.y + sourceActorSize;
    const bool touchesNorth = target.y == sourceAnchor.y + sourceActorSize;
    const bool touchesSouth = target.y == sourceAnchor.y - 1;
    const bool touchesEast = target.x == sourceAnchor.x + sourceActorSize;
    const bool touchesWest = target.x == sourceAnchor.x - 1;

    return (overlapsX && (touchesNorth || touchesSouth)) ||
           (overlapsY && (touchesEast || touchesWest));
}

TileFlag LineOfSight::GetSourceLineOfSightFlag(int dx, int dy)
{
    if (dx == 0 && dy == 1)
    {
        return TileFlag::BlockLineOfSightNorth;
    }

    if (dx == 1 && dy == 0)
    {
        return TileFlag::BlockLineOfSightEast;
    }

    if (dx == 0 && dy == -1)
    {
        return TileFlag::BlockLineOfSightSouth;
    }

    if (dx == -1 && dy == 0)
    {
        return TileFlag::BlockLineOfSightWest;
    }

    return TileFlag::None;
}

TileFlag LineOfSight::GetDestinationLineOfSightFlag(int dx, int dy)
{
    return GetSourceLineOfSightFlag(-dx, -dy);
}

bool LineOfSight::ContainsFootprint(
    SceneCoordinate anchor,
    int actorSize) const
{
    if (actorSize <= 0)
    {
        return false;
    }

    return m_Scene.Contains(anchor) &&
           m_Scene.Contains(
               {anchor.x + actorSize - 1,
                anchor.y + actorSize - 1,
                anchor.plane});
}

bool LineOfSight::IsLineOfSightFullBlocked(
    SceneCoordinate coordinate) const
{
    const Tile* tile = m_Scene.TryGetTile(coordinate);

    return tile == nullptr || tile->HasFlag(TileFlag::BlockLineOfSightFull);
}

bool LineOfSight::IsLineOfSightStepBlocked(
    SceneCoordinate from,
    SceneCoordinate to) const
{
    if (!IsCardinalStep(from, to))
    {
        return true;
    }

    const Tile* fromTile = m_Scene.TryGetTile(from);
    const Tile* toTile = m_Scene.TryGetTile(to);
    const int dx = to.x - from.x;
    const int dy = to.y - from.y;

    return fromTile == nullptr || toTile == nullptr ||
           fromTile->HasFlag(GetSourceLineOfSightFlag(dx, dy)) ||
           toTile->HasFlag(GetDestinationLineOfSightFlag(dx, dy));
}

bool LineOfSight::HasLineOfSightFromTile(
    SceneCoordinate source,
    SceneCoordinate target,
    int range) const
{
    if (source.plane != target.plane ||
        IsLineOfSightFullBlocked(source) ||
        IsLineOfSightFullBlocked(target))
    {
        return false;
    }

    const int dx = target.x - source.x;
    const int dy = target.y - source.y;
    const int dxAbs = Abs(dx);
    const int dyAbs = Abs(dy);

    if (dxAbs > range || dyAbs > range)
    {
        return false;
    }

    if (dx == 0 && dy == 0)
    {
        return true;
    }

    SceneCoordinate current = source;

    if (dxAbs > dyAbs)
    {
        int xTile = source.x;
        int y = (source.y << 16) + 0x8000;
        const int slope = (dy * 65536) / dxAbs;
        const int xIncrement = Sign(dx);

        if (dy < 0)
        {
            y -= 1;
        }

        while (xTile != target.x)
        {
            xTile += xIncrement;

            SceneCoordinate next{xTile, y >> 16, source.plane};

            if (!TryStepLineOfSight(current, next))
            {
                return false;
            }

            y += slope;
            SceneCoordinate verticalNext{xTile, y >> 16, source.plane};

            if (verticalNext.y != next.y &&
                !TryStepLineOfSight(current, verticalNext))
            {
                return false;
            }
        }
    }
    else
    {
        int yTile = source.y;
        int x = (source.x << 16) + 0x8000;
        const int slope = (dx * 65536) / dyAbs;
        const int yIncrement = Sign(dy);

        if (dx < 0)
        {
            x -= 1;
        }

        while (yTile != target.y)
        {
            yTile += yIncrement;

            SceneCoordinate next{x >> 16, yTile, source.plane};

            if (!TryStepLineOfSight(current, next))
            {
                return false;
            }

            x += slope;
            SceneCoordinate horizontalNext{x >> 16, yTile, source.plane};

            if (horizontalNext.x != next.x &&
                !TryStepLineOfSight(current, horizontalNext))
            {
                return false;
            }
        }
    }

    return true;
}

bool LineOfSight::TryStepLineOfSight(
    SceneCoordinate& current,
    SceneCoordinate next) const
{
    if (IsLineOfSightStepBlocked(current, next) ||
        IsLineOfSightFullBlocked(next))
    {
        return false;
    }

    current = next;
    return true;
}

SceneCoordinate LineOfSight::GetClosestSourceTile(
    SceneCoordinate sourceAnchor,
    int sourceActorSize,
    SceneCoordinate target) const
{
    return {
        Clamp(
            target.x,
            sourceAnchor.x,
            sourceAnchor.x + sourceActorSize - 1),
        Clamp(
            target.y,
            sourceAnchor.y,
            sourceAnchor.y + sourceActorSize - 1),
        sourceAnchor.plane};
}

}  // namespace osrssim
