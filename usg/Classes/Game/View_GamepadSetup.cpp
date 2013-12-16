#include "EventManager.h"
#include "GameState.h"
#include "MenuMgr.h"
#include "TempRender.h"
#include "Textures.h"
#include "View_GamepadSetup.h"

namespace Game
{
	void View_WinSetup::HandleFocus()
	{
		g_GameState->Interface_get()->ActiveMenu_set(GameMenu::MenuType_WinSetup);

		EventManager::I().Enable(EVENT_PRIO_JOYSETUP);
	}

	void View_WinSetup::HandleFocusLost()
	{
		EventManager::I().Disable(EVENT_PRIO_JOYSETUP);
	}

	void View_WinSetup::Render()
	{
		RenderRect(Vec2F(360.0f, 230.0f), SpriteColors::White, g_GameState->GetTexture(Textures::OPTIONSVIEW_GAMEPAD));
	}

	int View_WinSetup::RenderMask_get()
	{
		return RenderMask_Interface;
	}
}
