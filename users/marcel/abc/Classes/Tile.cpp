#include "Game.h"
#include "HackRender.h"
#include "Tile.h"

#define MAX_BREAK_COUNT 3
#define GROWBACK_TIME 3.0f
#define PERIOD_TIME 4.0f
#define PERIOD_TIME_ALIVE 3.0f
#define SOLID_SY 10.0f

Tile::Tile()
{
	mType = TileType_Undefined;
	mImage = 0;
	mBreakCount = 0;
	mBreakTime = 0.0f;
}

void Tile::Setup(TileType type, int tileX, int tileY)
{
	mType = type;
	mPosition[0] = tileX;
	mPosition[1] = tileY;
}

void Tile::Clear()
{
	mType = TileType_Empty;
}

void Tile::Update(float dt)
{
	switch (mType)
	{
		TileType_Brick_Breakable_Growback:
		{
			if (mBreakCount == MAX_BREAK_COUNT)
			{
				float dt = gGame->Time_get() - mBreakTime;
				if (dt >= GROWBACK_TIME)
					mBreakCount = 0;
			}
		}
			
		default:
			break;
	}
}

void Tile::Render()
{
	Vec2F min((mPosition[0] + 0) * TILE_SX, (mPosition[1] + 0) * TILE_SY);
	Vec2F max((mPosition[0] + 1) * TILE_SX, (mPosition[1] + 1) * TILE_SY);
	Vec2F maxBrick = min + Vec2F(TILE_SX, SOLID_SY);
	
	switch (mType)
	{
		case TileType_Empty:
			break;
		case TileType_Brick_Solid:
			HR_Rect(min, maxBrick);
			break;
		case TileType_Brick_Breakable:
		case TileType_Brick_Breakable_Growback:
		case TileType_Brick_Periodic:
			if (IsSolid_get())
				HR_Rect(min, maxBrick);
			break;
		case TileType_Ring:
			HR_Circle(min + (max - min) * 0.5f, TILE_SX / 8.0f);
			break;
		case TileType_Exit:
			HR_Circle(min + (max - min) * 0.5f, TILE_SX / 4.0f);
			break;
		case TileType_Spike:
			HR_Rect(min + Vec2F(4.0f, 4.0f), max - Vec2F(4.0f, 4.0f));
			break;
	}
}

bool Tile::HitTest(Vec2F position) const
{
	if (!IsSolid_get())
		return false;
	
	position -= Vec2F(mPosition[0] * TILE_SX, mPosition[1] * TILE_SY);
	
	return
		position[0] >= 0.0f && 
		position[0] <= TILE_SX &&
		position[1] >= 0.0f &&
		position[1] <= SOLID_SY;
}

void Tile::HandleHit()
{
	switch (mType)
	{
		case TileType_Brick_Solid:
			//gGame->mPlayer.HandleBounce();
			// todo: play sound
			break;
		case TileType_Brick_Breakable:
		case TileType_Brick_Breakable_Growback:
			//gGame->mPlayer.HandleBounce();
			if (gGame->mPlayer.mSpeed[1] >= 0.0f)
			{
				mBreakCount++;
				mBreakTime = gGame->Time_get();
				// todo: play sound
			}
			break;
		case TileType_Brick_Periodic:
			//gGame->mPlayer.HandleBounce();
			// todo: play sound
			break;
		case TileType_Ring:
			gGame->mPlayer.HandleRing();
			// todo: game: reduce ring count
			// todo: play sound
			break;
		case TileType_Exit:
			// todo: game: win map
			// todo: play sound
			break;
		case TileType_Spike:
			gGame->mPlayer.HandleDie();
			// todo: play sound
			break;
	}
}

bool Tile::IsSolid_get() const
{
	switch (mType)
	{
		case TileType_Brick_Solid:
			return true;
		case TileType_Brick_Breakable:
		case TileType_Brick_Breakable_Growback:
			return mBreakCount < MAX_BREAK_COUNT;
		case TileType_Brick_Periodic:
			return fmod(gGame->Time_get(), PERIOD_TIME) <= PERIOD_TIME_ALIVE;
		default:
			return false;
	}
}
