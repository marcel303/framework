#pragma once

#include "EventHandler.h"
#include "Types.h"

namespace Game
{
	class EventHandler_PlayerController_Gamepad_Psp : public EventHandler
	{
	public:
		static EventHandler_PlayerController_Gamepad_Psp& I();

		void Clear()
		{
			mJoyAxis_Move.SetZero();
			mJoyAxis_Fire.SetZero();
			mJoyFireActive = false;
			mJoySpecialActive = false;
			mJoyShockActive = false;
		}

		Vec2F mJoyAxis_Move;
		Vec2F mJoyAxis_Fire;
		bool mJoyFireActive;
		bool mJoySpecialActive;
		bool mJoyShockActive;

	private:
		EventHandler_PlayerController_Gamepad_Psp();
		~EventHandler_PlayerController_Gamepad_Psp();

		virtual bool OnEvent(Event& event);
	};
}
