#pragma once

#include "Menu.h"

namespace GameMenu
{
	class Menu_CustomSettings : public Menu
	{
	public:
		Menu_CustomSettings();
		virtual ~Menu_CustomSettings();
		
		virtual void Init();
		virtual void HandleFocus();
		virtual void HandleBack();

	private:
		void Save();

		static void Handle_CustomStart(void* obj, void* arg);

		static void Handle_Boss_Toggle(void* obj, void* arg);
		static void Handle_Custom_Boss(void* obj, void* arg);
		static void Handle_Waves_Toggle(void* obj, void* arg);
		static void Handle_Mines_Toggle(void* obj, void* arg);
		static void Handle_Clutter_Toggle(void* obj, void* arg);

		static void Handle_PowerUp_Toggle(void* obj, void* arg);
		static void Handle_UpgradesUnlock_Toggle(void* obj, void* arg);
		static void Handle_Invuln_Toggle(void* obj, void* arg);
		static void Handle_InfLives_Toggle(void* obj, void* arg);

		static void Handle_LevelUp(void* obj, void* arg);
		static void Handle_LevelDown(void* obj, void* arg);

		/*bool Boss_Toggle;
		bool Waves_Toggle;
		bool Mines_Toggle;
		bool Clutter_Toggle;
		bool PowerUp_Toggle;
		bool UpgradesUnlock_Toggle;
		bool Invuln_Toggle;
		bool InfLives_Toggle;

		int Lives_Cnt;
		int WaveStartLevelCnt;
		int BossStartLevelCnt;*/
	};
}
