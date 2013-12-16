#pragma once

#include "ColHashMap.h"
#include "Hash.h"
#include "Types.h"

/*
 
WorldGrid usage:
 
  - Create WorldGrid:
 
 WorldGrid* worldGrid = new WorldGrid;
 
 worldGrid->Setup(480, 320, 128);
 
 - When new entities are added to the world, add them to the world grid:
 
 Entity* entity = new Entity();
 entity->Position_set(Vec2F(100.0f, 100.0f));
 
 worldGrid->Add(entity->Position_get(), entity);
 
 - When the entity position gets updated, notify the grid:
 
 Vec2F oldPosition = entity->Positin_get();
 
 entity->Move(Vec2F(1.0f, 1.0f));
 
 Vec2F newPosition = entity->Position_get();
 
 worldGrid->Update(oldPosition, newPosition, entity);
 
 - When entities are removed from the world, remove them from the world grid:
 
 worldGrid->Remove(entity->Position_get(), entity);
 
 To retrieve the objects in a bounding box area, use:

 static void HandleForEach(void* obj, void* arg)
 {
 	BlackHole* bh = (BlackHole*)arg;
 
 	// ..
 }

 static void Update()
 {
 	worldGrid->ForEach_InArea(m_BlackHole->m_Min, m_BlackHole->m_Max, HandleForEach, m_BlackHole);
 }
 
*/

namespace Game
{
	typedef void (*Grid_ForEachCallBack)(void* obj, void* arg);
	
	class GridCell
	{
	public:
		inline GridCell()
		{
			m_ObjectCount = 0;
		}
		
#if 1
		inline static Hash CalcHash(const void* obj)
		{
			return HashFunc::Hash_FNV1a(&obj, sizeof(void*));
		}
		
		inline void Add(void* obj)
		{
			m_ObjectCount++;
			
			m_HashMap.Add(CalcHash(obj), obj);
		}
		
		inline void Remove(void* obj)
		{
			m_ObjectCount--;
			
			m_HashMap.Remove(CalcHash(obj), obj);
		}
#else
		inline void Add(void* obj)
		{
			m_List.AddTail(obj);
		}
		
		inline void Remove(void* obj)
		{
			m_List.Remove(m_List.FindNode(obj));
		}
#endif
		
		static void HandleForEach(void* obj, void* arg);
		
		void ForEach(Grid_ForEachCallBack callBack, void* arg) const;
		
		void DBG_Show(int x, int y);
		
		Col::HashMap m_HashMap;
		Col::List<void*> m_List;
		int m_ObjectCount;
	};
	
	class WorldGrid
	{
	public:
		WorldGrid();
		~WorldGrid();
		void Initialize();
		
		void Setup(int sx, int sy, float cellSx, float cellSy);
		
		void Add(const Vec2F& position, void* obj);
		void Remove(const Vec2F& position, void* obj);
		void Update(const Vec2F& oldPosition, const Vec2F& newPosition, void* obj);
		
		void ForEach_InArea(const Vec2F& min, const Vec2F& max, Grid_ForEachCallBack callBack, void* arg) const;
		
		void DBG_Show();
		
		int DBG_GridSx_get() const;
		int DBG_GridSy_get() const;
		const GridCell* DBG_GetCell(int x, int y) const;
		float DBG_CellSx_get() const;
		float DBG_CellSy_get() const;
		
	private:
		inline void GetCell(const Vec2F& position, PointI& out_Cell) const
		{
			out_Cell[0] = (int)(position[0] * m_CellSxRcp);
			out_Cell[1] = (int)(position[1] * m_CellSyRcp);
		}
		
		void GetcellArea(const Vec2F& min, const Vec2F& max, RectI& out_Area) const;
		
		int m_Sx;
		int m_Sy;
		
		float m_CellSx;
		float m_CellSy;
		
		GridCell* m_Grid;
		int m_GridSx;
		int m_GridSy;
		
		float m_CellSxRcp;
		float m_CellSyRcp;
		
		LogCtx m_Log;
	};
}
