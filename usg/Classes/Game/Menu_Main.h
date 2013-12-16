#pragma once

#include "AnimTimer.h"
#include "Menu.h"

namespace GameMenu
{
	class Menu_Main : public Menu
	{
	public:
		Menu_Main();
		virtual ~Menu_Main();
		
		virtual void Init();
		
		virtual void HandleFocus();
		
	private:
		bool m_HasSave;
		AnimTimer m_ContinueAnim;
		
		static void Handle_RenderPlay(void* obj, void* arg);
		
		static void Handle_New(void* obj, void* arg);
		static void Handle_NewGame(void* obj, void* arg);
		static void Handle_Store(void* obj, void* arg);
		static void Handle_Credits(void* obj, void* arg);
		static void Handle_GameCenter(void* obj, void* arg);
		static void Handle_Options(void* obj, void* arg);
		static void Handle_Scores(void* obj, void* arg);
	};
}
