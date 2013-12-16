#include "EventHandler_PlayerController_Gamepad_Psp.h"
#include "EventManager.h"
#include "EntityPlayer.h"
#include "GameState.h"
#include "World.h"

#define MAP_UPGRADE      INPUT_BUTTON_PSP_TRIANGLE
#define MAP_SPECIAL      INPUT_BUTTON_PSP_TOPRIGHT
#define MAP_SHOCKWAVE    INPUT_BUTTON_PSP_TOPLEFT
#define MAP_WEAPONSWITCH INPUT_BUTTON_PSP_CIRCLE

namespace Game
{
	EventHandler_PlayerController_Gamepad_Psp& EventHandler_PlayerController_Gamepad_Psp::I()
	{
		static EventHandler_PlayerController_Gamepad_Psp instance;
		return instance;
	}

	EventHandler_PlayerController_Gamepad_Psp::EventHandler_PlayerController_Gamepad_Psp()
	{
		EventManager::I().AddEventHandler(this, EVENT_PRIO_JOYSTICK);

		Clear();
	}

	bool EventHandler_PlayerController_Gamepad_Psp::OnEvent(Event& event)
	{
		bool result = false;

		switch (event.type)
		{
		case EVT_JOYMOVE_ABS:
			{
				// todo: axis -> left / right stick mapping

				const float scale = 32700.0f;

				if (event.joy_move.axis == INPUT_AXIS_X)
				{
					mJoyAxis_Move[0] = event.joy_move.value / scale;
					result = true;
				}
				if (event.joy_move.axis == INPUT_AXIS_Y)
				{
					mJoyAxis_Move[1] = event.joy_move.value / scale;
					result = true;
				}
				if (event.joy_move.axis == 0 && 0)
				{
					mJoyAxis_Fire[0] = event.joy_move.value / scale;
					result = true;
				}
				if (event.joy_move.axis == 0 && 0)
				{
					mJoyAxis_Fire[1] = event.joy_move.value / scale;
					result = true;
				}
			}
			break;

		case EVT_JOYBUTTON:
			{
				if (event.joy_button.button == MAP_UPGRADE)
				{
					if (event.joy_button.state)
						g_GameState->ActiveView_set(::View_Upgrade);

					result = true;
				}

				if (event.joy_button.button == MAP_SPECIAL)
				{
					if (event.joy_button.state)
						g_World->m_Player->SpecialAttack_Begin();

					result = true;
				}

				if (event.joy_button.button == MAP_SHOCKWAVE)
				{
					if (event.joy_button.state)
						mJoyShockActive = true;

					result = true;
				}

				if (event.joy_button.button == MAP_WEAPONSWITCH)
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
