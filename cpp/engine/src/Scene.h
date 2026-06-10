#pragma once

#include "Tile.h"

#include <array>
#include <cstddef>

namespace osrssim
{

class Scene
{
private:
    int m_BaseX = 0;
    int m_BaseY = 0;
    std::array<Tile, 4 * 104 * 104> m_Tiles;

public:
    static constexpr int PlaneCount = 4;
    static constexpr int Width = 104;
    static constexpr int Height = 104;

    explicit Scene(int baseX = 0, int baseY = 0);

    int GetBaseX() const;
    int GetBaseY() const;
    bool Contains(SceneCoordinate coordinate) const;
    Tile* TryGetTile(SceneCoordinate coordinate);
    const Tile* TryGetTile(SceneCoordinate coordinate) const;

private:
    static std::size_t GetIndex(SceneCoordinate coordinate);
};

}  // namespace osrssim
