#include "GameState.h"
#include "GuiButton.h"
#include "Menu_Credits.h"
#include "UsgResources.h"
#include "View_Credits.h"

namespace GameMenu
{
	Menu_Credits::Menu_Credits() : Menu()
	{
	}
	
	Menu_Credits::~Menu_Credits()
	{
	}
	
	void Menu_Credits::Init()
	{
		SetTransition(TransitionEffect_Slide, Vec2F(0.0f, 100.0f), 0.5f);
		
		Button btn_GoBack;
		
		btn_GoBack.Setup_Shape(0, Vec2F(20.0f, VIEW_SY - 50.0f), g_GameState->GetShape(Resources::BUTTON_BACK), 0, CallBack(this, Handle_GoBack));
		
		AddButton(btn_GoBack);
	}

	void Menu_Credits::HandleBack()
	{
		Handle_GoBack(this, 0);
	}

	void Menu_Credits::Handle_GoBack(void* obj, void* arg)
	{
		Game::View_Credits* view = (Game::View_Credits*)g_GameState->GetView(View_Credits);
		
		g_GameState->ActiveView_set(view->m_NextView);
	}
}
