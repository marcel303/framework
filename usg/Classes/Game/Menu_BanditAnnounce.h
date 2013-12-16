#pragma once

#include "Menu.h"

namespace GameMenu
{
	class Menu_BanditAnnounce : public Menu
	{
	public:
		Menu_BanditAnnounce();
		virtual ~Menu_BanditAnnounce();
		
		virtual void Init();
		
	private:
		static void Handle_UpgradeMenu(void* obj, void* arg);
		static void Handle_Continue(void* obj, void* arg);
	};
}
