#pragma once

#include "Callback.h"
#include "Touch.h"
#include "Types.h"

namespace Paint
{
	enum InputType
	{
		InputType_KeyUp,
		InputType_KeyDown,
		InputType_TouchBegin,
		InputType_TouchEnd,
		InputType_TouchMove,
		InputType_TouchPressed
	};

	typedef struct
	{
		InputType m_Type;
		union
		{
			struct
			{
				UInt32 m_Key;
				int m_State;
			} m_KeyInfo;
			TouchInfo m_TouchInfo;
		};
	} InputEvent;

	class InputMgr
	{
	public:
		InputMgr();

		void Update();

		void EmitKeyEvent(InputType type, int key, int state);
		void EmitTouchEvent(InputType type, TouchInfo* info);

		CallBack m_InputCB;
		UInt32 m_KeysDown;
		Touch m_Touch;
		bool m_TouchDown;
	};
};
