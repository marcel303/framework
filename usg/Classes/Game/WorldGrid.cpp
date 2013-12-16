#include "Calc.h"
#include "Log.h"
#include "WorldGrid.h"

namespace Game
{
	// GridCell

#if 1
	typedef struct ForEachState
	{
		void* arg;
		const void* self;
		Grid_ForEachCallBack callBack;
	} ForEachState;

	void GridCell::HandleForEach(void* obj, void* arg)
	{
		ForEachState* state = (ForEachState*)arg;
		
		state->callBack(obj, state->arg);
	}

	void GridCell::ForEach(Grid_ForEachCallBack callBack, void* arg) const
	{
		ForEachState state;
		
		state.arg = arg;
		state.self = this;
		state.callBack = callBack;
		
		m_HashMap.ForEach(HandleForEach, &state);
	}
#else
	void GridCell::ForEach(Grid_ForEachCallBack callBack, void* arg) const
	{
		for (const Col::ListNode<void*>* node = m_List.m_Head; node; node = node->m_Next)
			callBack(node->m_Object, arg);
	}
#endif
	
	void GridCell::DBG_Show(int x, int y)
	{
		LOG(LogLevel_Debug, "grid_cell: x:%d, y:%d, hmap.bucket_count:%d, hmap.obj_count:%d", x, y, m_HashMap.BucketCount_get(), m_HashMap.ObjectCount_get());
	}
	
	// WorldGrid
	
	WorldGrid::WorldGrid()
	{
		Initialize();
	}
	
	WorldGrid::~WorldGrid()
	{
		Setup(0, 0, 1.0f, 1.0f);
	}
	
	void WorldGrid::Initialize()
	{
		m_Log = LogCtx("WorldGrid");
		
		m_Sx = 0;
		m_Sy = 0;
		m_CellSx = 0.0f;
		m_CellSy = 0.0f;
		m_CellSxRcp = 0.0f;
		m_CellSyRcp = 0.0f;
		m_GridSx = 0;
		m_GridSy = 0;
		m_Grid = 0;
	}
	
	void WorldGrid::Setup(int sx, int sy, float cellSx, float cellSy)
	{
		delete[] m_Grid;
		m_Grid = 0;
		
		m_Sx = sx;
		m_Sy = sy;
		m_CellSx = cellSx;
		m_CellSy = cellSy;
		m_CellSxRcp = 1.0f / cellSx;
		m_CellSyRcp = 1.0f / cellSy;
		m_GridSx = (int)Calc::DivideUp((float)m_Sx, m_CellSx);
		m_GridSy = (int)Calc::DivideUp((float)m_Sy, m_CellSy);
		
		const int gridArea = m_GridSx * m_GridSy;
		
		if (gridArea > 0)
		{
			m_Grid = new GridCell[gridArea];
		}
		
//		m_Log.WriteLine(LogLevel_Debug, "Setup: sx=%d, sy=%d, cellSize=%d", sx, sy, cellSize);
	}
	
	void WorldGrid::Add(const Vec2F& position, void* obj)
	{
		PointI cell;
		
		GetCell(position, cell);
		
		if (cell[0] < 0 || cell[1] < 0 || cell[0] >= m_GridSx || cell[1] >= m_GridSy)
			return;
		
		const int index = cell[0] + cell[1] * m_GridSx;
		
		m_Grid[index].Add(obj);
	}
	
	void WorldGrid::Remove(const Vec2F& position, void* obj)
	{
		PointI cell;
		
		GetCell(position, cell);
		
		if (cell[0] < 0 || cell[1] < 0 || cell[0] >= m_GridSx || cell[1] >= m_GridSy)
			return;
		
		const int index = cell[0] + cell[1] * m_GridSx;
		
		m_Grid[index].Remove(obj);
	}
	
	void WorldGrid::Update(const Vec2F& oldPosition, const Vec2F& newPosition, void* obj)
	{
		PointI oldCell;
		PointI newCell;
		
		GetCell(oldPosition, oldCell);
		GetCell(newPosition, newCell);
		
		// nothing to be done?
		
		if (newCell[0] == oldCell[0] && newCell[1] == oldCell[1])
		{
			return;
		}
		
		Remove(oldPosition, obj);
		Add(newPosition, obj);
	}
		
	void WorldGrid::ForEach_InArea(const Vec2F& min, const Vec2F& max, Grid_ForEachCallBack callBack, void* arg) const
	{
		RectI area;
		
		GetcellArea(min, max, area);
		
		for (int oy = 0; oy < area.m_Size[1]; ++oy)
		{
			int y = oy + area.m_Position[1];
			
			for (int ox = 0; ox < area.m_Size[0]; ++ox)
			{
				int x = ox + area.m_Position[0];
				
				if (x < 0 || y < 0 || x >= m_GridSx || y >= m_GridSy)
					continue;
				
				const int index = x + y * m_GridSx;
				
				const GridCell& cell = m_Grid[index];
				
				cell.ForEach(callBack, arg);
			}
		}
	}
	
	//
	
	void WorldGrid::DBG_Show()
	{
		m_Log.WriteLine(LogLevel_Debug, "sx: %d", m_Sx);
		m_Log.WriteLine(LogLevel_Debug, "sy: %d", m_Sy);
		m_Log.WriteLine(LogLevel_Debug, "grid.sx: %d", m_GridSx);
		m_Log.WriteLine(LogLevel_Debug, "grid.sy: %d", m_GridSy);
		m_Log.WriteLine(LogLevel_Debug, "grid.area: %d", m_GridSx * m_GridSy);
		
		for (int y = 0; y < m_GridSy; ++y)
		{
			for (int x = 0; x < m_GridSx; ++x)
			{
				GridCell& cell = m_Grid[x + y * m_GridSx];
				
				cell.DBG_Show(x, y);
			}
		}
	}
	
	int WorldGrid::DBG_GridSx_get() const
	{
		return m_GridSx;
	}
	
	int WorldGrid::DBG_GridSy_get() const
	{
		return m_GridSy;
	}
	
	const GridCell* WorldGrid::DBG_GetCell(int x, int y) const
	{
		return m_Grid + x + y * m_GridSx;
	}
	
	float WorldGrid::DBG_CellSx_get() const
	{
		return m_CellSx;
	}
	
	float WorldGrid::DBG_CellSy_get() const
	{
		return m_CellSy;
	}
	
	//
	
	void WorldGrid::GetcellArea(const Vec2F& min, const Vec2F& max, RectI& out_Area) const
	{		
		PointI c1;
		PointI c2;
		
		GetCell(min, c1);
		GetCell(max, c2);
		
		out_Area.m_Position = c1;
		out_Area.m_Size[0] = c2[0] - c1[0] + 1;
		out_Area.m_Size[1] = c2[1] - c1[1] + 1;
	}
};
