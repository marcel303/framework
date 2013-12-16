#include "EntityPlayer.h"
#include "EventHandler_PlayerController_Keyboard.h"
#include "EventManager.h"
#include "GameHelp.h"
#include "GameState.h"
#include "World.h"

namespace Game
{
	EventHandler_PlayerController_Keyboard& EventHandler_PlayerController_Keyboard::I()
	{
		static EventHandler_PlayerController_Keyboard instance;
		return instance;
	}

	EventHandler_PlayerController_Keyboard::EventHandler_PlayerController_Keyboard()
	{
		EventManager::I().AddEventHandler(this, EVENT_PRIO_KEYBOARD);

		Clear();
	}

	void EventHandler_PlayerController_Keyboard::UpdateMoveDir()
	{
		if (!mKeyboardX && !mKeyboardY)
			return;

		mMoveDir = Vec2F((float)mKeyboardX, (float)mKeyboardY).Normal();
	}

	Vec2F EventHandler_PlayerController_Keyboard::MoveDir_get() const
	{
		return mMoveDir;
	}

	bool EventHandler_PlayerController_Keyboard::OnEvent(Event& event)
	{
		bool result = false;

		switch (event.type)
		{
		case EVT_KEY:
			{
				switch (event.key.key)
				{
				case IK_LEFT:
				case IK_a:
					mMoveX[0] = event.key.state != 0;
					result = true;
					break;
				case IK_RIGHT:
				case IK_d:
					mMoveX[1] = event.key.state != 0;
					result = true;
					break;
				case IK_UP:
				case IK_w:
					mMoveY[0] = event.key.state != 0;
					result = true;
					break;
				case IK_DOWN:
				case IK_s:
					mMoveY[1] = event.key.state != 0;
					result = true;
					break;
				case IK_SHIFTL:
					if (event.key.state)
						g_World->m_Player->SwitchWeapons();
					result = true;
					break;
				case IK_ALTL:
					if (event.key.state)
						g_World->m_Player->SpecialAttack_Begin();
					result = true;
					break;
				case IK_SPACE:
					mKeyboardShockActive = event.key.state != 0;
					result = true;
					break;
				case IK_u:
					g_GameState->ActiveView_set(::View_Upgrade);
					g_GameState->m_HelpState->DoComplete(Game::HelpState::State_HitUpgrade);
					result = true;
					break;
				}	
			}
			break;
			
		default:
			break;
		}

		mKeyboardX = (mMoveX[0] ? -1 : 0) + (mMoveX[1] ? +1 : 0);
		mKeyboardY = (mMoveY[0] ? -1 : 0) + (mMoveY[1] ? +1 : 0);

		UpdateMoveDir();

		return result;
	}
}
