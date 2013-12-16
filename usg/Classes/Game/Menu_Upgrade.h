#pragma once

#include "AnimTimer.h"
#include "Menu.h"

namespace GameMenu
{
	class Menu_Upgrade : public Menu
	{
	public:
		Menu_Upgrade();
		virtual ~Menu_Upgrade();
		
		virtual void Init();
		virtual void HandleFocus();
		virtual void HandleBack();
		virtual bool Render();
		
	private:
		static void Handle_Upgrade(void* obj, void* arg);
		static void Handle_GoBack(void* obj, void* arg);
		static void Handle_RenderUpgrade(void* obj, void* arg);
		
		const char* mDenyText;
		AnimTimer mDenyAnim;
	};

}
