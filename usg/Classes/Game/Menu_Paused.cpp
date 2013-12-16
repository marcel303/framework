#include "EntityPlayer.h"
#include "GameState.h"
#include "GuiButton.h"
#include "Menu_Paused.h"
#include "PlayerController.h"
#include "TempRender.h"
#include "Textures.h"
#include "UsgResources.h"
#include "View_Options.h"
#include "World.h"

namespace GameMenu
{
	Menu_Pause::Menu_Pause() : Menu()
	{
	}
	
	Menu_Pause::~Menu_Pause()
	{
	}
	
	void Menu_Pause::Init()
	{
		SetTransition(TransitionEffect_Fade, Vec2F(0.0f, 100.0f), 0.5f);
		
		Button btn_Options;
		Button btn_Stop;
		Button btn_Resume;
		Button btn_Calibrate;
		
		btn_Options.Setup_Shape(0, Vec2F(145.0f, 124.0f), g_GameState->GetShape(Resources::PAUSEVIEW_BUTTON_OPTIONS), 0, CallBack(this, Handle_Options));
		btn_Stop.Setup_Shape(0, Vec2F(145.0f, 165.0f), g_GameState->GetShape(Resources::PAUSEVIEW_BUTTON_QUIT), 0, CallBack(this, Handle_Stop));
		btn_Resume.Setup_Shape(0, Vec2F(240.0f, 125.0f), g_GameState->GetShape(Resources::PAUSEVIEW_BUTTON_RESUME), 0, CallBack(this, Handle_Resume));
		//btn_Calibrate = Button::Make_Custom("calibrate", Vec2F(240.0f - 60.0f, 220.0f), Vec2F(120.0f, 40.0f), 0, CallBack(this, Handle_Calibrate), CallBack(this, Render_Calibrate));
		btn_Calibrate = Button::Make_Shape("calibrate", Vec2F(158.0f, 220.0f), g_GameState->GetShape(Resources::PAUSEVIEW_BUTTON_CALIBRATE), 0, CallBack(this, Handle_Calibrate));
		
		AddButton(btn_Calibrate);
		AddButton(btn_Options);
		AddButton(btn_Stop);
		AddButton(btn_Resume);

		Translate((VIEW_SX-480)/2, (VIEW_SY-320)/2);
	}
	
	void Menu_Pause::HandleFocus()
	{
		bool tiltVisible = g_GameState->m_GameSettings->m_ControllerType == ControllerType_Tilt;

		FindButton("calibrate")->m_IsVisible = tiltVisible;

		Menu::HandleFocus();
	}

	void Menu_Pause::HandleBack()
	{
		Handle_Resume(this, 0);
	}

	bool Menu_Pause::HandlePause()
	{
		Handle_Resume(this, 0);

		return true;
	}
	
	void Menu_Pause::Render_Calibrate(void* obj, void* arg)
	{
		Button* button = (Button*)arg;
		
		Vec2F mid = button->Position_get() + button->Size_get() * 0.5f;
		
		RenderRect(button->Position_get(), button->Size_get(), 0.0f, 0.0f, 0.0f, 0.8f, g_GameState->GetTexture(Textures::MENU_COLOR_WHITE));
		
		float s = 20.0f;
		Game::g_World->m_PlayerController->Update(Game::g_World->m_Player->Position_get(), Vec2F(), 0.0f);
		Vec2F delta = Game::g_World->m_PlayerController->MovementDirection_get() * Game::g_World->m_PlayerController->MovementSpeed_get();
		
		Vec2F sizeX(delta[0], s);
		Vec2F sizeY(s, delta[1]);
		
		RenderRect(Vec2F(mid[0], mid[1]-s/2.0f), Vec2F(delta[0], s), 0.25f, 0.5f, 1.0f, 0.5f, g_GameState->GetTexture(Textures::BANDITVIEW_CURVE));
		RenderRect(Vec2F(mid[0]-s/2.0f, mid[1]), Vec2F(s, delta[1]), 0.25f, 0.5f, 1.0f, 0.5f, g_GameState->GetTexture(Textures::BANDITVIEW_CURVE));
		RenderText(mid, Vec2F(), g_GameState->GetFont(Resources::FONT_CALIBRI_SMALL), SpriteColors::White, TextAlignment_Center, TextAlignment_Center, false, "CALIBRATE");
	}
	
	void Menu_Pause::Handle_Calibrate(void* obj, void* arg)
	{
		Game::g_World->m_PlayerController->TiltCalibrate();
		
		g_GameState->m_GameSettings->Save();

		g_GameState->ActiveView_set(View_InGame);
	}
	
	void Menu_Pause::Handle_Resume(void* obj, void* arg)
	{
		g_GameState->ActiveView_set(View_InGame);
	}
	
	void Menu_Pause::Handle_Options(void* obj, void* arg)
	{
		Game::View_Options* view = (Game::View_Options*)g_GameState->GetView(::View_Options);
		
		view->Show(::View_Pause);
	}
	
	void Menu_Pause::Handle_Stop(void* obj, void* arg)
	{
		Game::g_World->HandleGameEnd();
	}
}
