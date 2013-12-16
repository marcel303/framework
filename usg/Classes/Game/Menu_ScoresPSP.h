#pragma once

#include "Menu.h"

namespace GameMenu
{
	class Menu_ScoresPSP : public Menu
	{
	public:
		virtual void Init();
		virtual void HandleFocus();
		virtual void HandleBack();
		virtual bool Render();

		void RefreshScoreView();

		static void Handle_GameMode_Prev(void* obj, void* arg);
		static void Handle_GameMode_Next(void* obj, void* arg);
		static void Handle_Difficulty_Prev(void* obj, void* arg);
		static void Handle_Difficulty_Next(void* obj, void* arg);
		static void Handle_GoBack(void* obj, void* arg);

		static void Handle_GameModeChanged(void* obj, void* arg);
		static void Handle_DifficultyChanged(void* obj, void* arg);
	};
}
