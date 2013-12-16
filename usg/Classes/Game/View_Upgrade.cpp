#include "GameSettings.h"
#include "GameState.h"
#include "MenuMgr.h"
#include "TempRender.h"
#include "Textures.h"
#include "View_Upgrade.h"

namespace Game
{
	View_Upgrade::View_Upgrade()
	{
		Initialize();
	}
	
	void View_Upgrade::Initialize()
	{
		mIsActive = false;
		
		mScreenLock.Initialize("UPGRADE");
	}
	
	// --------------------
	// View related
	// --------------------
	
	void View_Upgrade::Update(float dt)
	{
	}
	
	void View_Upgrade::Render()
	{
		if (mIsActive)
		{
//			Vec2F backP1(10.0f, 17.0f);
//			Vec2F backP2(470.0f, 277.0f);
			Vec2F backP1(10.0f, 45.0f);
			Vec2F backP2(VIEW_SX - 10.0f, VIEW_SY - 45.0f);
			Vec2F backSize = backP2 - backP1;
			
//			RenderRect(backP1, backSize, 0.0f, 0.0f, 0.0f, 0.25f, g_GameState->GetTexture(Textures::COLOR_BLACK));
			RenderRect(Vec2F(0.0f, 0.0f), Vec2F((float)VIEW_SX, (float)VIEW_SY), 0.0f, 0.0f, 0.0f, 0.3f, g_GameState->GetTexture(Textures::COLOR_BLACK));
		}
		
		mScreenLock.Render();
	}
	
	int View_Upgrade::RenderMask_get()
	{
//		return RenderMask_Interface | RenderMask_Particles | RenderMask_WorldBackground | RenderMask_WorldPrimary;
		return RenderMask_Interface | RenderMask_Particles | RenderMask_WorldBackground;
	}
	
	void View_Upgrade::HandleFocus()
	{
		g_GameState->Interface_get()->ActiveMenu_set(GameMenu::MenuType_Upgrade);
		
		mScreenLock.Start(true);
		
		mIsActive = true;
	}
	
	void View_Upgrade::HandleFocusLost()
	{
		mScreenLock.Start(false);
		
		mIsActive = false;
	}
	
	float View_Upgrade::FadeTime_get()
	{
		return mScreenLock.FadeTime_get();
	}
}
