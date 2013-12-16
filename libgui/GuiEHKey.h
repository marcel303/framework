#pragma once

#include "GuiEH.h"
#include "GuiObject.h"

namespace Gui
{
	typedef void (*KeyStateCallback)(Object* me, char key, int keyCode);
	typedef KeyStateCallback KeyDownCallback;
	typedef KeyStateCallback KeyUpCallback;
	//typedef KeyStateCallback KeyRepeatCallback;

	class EHKeyState : public EH
	{
	public:
		inline EHKeyState() : EH()
		{
		}

		inline void Do(char key, int keyCode)
		{
			m_callback(GetMe(), key, keyCode);
		}

		inline void Initialize(Object* me, KeyStateCallback callback)
		{
			SetMe(me);
			m_callback = callback;
		}

	private:
		KeyStateCallback m_callback;
	};

	typedef EHKeyState EHKeyDown;
	typedef EHKeyState EHKeyUp;
	//typedef EHKeyState EHKeyRepeat;
};
