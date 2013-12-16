//#include <nds.h>
#include "input.h"

#include "log.h"

namespace Paint
{
	InputMgr::InputMgr()
	{
		m_KeysDown = 0;

		m_TouchDown = false;
	}

	void InputMgr::Update()
	{
	#if 0
		scanKeys();

		// Update keys.

		{
			//uint down32 = keysDown();
			uint32 down = keysHeld();

			LogMgr::WriteLine(LogLevel_Info, "KeyDown: %d", down);

			uint32 change = m_KeysDown ^ down;

			uint32 scD = change & ~m_KeysDown;
			uint32 scU = change & m_KeysDown;

			//uint32 keys[] = { KEY_A, KEY_B, KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, 0 };

			//for (int i = 0; keys[i]; ++i)
			for (int i = 0; i < 13; ++i)
			{
				int key = BIT(i);

				if (scU & key)
					EmitKeyEvent(InputType_KeyUp, key, 0);
				if (scD & key)
					EmitKeyEvent(InputType_KeyDown, key, 1);
			}

			m_KeysDown = down;
		}
	#endif

		// Update touch.

		{
			TouchInfo info;

			if (m_Touch.Read(&info))
			{
				static int m_Coord[2];

				if (info.m_Pressed && !m_TouchDown)
				{
					m_Coord[0] = info.m_X;
					m_Coord[1] = info.m_Y;

					EmitTouchEvent(InputType_TouchBegin, &info);
				}

				if (!info.m_Pressed && m_TouchDown)
				{
					EmitTouchEvent(InputType_TouchEnd, &info);
				}

				// FIXME, generates unwanted move events.

				if (info.m_Pressed && m_TouchDown)
				{
					if (info.m_X != m_Coord[0] || info.m_Y != m_Coord[1])
					{
						m_Coord[0] = info.m_X;
						m_Coord[1] = info.m_Y;

						EmitTouchEvent(InputType_TouchMove, &info);
					}
				}

				if (info.m_Pressed)
				{
					EmitTouchEvent(InputType_TouchPressed, &info);
				}

				m_TouchDown = info.m_Pressed;
			}
		}
	}

	void InputMgr::EmitKeyEvent(InputType type, int key, int state)
	{
		LogMgr::WriteLine(LogLevel_Info, "KeyEvent: K: %d, S:%d", key, state);

		InputEvent e;

		e.m_Type = type;
		e.m_KeyInfo.m_Key = key;
		e.m_KeyInfo.m_State = state;

		m_InputCB.Invoke(&e);
	}

	void InputMgr::EmitTouchEvent(InputType type, TouchInfo* info)
	{
		InputEvent e;

		e.m_Type = type;
		e.m_TouchInfo = *info;

		m_InputCB.Invoke(&e);
	}
};
