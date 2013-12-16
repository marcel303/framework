#include "GameState.h"
#include "GuiButton.h"
#include "GuiCheckbox.h"
#include "Menu_ScoreEntry.h"
#include "MenuRender.h"
#include "System.h"
#include "UsgResources.h"
#include "View_KeyBoard.h"
#include "View_ScoreSubmit.h"

#ifdef IPHONEOS
#include "GameCenter.h"
#endif
#if defined(BBOS)
#include "GameView_BBOS.h"
#endif

#define BUTTON_ONLINE "submit score on-line"
#define BUTTON_LOCAL "submit score locally"

namespace GameMenu
{
	static DesignInfo design[] =
	{
		DesignInfo(BUTTON_ONLINE, true, Vec2F(55.0f, 120.0f), Vec2F(30.0f, 30.0f)),
		DesignInfo(BUTTON_LOCAL,  true, Vec2F(55.0f, 160.0f), Vec2F(30.0f, 30.0f))
	};

	void Menu_ScoreEntry::Init()
	{
		SetTransition(TransitionEffect_Slide, Vec2F(0.0f, 100.0f), 0.5f);

		AddButton(Button::Make_Shape(0, Vec2F(10.0f,  240.0f), g_GameState->GetShape(Resources::OPTIONSVIEW_ACCEPT ), 0, CallBack(this, Handle_Accept )));
		AddButton(Button::Make_Shape(0, Vec2F(110.0f, 240.0f), g_GameState->GetShape(Resources::OPTIONSVIEW_DISMISS), 0, CallBack(this, Handle_Dismiss)));
		
		AddElement(new GuiCheckbox(BUTTON_ONLINE, true, BUTTON_ONLINE, -1, CallBack(this, Handle_ToggleSubmitOnline)));
		AddElement(new GuiCheckbox(BUTTON_LOCAL,  true, BUTTON_LOCAL,  -1, CallBack(this, Handle_ToggleSubmitLocal ))); 

		DESIGN_APPLY(this, design);

		Translate((VIEW_SX-480)/2, (VIEW_SY-320)/2);
	}

	void Menu_ScoreEntry::HandleFocus()
	{
		Menu::HandleFocus();
		
#if defined(IPHONEOS) || defined(BBOS)
#if defined(IPHONEOS)
		m_useGameCenter = g_gameCenter->IsLoggedIn();
#endif
#if defined(BBOS)
		m_useGameCenter = true;
#endif
		
		if (m_useGameCenter)
		{
			FindCheckbox(BUTTON_ONLINE)->m_IsVisible = true;
			FindCheckbox(BUTTON_ONLINE)->m_IsEnabled = false;
			FindCheckbox(BUTTON_ONLINE)->IsChecked_set(true, false);
			FindCheckbox(BUTTON_LOCAL)->m_IsVisible = false;
			FindCheckbox(BUTTON_LOCAL)->m_IsEnabled = false;
			FindCheckbox(BUTTON_LOCAL)->IsChecked_set(false, false);
		}
		else
		{
			FindCheckbox(BUTTON_ONLINE)->m_IsVisible = false;
			FindCheckbox(BUTTON_ONLINE)->m_IsEnabled = false;
			FindCheckbox(BUTTON_ONLINE)->IsChecked_set(false, false);
			FindCheckbox(BUTTON_LOCAL)->m_IsVisible = true;
			FindCheckbox(BUTTON_LOCAL)->m_IsEnabled = false;
			FindCheckbox(BUTTON_LOCAL)->IsChecked_set(true, false);
		}
#else
		bool connectivity = g_System.HasNetworkConnectivity_get();
		
		FindCheckbox(BUTTON_ONLINE)->m_IsVisible = connectivity;
		FindCheckbox(BUTTON_ONLINE)->IsChecked_set(connectivity, false);
		FindCheckbox(BUTTON_LOCAL)->IsChecked_set(true, false);
#endif
	}
	
	void Menu_ScoreEntry::Handle_Accept(void* obj, void* arg)
	{
		Menu_ScoreEntry* self = (Menu_ScoreEntry*)obj;
		Game::View_ScoreSubmit* view = (Game::View_ScoreSubmit*)g_GameState->GetView(View_ScoreSubmit);
		
		if (self->FindCheckbox(BUTTON_ONLINE)->IsChecked_get())
			view->Database_set(Game::ScoreDatabase_Global);
		else
			view->Database_set(Game::ScoreDatabase_Local);
		
#ifdef IPHONEOS
		if (self->FindCheckbox(BUTTON_ONLINE)->IsChecked_get())
			g_GameState->ActiveView_set(::View_ScoreSubmit);
		else
			g_GameState->KeyboardView_get()->Show("Please enter your name", g_GameState->m_GameSettings->m_PlayerName.c_str(), MAX_NAME_LENGTH, View_ScoreSubmit);
#else
		g_GameState->KeyboardView_get()->Show("Please enter your name", g_GameState->m_GameSettings->m_PlayerName.c_str(), MAX_NAME_LENGTH, View_ScoreSubmit);
#endif
	}
	
	void Menu_ScoreEntry::Handle_Dismiss(void* obj, void* arg)
	{
		g_GameState->ActiveView_set(View_Main);
	}
	
	void Menu_ScoreEntry::Handle_ToggleSubmitOnline(void* obj, void* arg)
	{
		Menu_ScoreEntry* self = (Menu_ScoreEntry*)obj;
		Button* button = (Button*)arg;
		
		//self->m_SubmitOnline = true;
		//self->m_SubmitLocal = false;
		
		for (Col::ListNode<IGuiElement*>* node = self->m_Elements.m_Head; node; node = node->m_Next)
		{
			if (node->m_Object->Type_get() != GuiElementType_Button)
				continue;

			Button* temp = (Button*)node->m_Object;

			temp->m_Info = false;
		}
		
		button->m_Info = true;
	}
	
	void Menu_ScoreEntry::Handle_ToggleSubmitLocal(void* obj, void* arg)
	{
		Menu_ScoreEntry* self = (Menu_ScoreEntry*)obj;
		Button* button = (Button*)arg;
		
		//self->m_SubmitLocal = true;
		//self->m_SubmitOnline = false;
		
		for (Col::ListNode<IGuiElement*>* node = self->m_Elements.m_Head; node; node = node->m_Next)
		{
			if (node->m_Object->Type_get() != GuiElementType_Button)
				continue;

			Button* temp = (Button*)node->m_Object;

			temp->m_Info = false;
		}
		
		button->m_Info = true;
	}
}
