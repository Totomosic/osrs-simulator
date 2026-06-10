#include "Scene.h"

namespace osrssim
{

Scene::Scene(int baseX, int baseY)
    : m_BaseX(baseX),
      m_BaseY(baseY)
{
    for (int plane = 0; plane < PlaneCount; ++plane)
    {
        for (int x = 0; x < Width; ++x)
        {
            for (int y = 0; y < Height; ++y)
            {
                SceneCoordinate coordinate{x, y, plane};
                m_Tiles[GetIndex(coordinate)].coordinate = coordinate;
            }
        }
    }
}

int Scene::GetBaseX() const
{
    return m_BaseX;
}

int Scene::GetBaseY() const
{
    return m_BaseY;
}

bool Scene::Contains(SceneCoordinate coordinate) const
{
    return coordinate.plane >= 0 && coordinate.plane < PlaneCount &&
           coordinate.x >= 0 && coordinate.x < Width &&
           coordinate.y >= 0 && coordinate.y < Height;
}

bool Scene::PlaceWallObject(
    SceneCoordinate coordinate,
    EntityId id,
    CardinalDirection direction,
    const CollisionProfile& collisionProfile)
{
    SceneCoordinate adjacentCoordinate = GetAdjacentCoordinate(coordinate, direction);
    Tile* tile = TryGetTile(coordinate);
    Tile* adjacentTile = TryGetTile(adjacentCoordinate);

    if (tile == nullptr || adjacentTile == nullptr || tile->wallObject.has_value())
    {
        return false;
    }

    WallObject wallObject;
    wallObject.id = id;
    wallObject.directions[0] = direction;
    wallObject.directionCount = 1;
    tile->wallObject = wallObject;

    ApplyWallEdgeCollision(*tile, *adjacentTile, direction, collisionProfile);

    return true;
}

bool Scene::PlaceWallObject(
    SceneCoordinate coordinate,
    EntityId id,
    CardinalDirection firstDirection,
    const CollisionProfile& firstCollisionProfile,
    CardinalDirection secondDirection,
    const CollisionProfile& secondCollisionProfile)
{
    SceneCoordinate firstAdjacentCoordinate =
        GetAdjacentCoordinate(coordinate, firstDirection);
    SceneCoordinate secondAdjacentCoordinate =
        GetAdjacentCoordinate(coordinate, secondDirection);
    Tile* tile = TryGetTile(coordinate);
    Tile* firstAdjacentTile = TryGetTile(firstAdjacentCoordinate);
    Tile* secondAdjacentTile = TryGetTile(secondAdjacentCoordinate);

    if (tile == nullptr || firstAdjacentTile == nullptr ||
        secondAdjacentTile == nullptr || tile->wallObject.has_value() ||
        firstDirection == secondDirection)
    {
        return false;
    }

    WallObject wallObject;
    wallObject.id = id;
    wallObject.directions[0] = firstDirection;
    wallObject.directions[1] = secondDirection;
    wallObject.directionCount = 2;
    tile->wallObject = wallObject;

    ApplyWallEdgeCollision(
        *tile,
        *firstAdjacentTile,
        firstDirection,
        firstCollisionProfile);
    ApplyWallEdgeCollision(
        *tile,
        *secondAdjacentTile,
        secondDirection,
        secondCollisionProfile);

    return true;
}

bool Scene::PlaceGameObject(
    SceneCoordinate coordinate,
    EntityId id,
    CardinalDirection direction,
    const CollisionProfile& collisionProfile)
{
    Tile* tile = TryGetTile(coordinate);

    if (tile == nullptr || tile->gameObject.has_value())
    {
        return false;
    }

    GameObject gameObject;
    gameObject.id = id;
    gameObject.direction = direction;
    tile->gameObject = gameObject;

    ApplyGameObjectCollision(*tile, collisionProfile);

    return true;
}

Tile* Scene::TryGetTile(SceneCoordinate coordinate)
{
    if (!Contains(coordinate))
    {
        return nullptr;
    }

    return &m_Tiles[GetIndex(coordinate)];
}

const Tile* Scene::TryGetTile(SceneCoordinate coordinate) const
{
    if (!Contains(coordinate))
    {
        return nullptr;
    }

    return &m_Tiles[GetIndex(coordinate)];
}

std::size_t Scene::GetIndex(SceneCoordinate coordinate)
{
    return static_cast<std::size_t>(
        coordinate.plane * Width * Height +
        coordinate.x * Height +
        coordinate.y);
}

SceneCoordinate Scene::GetAdjacentCoordinate(
    SceneCoordinate coordinate,
    CardinalDirection direction)
{
    switch (direction)
    {
        case CardinalDirection::North:
            return {coordinate.x, coordinate.y + 1, coordinate.plane};
        case CardinalDirection::East:
            return {coordinate.x + 1, coordinate.y, coordinate.plane};
        case CardinalDirection::South:
            return {coordinate.x, coordinate.y - 1, coordinate.plane};
        case CardinalDirection::West:
            return {coordinate.x - 1, coordinate.y, coordinate.plane};
    }

    return coordinate;
}

TileFlag Scene::GetMovementFlag(CardinalDirection direction)
{
    switch (direction)
    {
        case CardinalDirection::North:
            return TileFlag::BlockMovementNorth;
        case CardinalDirection::East:
            return TileFlag::BlockMovementEast;
        case CardinalDirection::South:
            return TileFlag::BlockMovementSouth;
        case CardinalDirection::West:
            return TileFlag::BlockMovementWest;
    }

    return TileFlag::None;
}

TileFlag Scene::GetLineOfSightFlag(CardinalDirection direction)
{
    switch (direction)
    {
        case CardinalDirection::North:
            return TileFlag::BlockLineOfSightNorth;
        case CardinalDirection::East:
            return TileFlag::BlockLineOfSightEast;
        case CardinalDirection::South:
            return TileFlag::BlockLineOfSightSouth;
        case CardinalDirection::West:
            return TileFlag::BlockLineOfSightWest;
    }

    return TileFlag::None;
}

CardinalDirection Scene::GetOppositeDirection(CardinalDirection direction)
{
    switch (direction)
    {
        case CardinalDirection::North:
            return CardinalDirection::South;
        case CardinalDirection::East:
            return CardinalDirection::West;
        case CardinalDirection::South:
            return CardinalDirection::North;
        case CardinalDirection::West:
            return CardinalDirection::East;
    }

    return direction;
}

void Scene::ApplyWallEdgeCollision(
    Tile& tile,
    Tile& adjacentTile,
    CardinalDirection direction,
    const CollisionProfile& collisionProfile)
{
    CardinalDirection oppositeDirection = GetOppositeDirection(direction);

    if (collisionProfile.blocksMovement)
    {
        tile.AddFlag(GetMovementFlag(direction));
        adjacentTile.AddFlag(GetMovementFlag(oppositeDirection));
    }

    if (collisionProfile.blocksLineOfSight)
    {
        tile.AddFlag(GetLineOfSightFlag(direction));
        adjacentTile.AddFlag(GetLineOfSightFlag(oppositeDirection));
    }
}

void Scene::ApplyGameObjectCollision(
    Tile& tile,
    const CollisionProfile& collisionProfile)
{
    if (collisionProfile.blocksMovement)
    {
        tile.AddFlag(TileFlag::BlockMovementObject);
    }

    if (collisionProfile.blocksLineOfSight)
    {
        tile.AddFlag(TileFlag::BlockLineOfSightFull);
    }
}

}  // namespace osrssim
