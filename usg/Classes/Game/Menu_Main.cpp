#include "GameCenter.h"
#include "GameSave.h"
#include "GameSettings.h"
#include "GameState.h"
#include "GuiButton.h"
#include "Menu_Main.h"
#include "UsgResources.h"
#include "View_Credits.h"
#include "View_Options.h"

namespace GameMenu
{
	static DesignInfo design[] =
	{
		DesignInfo("new",        true, Vec2F(VIEW_SX / 2.0f,   0              ), Vec2F()),
		DesignInfo("new_game",   true, Vec2F(VIEW_SX / 2.0f,   VIEW_SY / 2.0f ), Vec2F()),
#if defined(IPAD)
		DesignInfo("store",      false, Vec2F(25.0f,            VIEW_SY - 60.0f), Vec2F()),
#endif
		DesignInfo("options",    true, Vec2F(VIEW_SX - 70.0f,  VIEW_SY - 60.0f), Vec2F()),
		DesignInfo("scores",     true, Vec2F(VIEW_SX - 198.0f, VIEW_SY - 60.0f), Vec2F()),
#if defined(BBOS)
		DesignInfo("credits",    true, Vec2F(15.0f,            VIEW_SY - 50.0f), Vec2F()),
#elif defined(IPAD)
		DesignInfo("gamecenter", true, Vec2F(20.0f,            VIEW_SY - 60.0f), Vec2F()),
#endif
	};

	Menu_Main::Menu_Main() : Menu()
	{
		m_HasSave = false;
	}
	
	Menu_Main::~Menu_Main()
	{
	}
	
	void Menu_Main::Init()
	{
		m_ContinueAnim.Initialize(g_GameState->m_TimeTracker_Global, false);
		
		SetTransition(TransitionEffect_Slide, Vec2F(0.0f, 100.0f), 0.5f);
		
		Button btn_New;
		Button btn_NewGame;
#if defined(IPAD)
		Button btn_Store;
#endif
		Button btn_Options;
		Button btn_Scores;
		
		btn_New.Setup_Shape(    "new",      Vec2F(), g_GameState->GetShape(Resources::MENU_MAIN_NEW),       0, CallBack(this, Handle_New)    );
		btn_New.SetTransition(TransitionEffect_Slide, Vec2F(0.0f, -50.0f));
		btn_NewGame.Setup_Shape("new_game", Vec2F(), g_GameState->GetShape(Resources::MENU_MAIN_START),     0, CallBack(this, Handle_NewGame));
		btn_NewGame.SetTransition(TransitionEffect_Fade, Vec2F());
		btn_NewGame.OnRender = CallBack(this, Handle_RenderPlay);
#if defined(IPAD)
		btn_Store.Setup_Shape("store",  Vec2F(), g_GameState->GetShape(Resources::MAINVIEW_OPTIONS),    0, CallBack(this, Handle_Store));
#endif
		btn_Scores.Setup_Shape( "scores",   Vec2F(), g_GameState->GetShape(Resources::MAINVIEW_HIGHSCORES), 0, CallBack(this, Handle_Scores) );
		btn_Options.Setup_Shape("options",  Vec2F(), g_GameState->GetShape(Resources::MAINVIEW_OPTIONS),    0, CallBack(this, Handle_Options));
		
		AddButton(btn_NewGame);
		AddButton(btn_New);
#if defined(IPAD)
		AddButton(btn_Store);
#endif
#if defined(BBOS)
		AddButton(Button::Make_Shape("credits", Vec2F(360.0f, 205.0f), g_GameState->GetShape(Resources::OPTIONSVIEW_CREDITS), 0, CallBack(this, Handle_Credits)));
#elif defined(IPAD)
		AddButton(Button::Make_Shape("gamecenter", Vec2F(), g_GameState->GetShape(Resources::MAINVIEW_GAMECENTER), 0, CallBack(this, Handle_GameCenter)));
#endif
		AddButton(btn_Scores);
		AddButton(btn_Options);

		DESIGN_APPLY(this, design);
 	}
	
	void Menu_Main::HandleFocus()
	{
		m_HasSave = g_GameState->m_GameSave->HasSave_get();
		
		FindButton("new")->m_IsVisible = m_HasSave;
		
		m_ContinueAnim.Start(AnimTimerMode_TimeBased, false, 0.5f, AnimTimerRepeat_Loop);

		Menu::HandleFocus();
	}
	
	//
	
	void Menu_Main::Handle_RenderPlay(void* obj, void* arg)
	{
		Menu_Main* self = (Menu_Main*)obj;
		
		if (self->m_HasSave)
		{
			if (self->m_ContinueAnim.Progress_get() < 0.5f)
				g_GameState->Render(g_GameState->GetShape(Resources::MENU_MAIN_CONTINUE), Vec2F(VIEW_SX / 2.0f, VIEW_SY / 2.0f), 0.0f, SpriteColors::White);
		}
	}
	
	//
		
	void Menu_Main::Handle_New(void* obj, void* arg)
	{
		// new game

		g_GameState->m_GameSettings->ResetCustomSettings();

		g_GameState->ActiveView_set(View_GameSelect);
	}
	
	void Menu_Main::Handle_NewGame(void* obj, void* arg)
	{
		// new game or continue

		Menu_Main* self = (Menu_Main*)obj;

		g_GameState->m_GameSettings->ResetCustomSettings();
		
		if (self->m_HasSave)
			g_GameState->GameBegin(Game::GameMode_ClassicPlay, Difficulty_Unknown, true);
		else
			g_GameState->ActiveView_set(View_GameSelect);
	}

	void Menu_Main::Handle_Store(void* obj, void* arg)
	{
		g_GameState->ActiveView_set(View_Upgrade);
	}

	void Menu_Main::Handle_GameCenter(void* obj, void* arg)
	{
		#if defined(IPHONEOS)
		g_gameCenter->ShowGameCenter();
		#endif
	}
	
	void Menu_Main::Handle_Credits(void* obj, void* arg)
	{
		Game::View_Credits* view = (Game::View_Credits*)g_GameState->GetView(::View_Credits);

		view->Show(::View_Main);
		
		g_gameCenter->ShowGameCenter();
	}

	void Menu_Main::Handle_Options(void* obj, void* arg)
	{
#if defined(IPHONEOS) || defined(PSP_UI) || defined(BBOS)
		Game::View_Options* view = (Game::View_Options*)g_GameState->GetView(::View_Options);
		
		view->Show(View_Main);
#else
		g_GameState->ActiveView_set(::View_WinSetup);
#endif
	}
	
	void Menu_Main::Handle_Scores(void* obj, void* arg)
	{
#if defined(PSP_UI)
		g_GameState->ActiveView_set(View_ScoresPSP);
#else
		g_GameState->ActiveView_set(View_Scores);
#endif
	}
}
