#include "GameSettings.h"
#include "GameState.h"
#include "GuiButton.h"
#include "GuiCheckbox.h"
#include "Menu_GameSelect.h"
#include "MenuRender.h"
#include "TempRender.h"
#include "Textures.h"
#include "UsgResources.h"
#include "View_GameSelect.h"

#if defined(BBOS)
	#define ENABLE_TUTORIAL 1
#else
	#define ENABLE_TUTORIAL 1
#endif

namespace GameMenu
{
	static DesignInfo design[] =
	{
		 DesignInfo("tutorial", true, Vec2F(20.0f, VIEW_SY - 33.0f), Vec2F(150.0f, 30.0f))
	};

	Menu_GameSelect::Menu_GameSelect() : Menu()
	{
	}
	
	Menu_GameSelect::~Menu_GameSelect()
	{
	}
	
	void Menu_GameSelect::Init()
	{
		m_PlayTutorial = true;
		
		SetTransition(TransitionEffect_Slide, Vec2F(0.0f, 100.0f), 0.5f);
		
		Button btn_DifficultyEasy;
		Button btn_DifficultyHard;
		Button btn_DifficultyCustom; // v1.3
		Button btn_ViewName;
		Button btn_ClickArea;
		
		//Vec2F s(96, 44);
		
		btn_DifficultyEasy.Setup_Shape(0, Vec2F(VIEW_SX / 2.0f - 50.0f, VIEW_SY / 2.0f - 22.0f), g_GameState->GetShape(Resources::MENU_GAMESELECT_EASY), 0, CallBack(this, Handle_DifficultyEasy));
		btn_DifficultyEasy.SetTransition(TransitionEffect_Slide, Vec2F(-(float)VIEW_SX, 0.0f));
		btn_DifficultyHard.Setup_Shape(0, Vec2F(VIEW_SX / 2.0f + 50.0f, VIEW_SY / 2.0f - 22.0f), g_GameState->GetShape(Resources::MENU_GAMESELECT_HARD), 0, CallBack(this, Handle_DifficultyHard));
		btn_DifficultyHard.SetTransition(TransitionEffect_Slide, Vec2F(+(float)VIEW_SX, 0.0f));
		btn_DifficultyCustom.Setup_Shape(0, Vec2F(VIEW_SX / 2.0f, VIEW_SY / 2.0f + 22.0f), g_GameState->GetShape(Resources::MENU_GAMESELECT_CUSTOM), 0, CallBack(this, Handle_DifficultyCustom));
		btn_DifficultyCustom.SetTransition(TransitionEffect_Slide, Vec2F(0.0f, -(float)VIEW_SY));

		btn_ClickArea.Setup_Custom(0, Vec2F(0.0f, 0.0f), Vec2F((float)VIEW_SX, (float)VIEW_SY), 0, CallBack(this, Handle_NextScreen), CallBack(this, Render_Empty));
		btn_ClickArea.SetHitEffect(HitEffect_None);
		btn_ClickArea.m_IsTouchOnly = true;
		
#if GAMESELECT_FANCY_SELECT == 0
		AddButton(btn_DifficultyEasy);
		AddButton(btn_DifficultyHard);
		AddButton(btn_DifficultyCustom);
#endif

		AddElement(new GuiCheckbox("tutorial", true, "tutorial", -1, CallBack(this, Handle_ToggleTutorial)));
		
		Button btn_Next;
		btn_Next.Setup_Shape(0, Vec2F(VIEW_SX - 73.0f, 37.0f), g_GameState->GetShape(Resources::MENU_GAMESELECT_HELP), 0, CallBack(this, Handle_NextScreen));
		btn_Next.SetTransition(TransitionEffect_Slide, Vec2F(0.0f, -100.0f));
		btn_Next.m_IsTouchOnly = true;
		AddButton(btn_Next);
		
#if /*!defined(USE_MENU_SELECT) &&*/ GAMESELECT_FANCY_SELECT == 0
		AddButton(btn_ClickArea);
#endif

		DESIGN_APPLY(this, design);

#if !ENABLE_TUTORIAL
		FindCheckbox("tutorial")->m_IsVisible = false;
#endif
	}

	void Menu_GameSelect::HandleFocus()
	{
		Menu::HandleFocus();

		m_PlayTutorial = g_GameState->m_GameSettings->m_PlayTutorial;

		FindCheckbox("tutorial")->IsChecked_set(m_PlayTutorial, false);
	}

	void Menu_GameSelect::HandleBack()
	{
		g_GameState->ActiveView_set(::View_Main);
	}

	void Menu_GameSelect::Save()
	{
		g_GameState->m_GameSettings->m_PlayTutorial = false;
		
		g_GameState->m_GameSettings->Save();
	}

	//
	
	static Game::GameMode GetGameMode(bool playTutorial)
	{
		if (playTutorial)
			return Game::GameMode_ClassicLearn;
		else
			return Game::GameMode_ClassicPlay;
	}
	
	void Menu_GameSelect::Handle_DifficultyEasy(void* obj, void* arg)
	{
		Menu_GameSelect* self = (Menu_GameSelect*)obj;
		
		self->Save();

#if !ENABLE_TUTORIAL
		self->m_PlayTutorial = false;
#endif

		g_GameState->GameBegin(GetGameMode(self->m_PlayTutorial), Difficulty_Easy, false);
	}
	
	void Menu_GameSelect::Handle_DifficultyHard(void* obj, void* arg)
	{
		Menu_GameSelect* self = (Menu_GameSelect*)obj;
		
		self->Save();

#if !ENABLE_TUTORIAL
		self->m_PlayTutorial = false;
#endif

		g_GameState->GameBegin(GetGameMode(self->m_PlayTutorial), Difficulty_Hard, false);
	}

	//v1.3

	void Menu_GameSelect::Handle_DifficultyCustom(void* obj, void* arg)
	{
		Menu_GameSelect* self = (Menu_GameSelect*)obj;
		
		self->Save();

#if !ENABLE_TUTORIAL
		self->m_PlayTutorial = false;
#endif

		g_GameState->ActiveView_set(View_CustomSettings);
	}

	//
	
	void Menu_GameSelect::Handle_ToggleTutorial(void* obj, void* arg)
	{
		Menu_GameSelect* self = (Menu_GameSelect*)obj;
		GuiCheckbox* element = (GuiCheckbox*)arg;
		
		self->m_PlayTutorial = !self->m_PlayTutorial;
		element->IsChecked_set(self->m_PlayTutorial, false);
	}
	
	void Menu_GameSelect::Handle_NextScreen(void* obj, void* arg)
	{
		Game::View_GameSelect* view = (Game::View_GameSelect*)g_GameState->GetView(::View_GameSelect);
		
		view->NextSlide(true);
	}
}
