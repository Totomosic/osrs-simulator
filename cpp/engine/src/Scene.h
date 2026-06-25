#pragma once

#include "Tile.h"

#include <cstddef>
#include <vector>

namespace osrssim
{

struct SceneEntityPlacement
{
    enum class Kind
    {
        GameObject,
        WallObject,
    };

    Kind kind = Kind::GameObject;
    EntityId id = 0;
    SceneCoordinate coordinate;
    CardinalDirection direction = CardinalDirection::North;
    std::array<CardinalDirection, 2> directions{
        CardinalDirection::North,
        CardinalDirection::North};
    std::array<CollisionProfile, 2> collisionProfiles{};
    int directionCount = 1;
    int sizeX = 1;
    int sizeY = 1;
    CollisionProfile collisionProfile;
};

class Scene
{
private:
    int m_BaseX = 0;
    int m_BaseY = 0;
    std::vector<Tile> m_Tiles;

public:
    static constexpr int PlaneCount = 4;
    static constexpr int Width = 104;
    static constexpr int Height = 104;

    explicit Scene(int baseX = 0, int baseY = 0);

    int GetBaseX() const;
    int GetBaseY() const;
    bool Contains(SceneCoordinate coordinate) const;
    bool PlaceWallObject(
        SceneCoordinate coordinate,
        EntityId id,
        CardinalDirection direction,
        const CollisionProfile& collisionProfile);
    bool PlaceWallObject(
        SceneCoordinate coordinate,
        EntityId id,
        CardinalDirection firstDirection,
        const CollisionProfile& firstCollisionProfile,
        CardinalDirection secondDirection,
        const CollisionProfile& secondCollisionProfile);
    bool PlaceGameObject(
        SceneCoordinate coordinate,
        EntityId id,
        CardinalDirection direction,
        const CollisionProfile& collisionProfile);
    bool PlaceGameObject(
        SceneCoordinate coordinate,
        EntityId id,
        CardinalDirection direction,
        int sizeX,
        int sizeY,
        const CollisionProfile& collisionProfile);
    bool RemoveWallObject(SceneCoordinate coordinate);
    bool RemoveGameObject(SceneCoordinate coordinate);
    Tile* TryGetTile(SceneCoordinate coordinate);
    const Tile* TryGetTile(SceneCoordinate coordinate) const;
    std::vector<SceneEntityPlacement> GetSceneEntityPlacements() const;

private:
    static std::size_t GetIndex(SceneCoordinate coordinate);
    static SceneCoordinate GetAdjacentCoordinate(
        SceneCoordinate coordinate,
        CardinalDirection direction);
    static TileFlag GetMovementFlag(CardinalDirection direction);
    static TileFlag GetLineOfSightFlag(CardinalDirection direction);
    static CardinalDirection GetOppositeDirection(CardinalDirection direction);
    static int GetFootprintWidth(
        CardinalDirection direction,
        int sizeX,
        int sizeY);
    static int GetFootprintHeight(
        CardinalDirection direction,
        int sizeX,
        int sizeY);
    static void ApplyWallEdgeCollision(
        Tile& tile,
        Tile& adjacentTile,
        CardinalDirection direction,
        const CollisionProfile& collisionProfile);
    static bool CanApplyWallEdgeCollision(
        const Tile& tile,
        const Tile& adjacentTile,
        CardinalDirection direction,
        const CollisionProfile& collisionProfile);
    static void RemoveWallEdgeCollision(
        Tile& tile,
        Tile& adjacentTile,
        CardinalDirection direction,
        const CollisionProfile& collisionProfile);
    static void ApplyGameObjectCollision(
        Tile& tile,
        const CollisionProfile& collisionProfile);
    static bool CanApplyGameObjectCollision(
        const Tile& tile,
        const CollisionProfile& collisionProfile);
    static void RemoveGameObjectCollision(
        Tile& tile,
        const CollisionProfile& collisionProfile);
};

}  // namespace osrssim
