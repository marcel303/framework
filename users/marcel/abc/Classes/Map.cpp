#include "Game.h"
#include "Map.h"

Map::Map()
{
}

void Map::Setup()
{
	for (int y = 0; y < MAP_SY; ++y)
	{
		for (int x = 0; x < MAP_SX; ++x)
		{
			TileType type = TileType_Undefined;
			
			int index = rand() % 8;
			
			if (index == 0)
				type = TileType_Empty;
			if (index == 1)
				type = TileType_Brick_Breakable;
			if (index == 2)
				type = TileType_Brick_Breakable_Growback;
			if (index == 3)
				type = TileType_Brick_Periodic;
			if (index == 4)
				type = TileType_Brick_Solid;
			if (index == 5)
				type = TileType_Ring;
			if (index == 6)
				type = TileType_Exit;
			if (index == 7)
				type = TileType_Spike;
			
			mTiles[y][x].Setup(type, x, y);
		}
	}
}

void Map::Update(float dt)
{
	for (int y = 0; y < MAP_SY; ++y)
		for (int x = 0; x < MAP_SX; ++x)
			mTiles[y][x].Update(dt);
}

void Map::Render()
{
	Vec2F min;
	Vec2F max;
	
	gGame->mViewer.GetViewRect(min, max);
	
	int x1, y1, x2, y2;
	
	GetTileRect(min, max, x1, y1, x2, y2);
	
	for (int y = y1; y <= y2; ++y)
		for (int x = x1; x <= x2; ++x)
			mTiles[y][x].Render();
}

void Map::TileQuery(Vec2F min, Vec2F max, TileQueryHandler handler, void* obj)
{
	int x1, y1, x2, y2;
	
	GetTileRect(min, max, x1, y1, x2, y2);
	
	for (int y = y1; y <= y2; ++y)
		for (int x = x1; x <= x2; ++x)
			handler(obj, mTiles[y][x]);
}

void Map::GetTileRect(Vec2F min, Vec2F max, int& oX1, int& oY1, int& oX2, int& oY2)
{
	oX1 = min[0] / TILE_SX;
	oY1 = min[1] / TILE_SY;
	oX2 = max[0] / TILE_SX;
	oY2 = max[1] / TILE_SY;
	
	if (oX1 < 0)
		oX1 = 0;
	if (oY1 < 0)
		oY1 = 0;
	if (oX2 >= TILE_SX)
		oX2 = TILE_SX - 1;
	if (oY2 >= TILE_SY)
		oY2 = TILE_SY - 1;
}

Tile* Map::GetTile(Vec2F position)
{
	if (position[0] < 0.0f || position[1] < 0.0f)
		return 0;
	
	int x = position[0] / TILE_SX;
	int y = position[1] / TILE_SY;
	
	if (x < 0 || y < 0 || x >= MAP_SX || y >= MAP_SY)
		return 0;
	
	return &mTiles[y][x];
}
