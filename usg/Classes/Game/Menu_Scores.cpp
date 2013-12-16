#include "GameState.h"
#include "GuiButton.h"
#include "Menu_Scores.h"
#include "StringBuilder.h"
#include "System.h"
#include "TempRender.h"
#include "UsgResources.h"
#include "View_Scores.h"

#ifdef IPHONEOS
#include "GameCenter.h"
#endif
#ifdef BBOS
#include "GameView_BBOS.h"
#endif

#define BUTTON_HISTORY_7 "7 DAYS"
#define BUTTON_HISTORY_30 "30 DAYS"
#define BUTTON_HISTORY_ALL "ALL TIME"

#if defined(BBOS)
	#define HAS_NETWORK 1
	#define SHOW_LOCAL_VS_GLOBAL 0
	#define SHOW_NETWORK_ERROR 0
#elif defined(IPAD)
	#define HAS_NETWORK 1
	#define SHOW_LOCAL_VS_GLOBAL 0
	#define SHOW_NETWORK_ERROR 0
#else
	#define HAS_NETWORK 1
	#define SHOW_LOCAL_VS_GLOBAL 1
	#define SHOW_NETWORK_ERROR 1
#endif

namespace GameMenu
{
	static void Render_History(void* obj, void* arg);
	
	Menu_Scores::Menu_Scores() : Menu()
	{
	}
	
	Menu_Scores::~Menu_Scores()
	{
	}
	
	void Menu_Scores::Init()
	{
		SetTransition(TransitionEffect_Slide, Vec2F(0.0f, 100.0f), 0.5f);
		
		Button btn_SelectDifficulty_Easy;
		Button btn_SelectDifficulty_Hard;
		Button btn_SelectDifficulty_Custom;

		#if HAS_NETWORK
		Button btn_FindMe;
		Button btn_Refresh;
		#endif

		#if SHOW_LOCAL_VS_GLOBAL
		Button btn_SelectArea_Global;
		Button btn_SelectArea_Local;
		#endif

		Button btn_GoBack;
		
		btn_SelectDifficulty_Easy.Setup_Shape(0, Vec2F(6.0f, 22.0f), g_GameState->GetShape(Resources::SCOREVIEW_BUTTON_EASY), 0, CallBack(this, Handle_SelectDifficulty_Easy));
		btn_SelectDifficulty_Easy.SetTransition(TransitionEffect_Slide, Vec2F(-100.0f, 0.0f));
		btn_SelectDifficulty_Hard.Setup_Shape(0, Vec2F(6.0f, 96.0f), g_GameState->GetShape(Resources::SCOREVIEW_BUTTON_HARD), 0, CallBack(this, Handle_SelectDifficulty_Hard));
		btn_SelectDifficulty_Hard.SetTransition(TransitionEffect_Slide, Vec2F(-100.0f, 0.0f));
		btn_SelectDifficulty_Custom.Setup_Shape(0, Vec2F(6.0f, 170.0f), g_GameState->GetShape(Resources::SCOREVIEW_BUTTON_CUSTOM), 0, CallBack(this, Handle_SelectDifficulty_Custom));
		btn_SelectDifficulty_Custom.SetTransition(TransitionEffect_Slide, Vec2F(-100.0f, 0.0f));

		#if HAS_NETWORK
		btn_FindMe.Setup_Shape(0, Vec2F(VIEW_SX - 100.0f, 22.0f), g_GameState->GetShape(Resources::SCOREVIEW_BUTTON_FINDME), 0, CallBack(this, Handle_FindMe));
		btn_FindMe.SetTransition(TransitionEffect_Slide, Vec2F(100.0f, 0.0f));
		btn_Refresh.Setup_Shape(0, Vec2F(VIEW_SX - 100.0f, 206.0f), g_GameState->GetShape(Resources::SCOREVIEW_BUTTON_REFRESH), 0, CallBack(this, Handle_Refresh));
		btn_Refresh.SetTransition(TransitionEffect_Slide, Vec2F(0.0f, 140.0f));
		#endif

		#if SHOW_LOCAL_VS_GLOBAL
		btn_SelectArea_Global.Setup_Shape("global", Vec2F(VIEW_SX - 100.0f, 60.0f), g_GameState->GetShape(Resources::SCOREVIEW_BUTTON_GLOBAL), 0, CallBack(this, Handle_SelectArea_Global));
		btn_SelectArea_Global.SetTransition(TransitionEffect_Slide, Vec2F(100.0f, 0.0f));
		btn_SelectArea_Local.Setup_Shape(0, Vec2F(VIEW_SX - 100.0f, 165.0f), g_GameState->GetShape(Resources::SCOREVIEW_BUTTON_LOCAL), 0, CallBack(this, Handle_SelectArea_Local));
		btn_SelectArea_Local.SetTransition(TransitionEffect_Slide, Vec2F(100.0f, 0.0f));
		#endif

		btn_GoBack.Setup_Shape(0, Vec2F(20.0f, VIEW_SY - 50.0f), g_GameState->GetShape(Resources::BUTTON_BACK), 0, CallBack(this, Handle_GoBack));
		
		AddButton(btn_SelectDifficulty_Easy);
		AddButton(btn_SelectDifficulty_Hard);
		AddButton(btn_SelectDifficulty_Custom);
		
#if 1
		// note: iOS only supports 'today', 'this week'  and 'all time'
		
		float historyX[3] = { VIEW_SX - 270.0f, VIEW_SX - 185.0f, VIEW_SX - 100.0f };
		
		Button btn_HistoryAll;
		btn_HistoryAll.Setup_Shape(BUTTON_HISTORY_ALL, Vec2F(historyX[1], VIEW_SY - 40.0f), g_GameState->GetShape(Resources::SCOREVIEW_TIME0), 0, CallBack(this, Handle_History));
		btn_HistoryAll.SetTransition(TransitionEffect_Slide, Vec2F(0.0f, 140.0f));
		btn_HistoryAll.OnRender = CallBack(this, Render_History);
		AddButton(btn_HistoryAll);
		
		Button btn_History7;
		btn_History7.Setup_Shape(BUTTON_HISTORY_7, Vec2F(historyX[2], VIEW_SY - 40.0f), g_GameState->GetShape(Resources::SCOREVIEW_TIME0), 7, CallBack(this, Handle_History));
		btn_History7.SetTransition(TransitionEffect_Slide, Vec2F(0.0f, 140.0f));
		btn_History7.OnRender = CallBack(this, Render_History);
		AddButton(btn_History7);

		/*Button btn_History30;
		btn_History30.Setup_Shape(BUTTON_HISTORY_30, Vec2F(historyX[2], VIEW_SY - 40.0f), g_GameState->GetShape(Resources::SCOREVIEW_TIME0), 30, CallBack(this, Handle_History));
		btn_History30.SetTransition(TransitionEffect_Slide, Vec2F(0.0f, 140.0f));
		btn_History30.OnRender = CallBack(this, Render_History);
		AddButton(btn_History30);*/
#endif
		
		#if HAS_NETWORK
		AddButton(btn_FindMe);
		AddButton(btn_Refresh);
		#endif

		#if SHOW_LOCAL_VS_GLOBAL
		AddButton(btn_SelectArea_Global);
		AddButton(btn_SelectArea_Local);
		#endif

		#if SHOW_NETWORK_ERROR
		#ifdef IPHONEOS
		AddButton(Button::Make_Text("connect", Vec2F(200.f, VIEW_SY - 20.0f), Vec2F((float)0.0f, 0.0f), "Game Center disabled", Resources::FONT_CALIBRI_LARGE, CallBack()));
		#else
		AddButton(Button::Make_Text("connect", Vec2F(0.0f, VIEW_SY - 20.0f), Vec2F((float)VIEW_SX, 20.0f), "network down", Resources::FONT_CALIBRI_LARGE, CallBack()));
		#endif
		#endif

		AddButton(btn_GoBack);

		#if SHOW_NETWORK_ERROR
		FindButton("connect")->m_IsEnabled = false;
		FindButton("connect")->SetTransition(TransitionEffect_None, Vec2F());
		#endif
		
		m_Difficulty = Difficulty_Easy;
	}
	
