#include "GameSettings.h"
#include "GameState.h"
#include "GuiButton.h"
#include "Menu_BanditAnnounce.h"
#include "UsgResources.h"
#include "World.h"

namespace GameMenu
{
	static void RenderContinue(void* obj, void* arg)
	{
	}
	
	Menu_BanditAnnounce::Menu_BanditAnnounce() : Menu()
	{
	}
	
	Menu_BanditAnnounce::~Menu_BanditAnnounce()
	{
	}
	
	void Menu_BanditAnnounce::Init()
	{
		SetTransition(TransitionEffect_Fade, Vec2F(0.0f, 100.0f), 0.5f);
		
		Button btn_Continue = Button::Make_Custom(0, Vec2F(0.0f, 0.0f), Vec2F((float)VIEW_SX, (float)VIEW_SY), 0, CallBack(this, Handle_Continue), CallBack(this, RenderContinue));
		btn_Continue.SetHitEffect(HitEffect_None);
		AddButton(btn_Continue);
	}
	
	void Menu_BanditAnnounce::Handle_UpgradeMenu(void* obj, void* arg)
	{
		g_GameState->ActiveView_set(View_Upgrade);
	}
				  
	void Menu_BanditAnnounce::Handle_Continue(void* obj, void* arg)
	{
		g_GameState->ActiveView_set(View_InGame);
	}
}
