#pragma once

#include "EventHandler.h"
#include "Types.h"

namespace Game
{
	class EventHandler_PlayerController_Keyboard : public EventHandler
	{
	public:
		static EventHandler_PlayerController_Keyboard& I();

		void Clear()
		{
			mMoveX[0] = false;
			mMoveX[1] = false;
			mMoveY[0] = false;
			mMoveY[1] = false;
			
			mKeyboardX = 0;
			mKeyboardY = 0;
			//mKeyboardFireActive = false;
			//mKeyboardSpecialActive = false;
			mKeyboardShockActive = false;
		}

		void UpdateMoveDir();
		Vec2F MoveDir_get() const;

		int mKeyboardX;
		int mKeyboardY;
		Vec2F mMoveDir;
		//bool mKeyboardFireActive;
		//bool mKeyboardSpecialActive;
		bool mKeyboardShockActive;

	private:
		bool mMoveX[2];
		bool mMoveY[2];

		EventHandler_PlayerController_Keyboard();

		virtual bool OnEvent(Event& event);
	};
}
