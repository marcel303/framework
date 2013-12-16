#include "GameState.h"
#include "MenuMgr.h"
#include "TempRender.h"
#include "Textures.h"
#include "UsgResources.h"
#include "View_Pause.h"

namespace Game
{
	// --------------------
	// View
	// --------------------

	void View_Pause::Initialize()
	{
		m_ScreenLock.Initialize("GAME PAUSED");
	}
	
	void View_Pause::HandleFocus()
	{
		m_ScreenLock.Start(true);
		
		g_GameState->Interface_get()->ActiveMenu_set(GameMenu::MenuType_Pause);
	}
	
	void View_Pause::HandleFocusLost()
	{
		m_ScreenLock.Start(false);
	}
	
	void View_Pause::Render()
	{
		m_ScreenLock.Render();
	}
	
	int View_Pause::RenderMask_get()
	{
		return RenderMask_Interface | RenderMask_Particles | RenderMask_WorldBackground | RenderMask_WorldPrimary;
	}
	
	float View_Pause::FadeTime_get()
	{
		return m_ScreenLock.FadeTime_get();
	}
}
