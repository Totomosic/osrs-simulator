#include "Scene.h"

namespace osrssim
{

Scene::Scene(int baseX, int baseY)
    : m_BaseX(baseX),
      m_BaseY(baseY),
      m_Tiles(PlaneCount * Width * Height)
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

    if (tile == nullptr || adjacentTile == nullptr || tile->wallObject.has_value() ||
        !CanApplyWallEdgeCollision(*tile, *adjacentTile, direction, collisionProfile))
    {
        return false;
    }

    WallObject wallObject;
    wallObject.id = id;
    wallObject.directions[0] = direction;
    wallObject.collisionProfiles[0] = collisionProfile;
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
        firstDirection == secondDirection ||
        !CanApplyWallEdgeCollision(
            *tile,
            *firstAdjacentTile,
            firstDirection,
            firstCollisionProfile) ||
        !CanApplyWallEdgeCollision(
            *tile,
            *secondAdjacentTile,
            secondDirection,
            secondCollisionProfile))
    {
        return false;
    }

    WallObject wallObject;
    wallObject.id = id;
    wallObject.directions[0] = firstDirection;
    wallObject.directions[1] = secondDirection;
    wallObject.collisionProfiles[0] = firstCollisionProfile;
    wallObject.collisionProfiles[1] = secondCollisionProfile;
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
    return PlaceGameObject(coordinate, id, direction, 1, 1, collisionProfile);
}

bool Scene::PlaceGameObject(
    SceneCoordinate coordinate,
    EntityId id,
    CardinalDirection direction,
    int sizeX,
    int sizeY,
    const CollisionProfile& collisionProfile)
{
    if (sizeX <= 0 || sizeY <= 0)
    {
        return false;
    }

    const int footprintWidth = GetFootprintWidth(direction, sizeX, sizeY);
    const int footprintHeight = GetFootprintHeight(direction, sizeX, sizeY);

    for (int dx = 0; dx < footprintWidth; ++dx)
    {
        for (int dy = 0; dy < footprintHeight; ++dy)
        {
            Tile* tile = TryGetTile(
                {coordinate.x + dx, coordinate.y + dy, coordinate.plane});

            if (tile == nullptr || tile->gameObject.has_value() ||
                !CanApplyGameObjectCollision(*tile, collisionProfile))
            {
                return false;
            }
        }
    }

    GameObject gameObject;
    gameObject.id = id;
    gameObject.origin = coordinate;
    gameObject.direction = direction;
    gameObject.sizeX = sizeX;
    gameObject.sizeY = sizeY;
    gameObject.collisionProfile = collisionProfile;

    for (int dx = 0; dx < footprintWidth; ++dx)
    {
        for (int dy = 0; dy < footprintHeight; ++dy)
        {
            Tile* tile = TryGetTile(
                {coordinate.x + dx, coordinate.y + dy, coordinate.plane});

            tile->gameObject = gameObject;
            ApplyGameObjectCollision(*tile, collisionProfile);
        }
    }

    return true;
}

bool Scene::RemoveWallObject(SceneCoordinate coordinate)
{
    Tile* tile = TryGetTile(coordinate);

    if (tile == nullptr || !tile->wallObject.has_value())
    {
        return false;
    }

    WallObject wallObject = *tile->wallObject;

    for (int index = 0; index < wallObject.directionCount; ++index)
    {
        CardinalDirection direction =
            wallObject.directions[static_cast<std::size_t>(index)];
        SceneCoordinate adjacentCoordinate = GetAdjacentCoordinate(coordinate, direction);
        Tile* adjacentTile = TryGetTile(adjacentCoordinate);

        if (adjacentTile == nullptr)
        {
            return false;
        }
    }

    for (int index = 0; index < wallObject.directionCount; ++index)
    {
        CardinalDirection direction =
            wallObject.directions[static_cast<std::size_t>(index)];
        SceneCoordinate adjacentCoordinate = GetAdjacentCoordinate(coordinate, direction);
        Tile* adjacentTile = TryGetTile(adjacentCoordinate);

        RemoveWallEdgeCollision(
            *tile,
            *adjacentTile,
            direction,
            wallObject.collisionProfiles[static_cast<std::size_t>(index)]);
    }

    tile->wallObject = std::nullopt;

    return true;
}

