#define USE_NDS 0
#define USE_IPHONE 1

#if USE_NDS
#include <nds.h>
#endif

#include "Touch.h"

#include "Log.h"

#define TOUCH_SX 320
#define TOUCH_SY 480

#define BUGFIX_COLUMN_SX 1
#define BUGFIX_COLUMN_SY 25
#define BUGFIX_MAXMOVE 150

namespace Paint
{
	Touch::Touch()
	{
		m_LastPressed = false;
		m_LastX = -1;
		m_LastY = -1;
	}

	bool Touch::Read(TouchInfo* info)
	{
	#if USE_IPHONE
		bool pressed = false;
		
		int x = 0;
		int y = 0;
	#endif

		if (pressed)
		{
			if (m_LastPressed)
			{
				// Calculate movement since last update.

				int deltaX = x - m_LastX;
				int deltaY = y - m_LastY;

				if (deltaX < 0)
					deltaX = -deltaX;
				if (deltaY < 0)
					deltaY = -deltaY;
			}

			// Detect screen boundary case.

			if (x < 0 || x >= TOUCH_SX)
				return false;
			if (y < 0 || y >= TOUCH_SY)
				return false;
		}

		//

		m_LastPressed = pressed;
		m_LastX = x;
		m_LastY = y;

		if (!pressed)
		{
			info->m_Pressed = false;
			info->m_X = 0;
			info->m_Y = 0;
			info->m_Pressure = 0;
		}
		else
		{
			LogMgr::WriteLine(LogLevel_Info, "TouchHeld");

			info->m_Pressed = true;
			info->m_X = x;
			info->m_Y = y;
			info->m_Pressure = 255;
			
			LogMgr::WriteLine(LogLevel_Info, "Press: %d", info->m_Pressure);
			//LogMgr::WriteLine(LogLevel_Info, "X: %d, Y: %d", info->m_X, info->m_Y);
		}

		return true;
	}
};
