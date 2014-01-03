#include "EventManager.h"
#include "Exception.h"
#include "GameSettings.h"
#include "GameState.h"
#include "GuiButton.h"
#include "Menu_GamepadSetup.h"
#include "StringBuilder.h"
#include "TempRender.h"
#include "Textures.h"
#include "UsgResources.h"

namespace GameMenu
{
	static void HandleClick(void* obj, void* arg);
	static void HandleAccept(void* obj, void* arg);
	static void HandleDismiss(void* obj, void* arg);

	Menu_WinSetup::Menu_WinSetup() : Menu()
	{
		mActiveOption = SetupOption_Undefined;
	}
	
	Menu_WinSetup::~Menu_WinSetup()
	{
		EventManager::I().RemoveEventHandler(this, EVENT_PRIO_JOYSETUP);
	}

	void Menu_WinSetup::Init()
	{
		SetTransition(TransitionEffect_Slide, Vec2F(0.0f, 100.0f), 0.5f);

		float sx = 200.0f;
		float dx = sx + 5.0f;

		float sy = 25;
		float dy = sy + 5.0f;

		float x = -dx + 10.0f;
		float y = -dy + 5.0f;

		x  += dx;

		AddButton(Button::Make_Custom("Fire X axis", Vec2F(x, y += dy), Vec2F(200.0f, sy), SetupOption_FireX, CallBack(this, HandleClick), CallBack(this, Render_SetupButton)));
		AddButton(Button::Make_Custom("Fire Y axis", Vec2F(x, y += dy), Vec2F(200.0f, sy), SetupOption_FireY, CallBack(this, HandleClick), CallBack(this, Render_SetupButton)));
		AddButton(Button::Make_Custom("Move X axis", Vec2F(x, y += dy), Vec2F(200.0f, sy), SetupOption_MoveX, CallBack(this, HandleClick), CallBack(this, Render_SetupButton)));
		AddButton(Button::Make_Custom("Move Y axis", Vec2F(x, y += dy), Vec2F(200.0f, sy), SetupOption_MoveY, CallBack(this, HandleClick), CallBack(this, Render_SetupButton)));
		AddButton(Button::Make_Custom("D-Pad Left", Vec2F(x, y += dy), Vec2F(200.0f, sy), SetupOption_NavigateL, CallBack(this, HandleClick), CallBack(this, Render_SetupButton)));
		AddButton(Button::Make_Custom("D-Pad Right", Vec2F(x, y += dy), Vec2F(200.0f, sy), SetupOption_NavigateR, CallBack(this, HandleClick), CallBack(this, Render_SetupButton)));
		AddButton(Button::Make_Custom("D-Pad Up", Vec2F(x, y += dy), Vec2F(200.0f, sy), SetupOption_NavigateU, CallBack(this, HandleClick), CallBack(this, Render_SetupButton)));
		AddButton(Button::Make_Custom("D-Pad Down", Vec2F(x, y += dy), Vec2F(200.0f, sy), SetupOption_NavigateD, CallBack(this, HandleClick), CallBack(this, Render_SetupButton)));

		x  += dx;
		y = -dy + 5.0f;

		AddButton(Button::Make_Custom("Switch weapon", Vec2F(x, y += dy), Vec2F(200.0f, sy), SetupOption_SwitchWeapons, CallBack(this, HandleClick), CallBack(this, Render_SetupButton)));
		AddButton(Button::Make_Custom("Shockwave", Vec2F(x, y += dy), Vec2F(200.0f, sy), SetupOption_UseShockwave, CallBack(this, HandleClick), CallBack(this, Render_SetupButton)));
		AddButton(Button::Make_Custom("Special", Vec2F(x, y += dy), Vec2F(200.0f, sy), SetupOption_UseSpecial, CallBack(this, HandleClick), CallBack(this, Render_SetupButton)));
		AddButton(Button::Make_Custom("OK", Vec2F(x, y += dy), Vec2F(200.0f, sy), SetupOption_ButtonSelect, CallBack(this, HandleClick), CallBack(this, Render_SetupButton)));
		AddButton(Button::Make_Custom("Cancel", Vec2F(x, y += dy), Vec2F(200.0f, sy), SetupOption_Dismiss, CallBack(this, HandleClick), CallBack(this, Render_SetupButton)));
		AddButton(Button::Make_Custom("Upgrade menu", Vec2F(x, y += dy), Vec2F(200.0f, sy), SetupOption_UpgradeMenu, CallBack(this, HandleClick), CallBack(this, Render_SetupButton)));
		AddButton(Button::Make_Custom("Pause/Select", Vec2F(x, y += dy), Vec2F(200.0f, sy), SetupOption_Select, CallBack(this, HandleClick), CallBack(this, Render_SetupButton)));

		AddButton(Button::Make_Shape(0, Vec2F(10.0f, 280.0f), g_GameState->GetShape(Resources::OPTIONSVIEW_ACCEPT), 0, CallBack(this, HandleAccept)));
		AddButton(Button::Make_Shape(0, Vec2F(110.0f, 280.0f), g_GameState->GetShape(Resources::OPTIONSVIEW_DISMISS), 0, CallBack(this, HandleDismiss)));

		EventManager::I().AddEventHandler(this, EVENT_PRIO_JOYSETUP);

		Translate((VIEW_SX-480)/2, (VIEW_SY-320)/2);
	}

