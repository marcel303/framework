#pragma once

#include "Menu.h"

namespace GameMenu
{
	class Menu_Credits : public Menu
	{
	public:
		Menu_Credits();
		virtual ~Menu_Credits();
		
		virtual void Init();
		virtual void HandleBack();

	private:
		static void Handle_GoBack(void* obj, void* arg);
	};
}
