#pragma once

#include "Menu.h"

namespace GameMenu
{
	class Menu_GameSelect : public Menu
	{
	public:
		Menu_GameSelect();
		virtual ~Menu_GameSelect();
		
		virtual void Init();
		virtual void HandleFocus();
		virtual void HandleBack();

	private:
		void Save();

		static void Handle_DifficultyEasy(void* obj, void* arg);
		static void Handle_DifficultyHard(void* obj, void* arg);
		static void Handle_ToggleTutorial(void* obj, void* arg);
		static void Handle_NextScreen(void* obj, void* arg);

		bool m_PlayTutorial;

		//v1.3
		static void Handle_DifficultyCustom(void* obj, void* arg);
	};
}
