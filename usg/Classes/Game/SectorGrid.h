#pragma once

#define SECTORGRID_SX 11
#define SECTORGRID_SY 7

#include "Forward.h"
#include "Types.h"

namespace Game
{
	typedef struct
	{
	public:
		XBOOL mIsBad;
	} Sector;

	class SectorGrid
	{
	public:
		SectorGrid();
		void Initialize();
		
		void Load(Archive& a);
		void Save(Archive& a);
		
		void Destroy(const Vec2F& pos);
		
		void DBG_DestroyAll();
		
	private:
		static bool Pos2Coord(const Vec2F& pos, int& out_X, int& out_Y);
		bool Destroy(int x, int y);
		
		Sector mGrid[SECTORGRID_SY][SECTORGRID_SX];
	};
}