	void Menu_WinSetup::HandleBack()
	{		
		HandleDismiss(this, 0);
	}
	
	void Menu_WinSetup::HandleFocus()
	{
		Menu::HandleFocus();
		
		mActiveOption = SetupOption_Undefined;
	}

	bool Menu_WinSetup::OnEvent(Event& event)
	{
		if (!IsActive_get())
			return false;

		if (event.type == EVT_JOYBUTTON && event.joy_button.state)
		{
			int button = event.joy_button.button;

			switch (mActiveOption)
			{
			case SetupOption_NavigateL:
				g_GameState->m_GameSettings->m_Button_NavigateL = button;
				mActiveOption = SetupOption_Undefined;
				return true;
			case SetupOption_NavigateR:
				g_GameState->m_GameSettings->m_Button_NavigateR = button;
				mActiveOption = SetupOption_Undefined;
				return true;
			case SetupOption_NavigateU:
				g_GameState->m_GameSettings->m_Button_NavigateU = button;
				mActiveOption = SetupOption_Undefined;
				return true;
			case SetupOption_NavigateD:
				g_GameState->m_GameSettings->m_Button_NavigateD = button;
				mActiveOption = SetupOption_Undefined;
				return true;
			case SetupOption_UseShockwave:
				g_GameState->m_GameSettings->m_Button_UseShockwave = button;
				mActiveOption = SetupOption_Undefined;
				return true;
			case SetupOption_UseSpecial:
				g_GameState->m_GameSettings->m_Button_UseSpecial = button;
				mActiveOption = SetupOption_Undefined;
				return true;
			case SetupOption_SwitchWeapons:
				g_GameState->m_GameSettings->m_Button_SwitchWeapons = button;
				mActiveOption = SetupOption_Undefined;
				return true;
			case SetupOption_UpgradeMenu:
				g_GameState->m_GameSettings->m_Button_UpgradeMenu = button;
				mActiveOption = SetupOption_Undefined;
				return true;
			case SetupOption_ButtonSelect:
				g_GameState->m_GameSettings->m_Button_MenuButtonSelect = button;
				mActiveOption = SetupOption_Undefined;
				return true;
			case SetupOption_Dismiss:
				g_GameState->m_GameSettings->m_Button_Dismiss = button;
				mActiveOption = SetupOption_Undefined;
				return true;
			case SetupOption_Select:
				g_GameState->m_GameSettings->m_Button_Select = button;
				mActiveOption = SetupOption_Undefined;
				return true;
			default:
#ifndef DEPLOYMENT
				//throw ExceptionVA("unknown option: %d", mActiveOption);
				break;
#else
				break;
#endif
			}
		}
		if (event.type == EVT_JOYMOVE_ABS)
		{
			if (Calc::Abs(event.joy_move.value) < 20000)
				return false;

			int axis = event.joy_move.axis;

			switch (mActiveOption)
			{
			case SetupOption_FireX:
				g_GameState->m_GameSettings->m_Axis_FireX = axis;
				mActiveOption = SetupOption_Undefined;
				return true;
			case SetupOption_FireY:
				g_GameState->m_GameSettings->m_Axis_FireY = axis;
				mActiveOption = SetupOption_Undefined;
				return true;
			case SetupOption_MoveX:
				g_GameState->m_GameSettings->m_Axis_MoveX = axis;
				mActiveOption = SetupOption_Undefined;
				return true;
			case SetupOption_MoveY:
				g_GameState->m_GameSettings->m_Axis_MoveY = axis;
				mActiveOption = SetupOption_Undefined;
				return true;
			default:
#ifndef DEPLOYMENT
				//throw ExceptionNA();
				break;
#else
				break;
#endif
			}
		}

		return false;
	}

