#pragma once

#include "IObject.h"
#include "Types.h"

#define BH_MAX_SIZE 100.0f

class BlackHoleGrid;

class BlackHole : public IObject // Black hole / smaller weapon against enemy ships.
{
public:
	BlackHole(Map* map, BlackHoleGrid* grid, Vec2F pos);

	virtual void Update(Map* map);
	virtual void Render(BITMAP* buffer);
	virtual void HandleMessage(ObjectMessage message);

	float Radius_get() const;

	Vec2F Pos;
	float Mass;
	float MassEnergization;
	float Dissipation;
	float Rotation;

	BlackHoleGrid* m_Grid;
};

#define BH_GRID_SX 16 // todo: calculate dynamically
#define BH_GRID_SY 16
#define BH_GRID_TILE_SX 64
#define BH_GRID_TILE_SY 64

typedef void (*BlackHoleQueryCB)(void* self, BlackHole* hole);

class BlackHoleGrid
{
public:
	class Tile
	{
	public:
		std::vector<BlackHole*> Holes;
	};

	BlackHoleGrid()
	{
		Initialize();
	}

	void Initialize()
	{
	}

	class TileRange
	{
	public:
		int x1;
		int y1;
		int x2;
		int y2;
	};

	void GetTileRange(Vec2F pos, TileRange& o_Range)
	{
		Vec2F min = pos - Vec2F(BH_MAX_SIZE, BH_MAX_SIZE) / 2.0f;
		Vec2F max = pos + Vec2F(BH_MAX_SIZE, BH_MAX_SIZE) / 2.0f;

		o_Range.x1 = min[0] / BH_GRID_TILE_SX;
		o_Range.y1 = min[1] / BH_GRID_TILE_SY;
		o_Range.x2 = max[0] / BH_GRID_TILE_SX;
		o_Range.y2 = max[1] / BH_GRID_TILE_SY;

		if (o_Range.x1 < 0)
			o_Range.x1 = 0;
		if (o_Range.y1 < 0)
			o_Range.y1 = 0;
		if (o_Range.x2 >= BH_GRID_SX)
			o_Range.x2 = BH_GRID_SX - 1;
		if (o_Range.y2 >= BH_GRID_SY)
			o_Range.y2 = BH_GRID_SY - 1;
	}

	void Add(BlackHole* hole)
	{
		TileRange range;

		GetTileRange(hole->Pos, range);

		for (int tileX = range.x1; tileX <= range.x2; ++tileX)
		{
			for (int tileY = range.y1; tileY <= range.y2; ++tileY)
			{
				m_Tiles[tileX][tileY].Holes.push_back(hole);
			}
		}
	}

	void Remove(BlackHole* hole)
	{
		TileRange range;

		GetTileRange(hole->Pos, range);

		for (int tileX = range.x1; tileX <= range.x2; ++tileX)
		{
			for (int tileY = range.y1; tileY <= range.y2; ++tileY)
			{
				std::vector<BlackHole*>& holes = m_Tiles[tileX][tileY].Holes;

				holes.erase(std::find(holes.begin(), holes.end(), hole));
			}
		}
	}

	const Tile* GetTile(const Vec2F& pos) const
	{
		int tileX = pos[0] / BH_GRID_TILE_SX;
		int tileY = pos[1] / BH_GRID_TILE_SY;

		if (tileX < 0 || tileY < 0 || tileX >= BH_GRID_SX || tileY >= BH_GRID_SY)
			return &m_DummyTile;

		return &m_Tiles[tileX][tileY];
	}

	void Query(const Vec2F& pos, void* self, BlackHoleQueryCB callBack) const
	{
		RectF rect(
			pos - Vec2F(BH_MAX_SIZE, BH_MAX_SIZE) / 2.0f,
			Vec2F(BH_MAX_SIZE, BH_MAX_SIZE) / 2.0f);

		Query(rect, self, callBack);
	}

	void Query(const RectF& rect, void* self, BlackHoleQueryCB callBack) const
	{
		int tileX1 = rect.Min_get()[0] / BH_GRID_TILE_SX;
		int tileY1 = rect.Min_get()[1] / BH_GRID_TILE_SY;
		int tileX2 = rect.Max_get()[0] / BH_GRID_TILE_SX;
		int tileY2 = rect.Max_get()[1] / BH_GRID_TILE_SY;

		if (tileX1 < 0)
			tileX1 = 0;
		if (tileY1 < 0)
			tileY1 = 0;
		if (tileX2 >= BH_GRID_SX)
			tileX2 = BH_GRID_SX - 1;
		if (tileY2 >= BH_GRID_SY)
			tileY2 = BH_GRID_SY - 1;

		for (int tileX = tileX1; tileX <= tileX2; ++tileX)
		{
			for (int tileY = tileY1; tileY <= tileY2; ++tileY)
			{
				const Tile& tile = m_Tiles[tileX][tileY];

				for (int i = 0; i < tile.Holes.size(); ++i)
				{
					callBack(self, tile.Holes[i]);
				}
			}
		}
	}

	void Render(BITMAP* buffer);

	Tile m_Tiles[BH_GRID_SX][BH_GRID_SY];
	Tile m_DummyTile;
};
