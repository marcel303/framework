#pragma once

#include "EventHandler.h"
#include "Menu.h"

namespace GameMenu
{
	enum SetupOption
	{
		SetupOption_Undefined,
		SetupOption_MoveX,
		SetupOption_MoveY,
		SetupOption_FireX,
		SetupOption_FireY,
		SetupOption_NavigateL,
		SetupOption_NavigateR,
		SetupOption_NavigateU,
		SetupOption_NavigateD,
		SetupOption_UseSpecial,
		SetupOption_UseShockwave,
		SetupOption_SwitchWeapons,
		SetupOption_UpgradeMenu,
		SetupOption_ButtonSelect,
		SetupOption_Dismiss,
		SetupOption_Select
	};

	class Menu_WinSetup : public Menu, public EventHandler
	{
	public:
		Menu_WinSetup();
		~Menu_WinSetup();

		virtual void Init();

		virtual void HandleFocus();
		virtual bool OnEvent(Event& event);
		virtual void HandleBack();

		static void Render_SetupButton(void* obj, void* arg);

		SetupOption mActiveOption;
	};
}
