#pragma once

#include "Tile.h"
#include "Types.h"

#define MAP_SX 128
#define MAP_SY 128

typedef void (*TileQueryHandler)(void* obj, Tile& tile);

class Map
{
public:
	Map();
	void Setup();
	
	void Update(float dt);
	void Render();
	
	void TileQuery(Vec2F min, Vec2F max, TileQueryHandler handler, void* obj);
	
	void GetTileRect(Vec2F min, Vec2F max, int& oX1, int& oY1, int& oX2, int& oY2);
	
	Tile* GetTile(Vec2F position);
	
	inline Tile& operator()(int x, int y)
	{
		return mTiles[y][x];
	}
	
	Tile mTiles[MAP_SY][MAP_SX];
};
