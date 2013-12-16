#pragma once

#include "Menu.h"

namespace GameMenu
{
	enum CheatType
	{
		CheatType_SpawnMiniBoss,
		CheatType_SpawnEnemies,
		CheatType_SpawnBandit,
		CheatType_Cheat_Invincibility,
		CheatType_Cheat_PowerUp,
		CheatType_Spawn_Circle,
		CheatType_Spawn_Cluster,
		CheatType_Spawn_Line,
		CheatType_Spawn_PlayerPos,
		CheatType_Spawn_Prepared_Line,
		CheatType_Player_Kill,
		CheatType_Player_Credits,
		CheatType_Player_Upgrade,
		CheatType_Round_LevelUp,
		CheatType_Round_LevelDown,
		CheatType_Render_BG,
		CheatType_Render_GameOnly
	};
	
	class Menu_Cheats : public Menu
	{
	public:
		Menu_Cheats();
		virtual void Init();
		
	private:
		static void Handle_Cheat(void* obj, void* arg);
		static void Handle_SpawnMiniBoss(void* obj, void* arg);
		static void Handle_SpawnBandit(void* obj, void* arg);
		static void Handle_GoBack(void* obj, void* arg);
	};
}
