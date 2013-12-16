#pragma once

#include "Menu.h"

namespace GameMenu
{
	
	class Menu_OptionsAdvanced : public Menu
	{
	public:
		virtual void Init();
		virtual void HandleFocus();
		virtual void HandleBack();
		virtual bool Render();

		static void Handle_ControllerMode_Move(void* obj, void* arg);
		static void Handle_ControllerMode_Fire(void* obj, void* arg);
		//static void Handle_HudMode(void* obj, void* arg);
		static void Handle_HudOpacity(void* obj, void* arg);
		static void Handle_Camera3dEnabled(void* obj, void* arg);
		static void Handle_ControllerType_Toggle(void* obj, void* arg);
		
		static void Handle_Credits(void* obj, void* arg);
		static void Handle_Basic(void* obj, void* arg);
		static void Handle_Accept(void* obj, void* arg);
		static void Handle_Dismiss(void* obj, void* arg);

//		static void Handle_Back(void* obj, void* arg);
	};
}
