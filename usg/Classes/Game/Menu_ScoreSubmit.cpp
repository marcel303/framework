#include "GameState.h"
#include "GuiButton.h"
//#include "grs.h"
#include "Menu_ScoreSubmit.h"
#include "View_ScoreSubmit.h"

namespace GameMenu
{
	static void RenderContinue(void* obj, void* arg)
	{
	}
	
	void Menu_ScoreSubmit::Init()
	{
		SetTransition(TransitionEffect_Slide, Vec2F(0.0f, 100.0f), 0.5f);
		
		Button btn_Skip;
		btn_Skip.Setup_Text("skip", Vec2F(70.0f, VIEW_SY - 75.0f), Vec2F(100.0f, 30.0f), "(skip)", 0, CallBack(this, Handle_Continue));
		btn_Skip.m_IsVisible = false;
		btn_Skip.SetTransition(TransitionEffect_Fade, Vec2F());
		AddButton(btn_Skip);
		
		Translate((VIEW_SX-480)/2, (VIEW_SY-320)/2);

		Button btn_Continue;
		btn_Continue.Setup_Custom("continue", Vec2F(0.0f, 0.0f), Vec2F((float)VIEW_SX, (float)VIEW_SY), 0, CallBack(this, Handle_Continue), CallBack(this, RenderContinue));
		btn_Continue.SetHitEffect(HitEffect_None);
		AddButton(btn_Continue);
	}
	
	void Menu_ScoreSubmit::HandleFocus()
	{
		Menu::HandleFocus();

		FindButton("skip")->m_IsVisible = false;
		
		mSkipTrigger.Start(6.0f);
	}
	
	void Menu_ScoreSubmit::Update(float dt)
	{
		Menu::Update(dt);
		
		if (mSkipTrigger.Read())
		{
			FindButton("skip")->m_IsVisible = true;
		}
	}
	
	void Menu_ScoreSubmit::Handle_Continue(void* obj, void* arg)
	{
		Game::View_ScoreSubmit* view = (Game::View_ScoreSubmit*)g_GameState->GetView(::View_ScoreSubmit);
		
		view->NextView();
	}
	
	void Menu_ScoreSubmit::Handle_SubmitResult()
	{
		FindButton("continue")->m_IsVisible = true;
		
		mSkipTrigger.Stop();
	}
}
