#pragma once

#include "Menu.h"

namespace GameMenu
{
	class Menu_Scores : public Menu
	{
	public:
		Menu_Scores();
		virtual ~Menu_Scores();
		
		virtual void Init();
		virtual void HandleFocus();
		virtual void HandleBack();
		virtual bool Render();
		
#ifdef IPHONEOS
		void Handle_GameCenterLogin();
#endif
		
	private:
		void UpdateScoreList();
		
		static void Handle_SelectMode_Arcade(void* obj, void* arg);
		static void Handle_SelectMode_TimeAttack(void* obj, void* arg);
		static void Handle_SelectDifficulty_Easy(void* obj, void* arg);
		static void Handle_SelectDifficulty_Hard(void* obj, void* arg);
		static void Handle_SelectDifficulty_Custom(void* obj, void* arg);
		static void Handle_History(void* obj, void* arg);
		static void Handle_FindMe(void* obj, void* arg);
		static void Handle_SelectArea_Global(void* obj, void* arg);
		static void Handle_SelectArea_Local(void* obj, void* arg);
		static void Handle_Refresh(void* obj, void* arg);
		static void Handle_GoBack(void* obj, void* arg);

		Difficulty m_Difficulty;
	};
}