bool Scene::RemoveGameObject(SceneCoordinate coordinate)
{
    const Tile* selectedTile = TryGetTile(coordinate);

    if (selectedTile == nullptr || !selectedTile->gameObject.has_value())
    {
        return false;
    }

    GameObject gameObject = *selectedTile->gameObject;
    const int footprintWidth =
        GetFootprintWidth(gameObject.direction, gameObject.sizeX, gameObject.sizeY);
    const int footprintHeight =
        GetFootprintHeight(gameObject.direction, gameObject.sizeX, gameObject.sizeY);

    for (int dx = 0; dx < footprintWidth; ++dx)
    {
        for (int dy = 0; dy < footprintHeight; ++dy)
        {
            SceneCoordinate coveredCoordinate{
                gameObject.origin.x + dx,
                gameObject.origin.y + dy,
                gameObject.origin.plane};
            Tile* tile = TryGetTile(coveredCoordinate);

            if (tile == nullptr || !tile->gameObject.has_value() ||
                tile->gameObject->id != gameObject.id ||
                !(tile->gameObject->origin == gameObject.origin))
            {
                return false;
            }
        }
    }

    for (int dx = 0; dx < footprintWidth; ++dx)
    {
        for (int dy = 0; dy < footprintHeight; ++dy)
        {
            SceneCoordinate coveredCoordinate{
                gameObject.origin.x + dx,
                gameObject.origin.y + dy,
                gameObject.origin.plane};
            Tile* tile = TryGetTile(coveredCoordinate);

            RemoveGameObjectCollision(*tile, gameObject.collisionProfile);
            tile->gameObject = std::nullopt;
        }
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

int Scene::GetFootprintWidth(
    CardinalDirection direction,
    int sizeX,
    int sizeY)
{
    switch (direction)
    {
        case CardinalDirection::North:
        case CardinalDirection::South:
            return sizeX;
        case CardinalDirection::East:
        case CardinalDirection::West:
            return sizeY;
    }

    return sizeX;
}

int Scene::GetFootprintHeight(
    CardinalDirection direction,
    int sizeX,
    int sizeY)
{
    switch (direction)
    {
        case CardinalDirection::North:
        case CardinalDirection::South:
            return sizeY;
        case CardinalDirection::East:
        case CardinalDirection::West:
            return sizeX;
    }

    return sizeY;
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

bool Scene::CanApplyWallEdgeCollision(
    const Tile& tile,
    const Tile& adjacentTile,
    CardinalDirection direction,
    const CollisionProfile& collisionProfile)
{
    CardinalDirection oppositeDirection = GetOppositeDirection(direction);

    if (collisionProfile.blocksMovement &&
        (tile.HasFlag(GetMovementFlag(direction)) ||
         adjacentTile.HasFlag(GetMovementFlag(oppositeDirection))))
    {
        return false;
    }

    if (collisionProfile.blocksLineOfSight &&
        (tile.HasFlag(GetLineOfSightFlag(direction)) ||
         adjacentTile.HasFlag(GetLineOfSightFlag(oppositeDirection))))
    {
        return false;
    }

    return true;
}

void Scene::RemoveWallEdgeCollision(
    Tile& tile,
    Tile& adjacentTile,
    CardinalDirection direction,
    const CollisionProfile& collisionProfile)
{
    CardinalDirection oppositeDirection = GetOppositeDirection(direction);

    if (collisionProfile.blocksMovement)
    {
        tile.RemoveFlag(GetMovementFlag(direction));
        adjacentTile.RemoveFlag(GetMovementFlag(oppositeDirection));
    }

    if (collisionProfile.blocksLineOfSight)
    {
        tile.RemoveFlag(GetLineOfSightFlag(direction));
        adjacentTile.RemoveFlag(GetLineOfSightFlag(oppositeDirection));
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

bool Scene::CanApplyGameObjectCollision(
    const Tile& tile,
    const CollisionProfile& collisionProfile)
{
    if (collisionProfile.blocksMovement &&
        tile.HasFlag(TileFlag::BlockMovementObject))
    {
        return false;
    }

    if (collisionProfile.blocksLineOfSight &&
        tile.HasFlag(TileFlag::BlockLineOfSightFull))
    {
        return false;
    }

    return true;
}

void Scene::RemoveGameObjectCollision(
    Tile& tile,
    const CollisionProfile& collisionProfile)
{
    if (collisionProfile.blocksMovement)
    {
        tile.RemoveFlag(TileFlag::BlockMovementObject);
    }

    if (collisionProfile.blocksLineOfSight)
    {
        tile.RemoveFlag(TileFlag::BlockLineOfSightFull);
    }
}

}  // namespace osrssim
