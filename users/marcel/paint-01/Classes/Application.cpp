#include "Application.h"

#include "Calc.h"
#include "Types.h"

#include <stdlib.h> // todo, remove

namespace Paint
{
//	static int s_ToolType = ToolType_Brush; // fixme, app member
//	static Col s_Col; // fixme, app member.

	static BrushCache s_BrushCache; // fixme, app member.

	Application::Application(MemBitmap* bitmap)
	{
		m_Log = LogCtx("App"); // TODO: Get LogCtx from LogMgr, use pool.

		m_Log.WriteLine(LogLevel_Info, "Initializing");

		m_Bitmap = bitmap;
		m_StatDirty = 0;
		//m_AppScreen = AppScreen_Canvas;
		m_ToolType = ToolType_Brush;
		m_Col.v[0] = INT_TO_FIX(0);
		m_Col.v[1] = INT_TO_FIX(0);
		m_Col.v[2] = INT_TO_FIX(127);
		m_Opacity = REAL_TO_FIX(1.0f);

		s_BrushCache.Prepare();

		m_Brush.MakeCircle(50, REAL_TO_FIX(0.0f));
//		m_Brush.MakeCircle(50, REAL_TO_FIX(0.6f));
//		m_Brush.MakeCircle(10, REAL_TO_FIX(0.6f));
		UpdateBrush();

		m_InputMgr.m_InputCB = CallBack(this, HandleInput);
		m_Dirty.m_DirtyCB = CallBack(this, HandleDirty);
		m_Traveller.m_TravelCB = CallBack(this, HandleTravel);

		// FIXME, dirty should be updated in ColBuf.
		m_Dirty.MakeDirty();
	}

	void Application::UpdateBrush()
	{
		// Update brush properties.

		//m_Brush.MakeCircle(50, REAL_TO_FIX(0.0f));
		//m_Brush.MakeCircle(20, REAL_TO_FIX(0.2f));

		// Update travel interval.

		m_Traveller.m_Step = m_Brush.m_Sx / 6.0f;
		
		if (m_Traveller.m_Step < 1.0f)
			m_Traveller.m_Step = 1.0f;
	}

	void Application::HandleInput(void* obj, void* arg)
	{
		Application* self = (Application*)obj;
		InputEvent* e = (InputEvent*)arg;

//		switch (self->m_AppScreen)
//		{
//		case AppScreen_Canvas:
			switch (e->m_Type)
			{
			case InputType_TouchBegin:
//				self->m_Log.WriteLine(LogLevel_Info, "TouchBegin");
				self->m_Traveller.Begin(
					e->m_TouchInfo.m_X,
					e->m_TouchInfo.m_Y,
					e->m_TouchInfo.m_Pressure);
				break;
			case InputType_TouchEnd:
//				self->m_Log.WriteLine(LogLevel_Info, "TouchEnd");
				self->m_Traveller.End(
					e->m_TouchInfo.m_X,
					e->m_TouchInfo.m_Y);
				break;
			case InputType_TouchMove:
				{
//					self->m_Log.WriteLine(LogLevel_Info, "TouchMove");
					
					float pressure = e->m_TouchInfo.m_Pressure / 255.0f;
					
					//self->m_Log.WriteLine(LogLevel_Info, "Pressure: %f", pressure);
					
					self->m_Traveller.Update(
						e->m_TouchInfo.m_X,
						e->m_TouchInfo.m_Y,
						pressure); // fixme, pass touch info.
				}
				break;

			default:
				break;
			}
//			break;
//		}
	}

	void Application::HandleDirty(void* obj, void* arg)
	{
		Application* self = (Application*)obj;
		DirtyEvent* e = (DirtyEvent*)arg;

//		self->m_Log.WriteLine(LogLevel_Info, "Dirty, X1=%d, X2=%d, Y=%d", e->x1, e->x2, e->y);

#if 1
		// copy pixels to bitmap.
		
		self->m_Bitmap->DrawLine(
			e->x1,
			e->x2,
			e->y,
			&self->m_ColBuf.m_Lines[e->y][e->x1]);
#endif
		
		self->m_DirtyCB.Invoke(e);
		
		++self->m_StatDirty;
	}

	void Application::HandleTravel(void* obj, void* arg)
	{
		Application* self = (Application*)obj;
		TravelEvent* e = (TravelEvent*)arg;

//		self->m_Log.WriteLine(LogLevel_Info, "Travel X: %f, Y: %f, DX: %f, DY: %f", e->x, e->y, e->dx, e->dy);
		
#if 0
		if ((rand() % 500) == 0)
			s_ToolType = ToolType_Brush + (rand() % 3);
		if ((rand() % 250) == 0)
		{
			self->m_Col.v[0] = INT_TO_FIX(rand() % 256);
			self->m_Col.v[1] = INT_TO_FIX(rand() % 256);
			self->m_Col.v[2] = INT_TO_FIX(rand() % 256);
		}
#endif

//		m_Opacity = REAL_TO_FIX(e->pressure);

		switch (self->m_ToolType)
		{
		case ToolType_Brush:
			self->m_ColBuf.ApplyPaint(
				&self->m_Dirty,
				e->x,
				e->y,
				&self->m_Brush,
				&self->m_Col,
				self->m_Opacity);
			break;

		case ToolType_Smudge:
			self->m_ColBuf.ApplySmudge(
				&self->m_Dirty,
				e->x,
				e->y,
				REAL_TO_FIX(e->dx),
				REAL_TO_FIX(e->dy),
				&self->m_Brush,
				INT_TO_FIX(1));
			break;

		case ToolType_Smoothe:
			self->m_ColBuf.ApplySmoothe(
				&self->m_Dirty,
				e->x,
				e->y,
				&self->m_Brush,
				1);
			break;
		};
	}
	
	void Application::HandlePaint(void* obj, void* arg)
	{
		Application* self = (Application*)obj;
		
		self->m_Dirty.Validate();
	}
};