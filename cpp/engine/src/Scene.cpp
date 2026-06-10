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

    tile->wallObject = WallObject{id, direction};

    if (collisionProfile.blocksMovement)
    {
        tile->AddFlag(GetMovementFlag(direction));
        adjacentTile->AddFlag(GetMovementFlag(GetOppositeDirection(direction)));
    }

    if (collisionProfile.blocksLineOfSight)
    {
        tile->AddFlag(GetLineOfSightFlag(direction));
        adjacentTile->AddFlag(GetLineOfSightFlag(GetOppositeDirection(direction)));
    }

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

}  // namespace osrssim
