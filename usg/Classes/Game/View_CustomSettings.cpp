#include "GameState.h"
#include "MenuMgr.h"
#include "ResIO.h"
#include "TempRender.h"
#include "Textures.h"
#include "UsgResources.h"
#include "View_CustomSettings.h"

namespace Game
{
	View_CustomSettings::View_CustomSettings()
	{
	}
	
	View_CustomSettings::~View_CustomSettings()
	{
	}
	
	void View_CustomSettings::Initialize()
	{
		m_ScreenLock.Initialize("");
	}
	
	void View_CustomSettings::HandleFocus()
	{
		m_IsActive = true;
		
		g_GameState->Interface_get()->ActiveMenu_set(GameMenu::MenuType_CustomSettings);
		
		m_ScreenLock.Start(true);
	}
	
	void View_CustomSettings::HandleFocusLost()
	{
		m_IsActive = false;
		
		m_ScreenLock.Start(false);
	}
	
	float View_CustomSettings::FadeTime_get()
	{
		return m_ScreenLock.FadeTime_get();
	}
	
	void View_CustomSettings::Update(float dt)
	{
	}
	
	void View_CustomSettings::Render()
	{
		m_ScreenLock.Render();
	}
	
	int View_CustomSettings::RenderMask_get()
	{
		return RenderMask_Interface | RenderMask_Particles | RenderMask_WorldBackground;
	}

	void View_CustomSettings::Show(View nextView)
	{
		m_NextView = nextView;
		
		g_GameState->ActiveView_set(::View_Options);
	}




}