	void Menu_Scores::HandleFocus()
	{
		Menu::HandleFocus();
		
		#ifdef IPHONEOS
		if (!g_gameCenter->IsLoggedIn())
			g_gameCenter->Login();
		
		Handle_GameCenterLogin();
		#endif

		#if SHOW_LOCAL_VS_GLOBAL || SHOW_NETWORK_ERROR
		#ifdef IPHONEOS
		bool enabled = g_gameCenter->IsLoggedIn();
		#else
		bool enabled = g_System.HasNetworkConnectivity_get() == true;
		#endif
		#endif

		#if SHOW_LOCAL_VS_GLOBAL
		float opacity = enabled ? 1.0f : 0.5f;
		FindButton("global")->m_IsEnabled = enabled;
		FindButton("global")->m_CustomOpacity = opacity;
		#endif

		#if SHOW_NETWORK_ERROR
		FindButton("connect")->m_IsVisible = !enabled;
		#endif
	}

	void Menu_Scores::HandleBack()
	{
		Handle_GoBack(this, 0);
	}
	
	bool Menu_Scores::Render()
	{
		if (!Menu::Render())
			return false;
		
#if 0
		StringBuilder<32> sb;
		sb.AppendFormat("showing past %d days", g_GameState->m_GameSettings->m_ScoreHistory);
		RenderText(Vec2F(375.0f, 275.0f), Vec2F(), g_GameState->GetFont(Resources::FONT_CALIBRI_SMALL), SpriteColors::White, TextAlignment_Right, TextAlignment_Bottom, true, sb.ToString());
#endif
		
		return true;
	}
	
	#ifdef IPHONEOS
	void Menu_Scores::Handle_GameCenterLogin()
	{
		bool enabled = g_gameCenter->IsLoggedIn();
		#if SHOW_LOCAL_VS_GLOBAL
		float opacity = enabled ? 1.0f : 0.5f;
		FindButton("global")->m_IsEnabled = enabled;
		FindButton("global")->m_CustomOpacity = opacity;
		#endif

		#if SHOW_NETWORK_ERROR
		FindButton("connect")->m_IsVisible = enabled == false;
		#endif
	}
	#endif
	
