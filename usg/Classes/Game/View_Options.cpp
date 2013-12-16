#include "GameState.h"
#include "MenuMgr.h"
#include "TempRender.h"
#include "Textures.h"
#include "View_Options.h"

namespace Game
{
	View_Options::View_Options()
	{
	}
	
	View_Options::~View_Options()
	{
	}
	
	void View_Options::Initialize()
	{
		m_ScreenLock.Initialize("OPTIONS");
		
		m_NextView = ::View_Undefined;
	}
	
	void View_Options::Show(View nextView)
	{
		m_NextView = nextView;
		
		g_GameState->ActiveView_set(::View_Options);
	}
	
	// --------------------
	// View
	// --------------------

	void View_Options::Update(float dt)
	{
	}
	
	void View_Options::Render()
	{
		m_ScreenLock.Render();
	}
			
	int View_Options::RenderMask_get()
	{
		return RenderMask_Interface | RenderMask_Particles | RenderMask_WorldBackground;
	}
	
	float View_Options::FadeTime_get()
	{
		return m_ScreenLock.FadeTime_get();
	}
	
	void View_Options::HandleFocus()
	{
		g_GameState->Interface_get()->ActiveMenu_set(GameMenu::MenuType_Options);
		
		m_ScreenLock.Start(true);
	}
	
	void View_Options::HandleFocusLost()
	{
		m_ScreenLock.Start(false);
	}
}
