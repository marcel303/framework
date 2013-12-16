/*
#include "GameState.h"
#include "UsgResources.h"
#include "TempRender.h"
#include "View_PauseTouch.h"

namespace Game
{
	void View_PauseTouch::HandleFocus()
	{
		// fixme: none
		
		g_GameState->Interface_get()->ActiveMenu_set(GameMenu::MenuType_Pause);
	}
	
	void View_PauseTouch::HandleFocusLost()
	{
	}
	
	void View_PauseTouch::Render()
	{
		RenderText(Vec2F(0.0f, 0.0f), Vec2F(480.0f, 320.0f), g_GameState->GetFont(Resources::FONT_CALIBRI_LARGE), SpriteColors::White, TextAlignment_Center, TextAlignment_Center, true, "Game Paused - Touch to Continue");
	}
	
	int View_PauseTouch::RenderMask_get()
	{
		return RenderMask_HudInfo | RenderMask_HudPlayer | RenderMask_Interface | RenderMask_Particles | RenderMask_WorldBackground | RenderMask_WorldPrimary;
	}
}
*/