	static void HandleClick(void* obj, void* arg)
	{
		Menu_WinSetup* self = (Menu_WinSetup*)obj;
		Button* button = (Button*)arg;

		self->mActiveOption = (SetupOption)button->m_Info;
	}

	static void HandleAccept(void* obj, void* arg)
	{
		g_GameState->ActiveView_set(View_Main);

		g_GameState->m_GameSettings->Save();
	}

	static void HandleDismiss(void* obj, void* arg)
	{
		g_GameState->ActiveView_set(View_Main);

		g_GameState->m_GameSettings->Load();
	}

	void Menu_WinSetup::Render_SetupButton(void* obj, void* arg)
	{
		Menu_WinSetup* self = (Menu_WinSetup*)obj;
		Button* button = (Button*)arg;

		float a = 0.5f;

		SetupOption option = (SetupOption)button->m_Info;

		if (self->mActiveOption == option)
			a = 1.0f;

		RenderRect(button->Position_get(), button->Size_get(), 0.5f, 0.5f, 0.5f, a, g_GameState->GetTexture(Textures::MENU_COLOR_WHITE));

		int value = -1;

		switch (option)
		{
		case SetupOption_FireX:
			value = g_GameState->m_GameSettings->m_Axis_FireX;
			break;
		case SetupOption_FireY:
			value = g_GameState->m_GameSettings->m_Axis_FireY;
			break;
		case SetupOption_MoveX:
			value = g_GameState->m_GameSettings->m_Axis_MoveX;
			break;
		case SetupOption_MoveY:
			value = g_GameState->m_GameSettings->m_Axis_MoveY;
			break;
		case SetupOption_NavigateL:
			value = g_GameState->m_GameSettings->m_Button_NavigateL;
			break;
		case SetupOption_NavigateR:
			value = g_GameState->m_GameSettings->m_Button_NavigateR;
			break;
		case SetupOption_NavigateU:
			value = g_GameState->m_GameSettings->m_Button_NavigateU;
			break;
		case SetupOption_NavigateD:
			value = g_GameState->m_GameSettings->m_Button_NavigateD;
			break;
		case SetupOption_UseShockwave:
			value = g_GameState->m_GameSettings->m_Button_UseShockwave;
			break;
		case SetupOption_UseSpecial:
			value = g_GameState->m_GameSettings->m_Button_UseSpecial;
			break;
		case SetupOption_SwitchWeapons:
			value = g_GameState->m_GameSettings->m_Button_SwitchWeapons;
			break;
		case SetupOption_ButtonSelect:
			value = g_GameState->m_GameSettings->m_Button_MenuButtonSelect;
			break;
		case SetupOption_Dismiss:
			value = g_GameState->m_GameSettings->m_Button_Dismiss;
			break;
		case SetupOption_UpgradeMenu:
			value = g_GameState->m_GameSettings->m_Button_UpgradeMenu;
			break;
		case SetupOption_Select:
			value = g_GameState->m_GameSettings->m_Button_Select;
			break;
		default:
			throw ExceptionNA();
		}

		RenderText(button->Position_get(), button->Size_get() ^ Vec2F(1.0f, 0.5f), g_GameState->GetFont(Resources::FONT_CALIBRI_SMALL), SpriteColors::White, TextAlignment_Center, TextAlignment_Center, true, button->m_Name);
		StringBuilder<32> sb;
		sb.AppendFormat("assignment: %d", value);
		RenderText(button->Position_get() + (button->Size_get() ^ Vec2F(0.0f, 0.5f)), button->Size_get() ^ Vec2F(1.0f, 0.5f), g_GameState->GetFont(Resources::FONT_CALIBRI_SMALL), SpriteColors::White, TextAlignment_Center, TextAlignment_Center, true, sb.ToString());
	}
}
