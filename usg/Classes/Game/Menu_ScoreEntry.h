#pragma once

#include "Menu.h"

namespace GameMenu
{
	class Menu_ScoreEntry : public Menu
	{
	public:
		virtual void Init();
		virtual void HandleFocus();

		static void Handle_Accept(void* obj, void* arg);
		static void Handle_Dismiss(void* obj, void* arg);
		static void Handle_ToggleSubmitOnline(void* obj, void* arg);
		static void Handle_ToggleSubmitLocal(void* obj, void* arg);
		
#if defined(IPHONEOS) || defined(BBOS)
		bool m_useGameCenter;
#endif
	};
}
