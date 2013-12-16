#include "Archive.h"
#include "GameSettings.h"
#include "MemOps.h"
#include "SectorGrid.h"
#include "World.h"

const static float CELL_SX = WORLD_SX / (float)SECTORGRID_SX;
const static float CELL_SY = WORLD_SY / (float)SECTORGRID_SY;

const static float WORLD_TO_CELL_X = 1.0f / CELL_SX;
const static float WORLD_TO_CELL_Y = 1.0f / CELL_SY;

#define IS_OUTSIDE(x, y) (x < 0 || x >= SECTORGRID_SX || y < 0 || y >= SECTORGRID_SY)

namespace Game
{
	SectorGrid::SectorGrid()
	{
		Initialize();
	}
	
	void SectorGrid::Initialize()
	{
		Mem::ClearZero(mGrid, sizeof(Sector) * SECTORGRID_SX * SECTORGRID_SY);
	}
	
	void SectorGrid::Load(Archive& a)
	{
		while (a.NextSection())
		{
			if (a.IsSection("s"))
			{
				int x = 0;
				int y = 0;
				int s = 0;
				
				while (a.NextValue())
				{
					if (a.IsKey("x"))
					{
						x = a.GetValue_Int32();
					}
					else if (a.IsKey("y"))
					{
						y = a.GetValue_Int32();
					}
					else if (a.IsKey("s"))
					{
						s = a.GetValue_Int32();
					}
					else
					{
#ifndef DEPLOYMENT
						throw ExceptionVA("unknown key: %s", a.GetKey());
#endif
					}
				}
				
				if (s)
				{
					Destroy(x, y);
				}
			}
		}
	}
	
	void SectorGrid::Save(Archive& a)
	{
		for (int y = 0; y < SECTORGRID_SY; ++y)
		{
			for (int x = 0; x < SECTORGRID_SX; ++x)
			{
				if (mGrid[y][x].mIsBad)
				{
					a.WriteSection("s");
					a.WriteValue_Int32("x", x);
					a.WriteValue_Int32("y", y);
					a.WriteValue_Int32("s", mGrid[y][x].mIsBad ? 1 : 0);
					a.WriteSectionEnd();
				}
			}
		}
	}
	
	void SectorGrid::Destroy(const Vec2F& pos)
	{
		int x;
		int y;
		
		if (!Pos2Coord(pos, x, y))
			return;
		
		Destroy(x, y);
	}
	
	void SectorGrid::DBG_DestroyAll()
	{
		for (int x = 0; x < SECTORGRID_SX; ++x)
		{
			for (int y = 0; y < SECTORGRID_SY; ++y)
			{
				Destroy(x, y);
			}
		}
	}
	
	bool SectorGrid::Pos2Coord(const Vec2F& pos, int& out_X, int& out_Y)
	{
		out_X = (int)(pos[0] * WORLD_TO_CELL_X);
		out_Y = (int)(pos[1] * WORLD_TO_CELL_Y);
		
		return !IS_OUTSIDE(out_X, out_Y);
	}
	
	bool SectorGrid::Destroy(int x, int y)
	{
		if (IS_OUTSIDE(x, y))
			return false;
		
		if (mGrid[y][x].mIsBad)
			return true;
		
		mGrid[y][x].mIsBad = true;
		
		//
		
		Vec2F pos(CELL_SX * (x + 0.5f), CELL_SY * (y + 0.5f));
		
		g_World->SpawnBadSector(pos);
		
		return true;
	}
}