	void Menu_Scores::UpdateScoreList()
	{
		Game::View_Scores* view = (Game::View_Scores*)g_GameState->m_Views[View_Scores];
		
		view->Show(view->Database_get(), m_Difficulty, g_GameState->m_GameSettings->m_ScoreHistory);
	}
	
	void Menu_Scores::Handle_SelectMode_Arcade(void* obj, void* arg)
	{
		Menu_Scores* self = (Menu_Scores*)obj;
		
//		self->m_Mode = GameMode_Arcade;
		self->UpdateScoreList();
	}
	
	void Menu_Scores::Handle_SelectMode_TimeAttack(void* obj, void* arg)
	{
		Menu_Scores* self = (Menu_Scores*)obj;
		
//		self->m_Mode = GameMode_TimeAttack;
		self->UpdateScoreList();
	}
	
	void Menu_Scores::Handle_SelectDifficulty_Easy(void* obj, void* arg)
	{
		Menu_Scores* self = (Menu_Scores*)obj;
		
		self->m_Difficulty = Difficulty_Easy;
		self->UpdateScoreList();
	}
	
	void Menu_Scores::Handle_SelectDifficulty_Hard(void* obj, void* arg)
	{
		Menu_Scores* self = (Menu_Scores*)obj;
		
		self->m_Difficulty = Difficulty_Hard;
		self->UpdateScoreList();
	}

	void Menu_Scores::Handle_SelectDifficulty_Custom(void* obj, void* arg)
	{
		Menu_Scores* self = (Menu_Scores*)obj;
		
		self->m_Difficulty = Difficulty_Custom;
		self->UpdateScoreList();
	}
	
#if 1
	void Menu_Scores::Handle_History(void* obj, void* arg)
	{
		Menu_Scores* self = (Menu_Scores*)obj;
		Button* button = (Button*)arg;
		//Game::View_Scores* view = (Game::View_Scores*)g_GameState->m_Views[View_Scores];
		
		int history = button->m_Info;
		
		g_GameState->m_GameSettings->m_ScoreHistory = history;
		//g_GameState->m_GameSettings->Save();
		
		self->UpdateScoreList();
	}
#endif
	
	void Menu_Scores::Handle_FindMe(void* obj, void* arg)
	{
		Game::View_Scores* view = (Game::View_Scores*)g_GameState->m_Views[View_Scores];
		
#if defined(BBOS)
		view->ScrollUserIntoView();
#else
		int rank = view->BestRank_get();
		
		if (rank < 0)
		{
			// todo: play menu deny sound
		}
		else
			view->ScrollIntoView(rank);
#endif
	}
	
	void Menu_Scores::Handle_SelectArea_Global(void* obj, void* arg)
	{
		Game::View_Scores* view = (Game::View_Scores*)g_GameState->m_Views[View_Scores];
		
		view->Database_set(Game::ScoreDatabase_Global);
		view->Refresh();
	}
	
	void Menu_Scores::Handle_SelectArea_Local(void* obj, void* arg)
	{
		Game::View_Scores* view = (Game::View_Scores*)g_GameState->m_Views[View_Scores];
		
		view->Database_set(Game::ScoreDatabase_Local);
		view->Refresh();
	}
	
	void Menu_Scores::Handle_Refresh(void* obj, void* arg)
	{
//		Menu_Scores* self = (Menu_Scores*)obj;
		
		Game::View_Scores* view = (Game::View_Scores*)g_GameState->m_Views[View_Scores];
		
		view->Refresh();
	}
	
	void Menu_Scores::Handle_GoBack(void* obj, void* arg)
	{
//		Menu_Scores* self = (Menu_Scores*)obj;
		
		g_GameState->ActiveView_set(View_Main);
	}
	
#if 1
	static void Render_History(void* obj, void* arg)
	{
		Button* button = (Button*)arg;
		
		Vec2F p = button->Position_get();
		Vec2F s = button->Size_get();
		
		const VectorShape* shape0 = g_GameState->GetShape(Resources::SCOREVIEW_TIME0);
		const VectorShape* shape1 = g_GameState->GetShape(Resources::SCOREVIEW_TIME1);
		Vec2I delta = shape1->m_Shape.m_BoundingBox.ToRect().m_Size - shape0->m_Shape.m_BoundingBox.ToRect().m_Size;
		
		if (g_GameState->m_GameSettings->m_ScoreHistory == button->m_Info)
		{
			g_GameState->Render(shape1, p - Vec2F(delta[0] / 2.0f, delta[1] / 2.0f), 0.0f, SpriteColors::White);
		}
		
		g_GameState->Render(shape0, p, 0.0f, SpriteColors::White);
		
		RenderText(p, s, g_GameState->GetFont(Resources::FONT_LGS), SpriteColors::White, TextAlignment_Center, TextAlignment_Center, true, button->m_Name);
	}
#endif
}
