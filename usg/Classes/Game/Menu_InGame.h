#pragma once

#include "GameTypes.h"
#include "Menu.h"

namespace GameMenu
{
	class Menu_InGame : public Menu
	{
	public:
		Menu_InGame();
		virtual ~Menu_InGame();
		
		virtual void Init();
		virtual void Update(float dt);
		virtual bool Render();
		virtual void HandleFocus();
		virtual bool HandlePause();
		
		void HudMode_set(Game::HudMode mode);
		void HudOpacity_set(float value);
		
	private:
		static void Handle_Pause(void* obj, void* arg);
		static void Handle_CheatMenu(void* obj, void* arg);
		static void Handle_WeaponSwitch(void* obj, void* arg);
		static void Render_WeaponSwitch(void* obj, void* arg);
		static void Handle_Special(void* obj, void* arg);
		static void Render_Special(void* obj, void* arg);
		static void Handle_Upgrade(void* obj, void* arg);
		static void Handle_Screenshot(void* obj, void* arg);
		
		static void Render_Upgrade(void* obj, void* arg);
	public:
		static void Render_Upgrade(const Vec2F& pos, float opacity);
		
	private:
		float mParticleAngle;
	};
}
