#include "GameState.h"
#include "MenuMgr.h"
#include "TempRender.h"
#include "Textures.h"
#include "UsgResources.h"
#include "View_ScoreEntry.h"
#include "World.h"

namespace Game
{
	View_ScoreEntry::View_ScoreEntry()
	{
	}
	
	void View_ScoreEntry::Initialize()
	{
		m_BackAnim.Initialize(g_GameState->m_TimeTracker_Global, false);
	}
	
	void View_ScoreEntry::Update(float dt)
	{
	}
	
	void View_ScoreEntry::Render()
	{
		// render background
		
		float a = 1.0f - m_BackAnim.Progress_get();
		
		RenderRect(Vec2F(0.0f, 0.0f), Vec2F((float)VIEW_SX, (float)VIEW_SY), 0.0f, 0.0f, 0.0f, a, g_GameState->GetTexture(Textures::COLOR_BLACK));
		
		RenderText(Vec2F(20.0f, 60.0f), Vec2F(), g_GameState->GetFont(Resources::FONT_CALIBRI_LARGE), SpriteColors::White, TextAlignment_Left, TextAlignment_Top, true, "Highscore submission");
	}
	
	void View_ScoreEntry::HandleFocus()
	{
		g_GameState->Interface_get()->ActiveMenu_set(GameMenu::MenuType_ScoreEntry);
		
		g_World->IsPaused_set(true);
		
		m_BackAnim.Start(AnimTimerMode_TimeBased, true, 0.5f, AnimTimerRepeat_None);
	}
	
	int View_ScoreEntry::RenderMask_get()
	{
		return RenderMask_Interface | RenderMask_Particles | RenderMask_WorldBackground;
	}
}
