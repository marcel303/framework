#pragma once

#include "Types.h"

#define TILE_SX 64
#define TILE_SY 64

class TileImage
{
public:
	float mTexPos[2];
	float mTexSize[2];
};

enum TileType
{
	TileType_Undefined,
	TileType_Empty,
	TileType_Brick_Solid,
	TileType_Brick_Breakable,
	TileType_Brick_Breakable_Growback,
	TileType_Brick_Periodic,
	TileType_Ring,
	TileType_Exit,
	TileType_Spike
};
	
class Tile
{
public:
	Tile();
	void Setup(TileType type, int tileX, int tileY);
	void Clear();
	
	void Update(float dt);
	void Render();
	
	bool HitTest(Vec2F position) const;
	
	void HandleHit();
	
	bool IsSolid_get() const;
	
	TileType mType;
	int mPosition[2];
	TileImage* mImage;
	int mBreakCount;
	float mBreakTime;
};
