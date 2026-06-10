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

}  // namespace osrssim
