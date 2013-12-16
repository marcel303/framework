#include "EntityPlayer.h"
#include "EventHandler_PlayerController_Gamepad.h"
#include "EventManager.h"
#include "GameSettings.h"
#include "GameState.h"
#include "World.h"

namespace Game
{
	EventHandler_PlayerController_Gamepad& EventHandler_PlayerController_Gamepad::I()
	{
		static EventHandler_PlayerController_Gamepad instance;
		return instance;
	}

	void EventHandler_PlayerController_Gamepad::Initialize()
	{
		EventManager::I().AddEventHandler(this, EVENT_PRIO_JOYSTICK);

		Clear();
	}

	void EventHandler_PlayerController_Gamepad::Shutdown()
	{
		EventManager::I().RemoveEventHandler(this, EVENT_PRIO_JOYSTICK);
	}

	EventHandler_PlayerController_Gamepad::EventHandler_PlayerController_Gamepad()
	{
	}

	bool EventHandler_PlayerController_Gamepad::OnEvent(Event& event)
	{
		bool result = false;

		switch (event.type)
		{
		case EVT_JOYMOVE_ABS:
			{
				// todo: axis -> left / right stick mapping

				const float scale = 32700.0f;

				if (event.joy_move.axis == g_GameState->m_GameSettings->m_Axis_MoveX)
				{
					mJoyAxis_Move[0] = event.joy_move.value / scale;
					result = true;
				}
				if (event.joy_move.axis == g_GameState->m_GameSettings->m_Axis_MoveY)
				{
					mJoyAxis_Move[1] = event.joy_move.value / scale;
					result = true;
				}
				if (event.joy_move.axis == g_GameState->m_GameSettings->m_Axis_FireX)
				{
					mJoyAxis_Fire[0] = event.joy_move.value / scale;
					result = true;
				}
				if (event.joy_move.axis == g_GameState->m_GameSettings->m_Axis_FireY)
				{
					mJoyAxis_Fire[1] = event.joy_move.value / scale;
					result = true;
				}
			}
			break;

		case EVT_JOYBUTTON:
			{
				LOG(LogLevel_Warning, "joy button: %d", event.joy_button.button);

				if (event.joy_button.button == g_GameState->m_GameSettings->m_Button_UpgradeMenu)
				{
					if (event.joy_button.state)
						g_GameState->ActiveView_set(::View_Upgrade);

					result = true;
				}
				
				if (event.joy_button.button == g_GameState->m_GameSettings->m_Button_UseSpecial)
				{
					if (event.joy_button.state)
						g_World->m_Player->SpecialAttack_Begin();

					result = true;
				}

				if (event.joy_button.button == g_GameState->m_GameSettings->m_Button_UseShockwave)
				{
					if (event.joy_button.state)
						mJoyShockActive = true;

					result = true;
				}

				if (event.joy_button.button == g_GameState->m_GameSettings->m_Button_SwitchWeapons)
				{
					if (event.joy_button.state)
						g_World->m_Player->SwitchWeapons();

					result = true;
				}
			}
			break;
			
		default:
			break;
		}

		return result;
	}
}
