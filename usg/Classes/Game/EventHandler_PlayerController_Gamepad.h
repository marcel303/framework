#pragma once

#include "EventHandler.h"
#include "Types.h"

namespace Game
{
	class EventHandler_PlayerController_Gamepad : public EventHandler
	{
	public:
		static EventHandler_PlayerController_Gamepad& I();

		void Initialize();
		void Shutdown();

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
		EventHandler_PlayerController_Gamepad();

		virtual bool OnEvent(Event& event);
	};
}
