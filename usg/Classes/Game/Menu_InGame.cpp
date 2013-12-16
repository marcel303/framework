#include "Atlas_ImageInfo.h"
#include "EntityPlayer.h"
#include "GameHelp.h"
#include "GameState.h"
#if defined(BBOS)
#include "GameView_BBOS.h"
#endif
#include "GuiButton.h"
#include "Menu_InGame.h"
#include "MenuMgr.h"
#include "MenuRender.h"
#if defined(IPHONEOS) || defined(WIN32) || defined(LINUX) || defined(MACOS)
	#include "Screenshot.h"
#endif
#include "System.h"
#include "TempRender.h"
#include "Textures.h"
#include "UsgResources.h"
#include "World.h"

#define UPGRADE_SIZE (g_GameState->GetTexture(Textures::GAMEVIEW_UPGRADE)->m_Info->ImageSizeF_get() / 2.f)

namespace GameMenu
{
#if defined(PSP_UI)
	static DesignInfo design[] =
	{
		DesignInfo("upgrade", true, Vec2F(VIEW_SX - 105.0f, 5.0f), Vec2F())
	};
#else
	static DesignInfo design[] =
	{
		DesignInfo("upgrade", true, Vec2F(VIEW_SX - 105.0f, 5.0f), Vec2F())
#ifdef IPAD
		// iPad: (380, 210), (430, 195)
		, DesignInfo("weapon", true, Vec2F(VIEW_SX - 100.0f - 23.0f, VIEW_SY - 150.0f - 23.0f), Vec2F())
		, DesignInfo("special", true, Vec2F(VIEW_SX - 50.0f - 23.0f, VIEW_SY - 165.0f - 23.0f), Vec2F())
#endif
	};
#endif

	Menu_InGame::Menu_InGame() : Menu()
	{
		mParticleAngle = 0.0f;
	}
	
	Menu_InGame::~Menu_InGame()
	{
	}
	
	void Menu_InGame::Init()
	{
		SetTransition(TransitionEffect_Fade, Vec2F(0.0f, 100.0f), 0.5f);
		
#ifndef PSP_UI
		AddButton(Button::Make_Custom("pause_area", Vec2F(0.0f, 0.0f), Vec2F(115.0f, 35.0f), 0, CallBack(this, Handle_Pause), CallBack(this, Render_Empty)));
		AddButton(Button::Make_Shape("pause", Vec2F(85.0f, 5.0f), g_GameState->GetShape(Resources::BUTTON_PAUSE), 0, CallBack(this, Handle_Pause)));
		FindButton("pause")->m_IsEnabled = false;
#endif
		AddButton(Button::Make_Custom("upgrade", Vec2F(VIEW_SX - 105.0f, 25.0f), UPGRADE_SIZE, 0, CallBack(this, Handle_Upgrade), CallBack(this, Render_Upgrade)));
//		Button btn_WeaponSwitch = Button::Make_Shape("weapon", Vec2F(VIEW_SX - 60.0f, 50.0f), g_GameState->GetShape(Resources::GAMEVIEW_WEAPONSWITCH), 0, CallBack(this, Handle_WeaponSwitch));
//		Button btn_WeaponSwitch = Button::Make_Custom("weapon", Vec2F(VIEW_SX - 60.0f, 50.0f), Vec2F(50.0f, 100.0f), 0, CallBack(this, Handle_WeaponSwitch), CallBack(this, Render_WeaponSwitch));
		Button btn_WeaponSwitch = Button::Make_Custom("weapon", Vec2F(VIEW_SX - 60.0f, 75.0f), Vec2F(50.0f, 50.0f), 0, CallBack(this, Handle_WeaponSwitch), CallBack(this, Render_WeaponSwitch));
		AddButton(btn_WeaponSwitch);
		Button btn_Special = Button::Make_Custom("special", Vec2F(VIEW_SX - 60.0f, 145.0f), Vec2F(50.0f, 50.0f), 0, CallBack(this, Handle_Special), CallBack(this, Render_Special));
		AddButton(btn_Special);
		
#ifndef DEPLOYMENT
//		AddButton(Button::Make_Shape(0, Vec2F(VIEW_SX - 20.0f, VIEW_SY - 20.0f), g_GameState->GetShape(Resources::POWERUP_POWER_BEAM), 0, CallBack(this, Handle_Screenshot)));
		AddButton(Button::Make_Custom(0, Vec2F(VIEW_SX - 20.0f, VIEW_SY - 20.0f), Vec2F(40.0f, 40.0f), 0, CallBack(this, Handle_Screenshot), CallBack(this, Render_Empty)));
//		AddButton(Button::Make_Shape(0, Vec2F(145.0f, VIEW_SY - 30.0f), g_GameState->GetShape(Resources::BUTTON_PAUSE), 0, CallBack(this, Handle_CheatMenu)));
		AddButton(Button::Make_Custom(0, Vec2F(145.0f, VIEW_SY - 30.0f), Vec2F(40.0f, 40.0f), 0, CallBack(this, Handle_CheatMenu), CallBack(this, Render_Empty)));
#endif

#ifdef PSP_UI
		FindButton("upgrade")->m_IsEnabled = false;
		FindButton("special")->m_IsEnabled = false;
		FindButton("weapon")->m_IsEnabled = false;
#endif

		DESIGN_APPLY(this, design);
	}
	
	void Menu_InGame::Update(float dt)
	{
		Menu::Update(dt);

#if !defined(PSP_UI)
		Button* pauseButton = FindButton("pause");
		
		if (pauseButton)
		{
			if (Game::g_World->HudOpacity_get() == 1.0f)
				pauseButton->m_IsVisible = true;
			else
				pauseButton->m_IsVisible = false;
		}
#endif
		
#if defined(BBOS)
		bool enabled = !gGameView->m_GamepadIsEnabled;
		FindButton("pause")->m_IsVisible = enabled;
		FindButton("pause_area")->m_IsVisible = enabled;
		FindButton("upgrade")->m_IsEnabled = enabled;
		FindButton("weapon")->m_IsEnabled = enabled;
		FindButton("special")->m_IsEnabled = enabled;
#endif

#if !defined(PSP_UI)
		if (!g_GameState->m_HelpState->IsComplete(Game::HelpState::State_HitUpgrade))
			FindButton("upgrade")->m_IsVisible = false;
		if (g_GameState->m_HelpState->IsActive(Game::HelpState::State_HitUpgrade))
			FindButton("upgrade")->m_IsVisible = true;
		
		if (!g_GameState->m_HelpState->IsComplete(Game::HelpState::State_HitWeaponSwitch))
			FindButton("weapon")->m_IsVisible = false;
		if (g_GameState->m_HelpState->IsActive(Game::HelpState::State_HitWeaponSwitch))
			FindButton("weapon")->m_IsVisible = true;
		
		if (!g_GameState->m_HelpState->IsComplete(Game::HelpState::State_HitSpecial))
			FindButton("special")->m_IsVisible = false;
		if (g_GameState->m_HelpState->IsActive(Game::HelpState::State_HitSpecial))
			FindButton("special")->m_IsVisible = true;
#endif
	}
	
	bool Menu_InGame::Render()
	{
		if (!Menu::Render())
			return false;
		
		if (g_GameState->m_HelpState->IsActive(Game::HelpState::State_HitUpgrade))
		{
			Vec2F pos = FindButton("upgrade")->Position_get() + FindButton("weapon")->Size_get() * 0.5f;
			RenderBouncyText(pos + Vec2F(0.0f, 20.0f), Vec2F(0.0f, -10.0f), "UPGRADE");
		}
		if (g_GameState->m_HelpState->IsActive(Game::HelpState::State_HitWeaponSwitch))
		{
			Vec2F pos = FindButton("weapon")->Position_get() + FindButton("weapon")->Size_get() * 0.5f;
			RenderBouncyText(pos - Vec2F(100.0f, 0.0f), Vec2F(10.0f, 0.0f), "WEAPON");
		}
		if (g_GameState->m_HelpState->IsActive(Game::HelpState::State_HitSpecial))
		{
			Vec2F pos = FindButton("special")->Position_get() + FindButton("special")->Size_get() * 0.5f;
			RenderBouncyText(pos - Vec2F(100.0f, 0.0f), Vec2F(10.0f, 0.0f), "SPECIAL");
		}
		
		return true;
	}
	
	void Menu_InGame::HandleFocus()
	{
		Menu::HandleFocus();

		FindButton("upgrade")->m_IsVisible = true;
		FindButton("weapon")->m_IsVisible = true;
		FindButton("special")->m_IsVisible = true;
	}
	
	bool Menu_InGame::HandlePause()
	{
		Handle_Pause(this, 0);

		return true;
	}

	void Menu_InGame::HudMode_set(Game::HudMode mode)
	{
		// todo: show/hide appropriate buttons
	}
	
	void Menu_InGame::HudOpacity_set(float value)
	{
		// adjust button opacity
		
		FindButton("upgrade")->m_CustomOpacity = value;
		FindButton("weapon")->m_CustomOpacity = value;
		FindButton("special")->m_CustomOpacity = value;
	}
	
	void Menu_InGame::Handle_Pause(void* obj, void* arg)
	{
//		Menu_InGame* self = (Menu_InGame*)obj;
		
		g_GameState->ActiveView_set(View_Pause);
	}
	
	void Menu_InGame::Handle_CheatMenu(void* obj, void* arg)
	{
		g_GameState->Interface_get()->ActiveMenu_set(MenuType_Cheats);
	}
	
	void Menu_InGame::Handle_WeaponSwitch(void* obj, void* arg)
	{
		Game::g_World->m_Player->SwitchWeapons();
	}
	
	static int WeaponType_To_Shape(Game::WeaponType type)
	{
		switch (type)
		{
			case Game::WeaponType_Laser:
				//return Resources::POWERUP_POWER_BEAM;
				return Resources::GAMEVIEW_WEAPONSWITCH_BEAM;
			case Game::WeaponType_Vulcan:
				//return Resources::POWERUP_POWER_VULCAN;
				return Resources::GAMEVIEW_WEAPONSWITCH_VULCAN;
			default:
#ifndef DEPLOYMENT
				throw ExceptionVA("not implemented");
#else
				return Resources::PLAYER_SHIP;
#endif
		}
	}
	
	void Menu_InGame::Render_WeaponSwitch(void* obj, void* arg)
	{
		Button* button = (Button*)arg;
		
		// draw current weapon
		
		Vec2F mid = button->Position_get() + button->Size_get() / 2.0f;
		mid[0] = Calc::RoundDown(mid[0]);
		mid[1] = Calc::RoundDown(mid[1]);
		
		if (g_GameState->m_GameSettings->m_HudOpacity >= 0.8f && false)
		{
//			Vec2F offset_Icon = button->Size_get() / 2.0f;
//			g_GameState->Render(g_GameState->GetShape(Resources::GAMEVIEW_WEAPONSWITCH), mid, 0.0f, SpriteColors::White);
			g_GameState->Render(g_GameState->GetShape(WeaponType_To_Shape(Game::g_World->m_Player->CurrentWeapon_get())), mid, 0.0f, SpriteColors::White);
		}
		else
		{
			g_GameState->Render(g_GameState->GetShape(WeaponType_To_Shape(Game::g_World->m_Player->CurrentWeapon_get())), mid /*+ offset_Icon*/, 0.0f, SpriteColor_MakeF(1.f, 1.f, 1.f, button->m_CustomOpacity));
		}
	}
	
	void Menu_InGame::Handle_Special(void* obj, void* arg)
	{
		//Menu_InGame* self = (Menu_InGame*)obj;
		//Button* button = (Button*)arg;
		
		Game::g_World->m_Player->SpecialAttack_Begin();
	}
	
	void Menu_InGame::Render_Special(void* obj, void* arg)
	{
		//Menu_InGame* self = (Menu_InGame*)obj;
		Button* button = (Button*)arg;
		
		const AtlasImageMap* image = g_GameState->GetTexture(Textures::POWERUP_SPECIAL_BUTTON);
		
		Vec2F mid = button->Position_get() + button->Size_get() / 2.0f;
		mid[0] = Calc::RoundDown(mid[0]);
		mid[1] = Calc::RoundDown(mid[1]);
		
		Vec2F size((float)image->m_Info->m_ImageSize[0], (float)image->m_Info->m_ImageSize[1]);
		
		const float RADIUS = 14.0f;
		
		Vec2F min = mid - Vec2F(RADIUS, RADIUS);
		Vec2F max = mid + Vec2F(RADIUS, RADIUS);
		
		g_GameState->Render(g_GameState->GetShape(Resources::POWERUP_SPECIAL_BUTTON_BACK), mid, 0.0f, SpriteColor_MakeF(1.f, 1.f, 1.f, button->m_CustomOpacity));
		
		DrawPieQuadThingy(min, max, 0.0f, Calc::m2PI * Game::g_World->m_Player->SpecialAttackFill_get(), image, SpriteColor_MakeF(1.f, 1.f, 1.f, button->m_CustomOpacity));
	}
	
	void Menu_InGame::Handle_Upgrade(void* obj, void* arg)
	{
		//View_Upgrade* view = (View_Upgrade*)g_GameState->GetView(::View_Upgrade);
		
		g_GameState->ActiveView_set(View_Upgrade);
		
		g_GameState->m_HelpState->DoComplete(Game::HelpState::State_HitUpgrade);
	}
	
	void Menu_InGame::Handle_Screenshot(void* obj, void* arg)
	{
#if defined(IPHONEOS) || defined(WIN32) || defined(LINUX) || defined(MACOS)
		Screenshot* ss1 = ScreenshotUtil::CaptureGL(VIEW_SY, VIEW_SX);
		Screenshot* ss2 = 0;
		
		if (!g_GameState->m_GameSettings->m_ScreenFlip)
			ss2 = ss1->RotateCCW90();
		else
			ss2 = ss1->RotateCW90();
		
		delete ss1;
		ss1 = 0;
		
		g_System.SaveToAlbum(ss2, "test");
		
		delete ss2;
		ss2 = 0;
#elif defined(PSP) || defined(BBOS)
#else
#error unknown system
#endif
	}
	
	void Menu_InGame::Render_Upgrade(void* obj, void* arg)
	{
		Menu_InGame* self = (Menu_InGame*)obj;
		Button* button = (Button*)arg;
		
		Render_Upgrade(button->Position_get(), button->m_CustomOpacity);
		
		if (Game::g_World->m_Player->Credits_get() == 1000)
		{
			Vec2F pos = button->Position_get() + UPGRADE_SIZE * 0.5f + (Vec2F::FromAngle(self->mParticleAngle) ^ Vec2F(90.0f, 30.0f)) * 0.5f;
			
			Particle& p = g_GameState->m_ParticleEffect_UI.Allocate(0, g_GameState->GetShape(Resources::INDICATOR_UPGRADE), Particle_Default_Update);
			Particle_Default_Setup(&p, pos[0], pos[1], 0.2f, 0.0f, 0.0f, 0.0f, 0.0f);
			p.m_Color = SpriteColor_MakeF(1.f, 1.f, 1.f, button->m_CustomOpacity);
			
			self->mParticleAngle += 0.2f;
		}
	}
	
	void Menu_InGame::Render_Upgrade(const Vec2F& pos, float opacity)
	{
		const AtlasImageMap* image = g_GameState->GetTexture(Textures::GAMEVIEW_UPGRADE);
		
		Vec2F size = UPGRADE_SIZE;

		float progress = Game::g_World->m_Player->Credits_get() / 1000.0f;
		
		DrawBarH(*g_GameState->m_SpriteGfx, pos, pos + UPGRADE_SIZE, g_GameState->GetTexture(Textures::GAMEVIEW_UPGRADE_BACK2), 1.0f, SpriteColor_MakeF(1.f, 1.f, 1.f, opacity).rgba);
		DrawBarH(*g_GameState->m_SpriteGfx, pos, pos + UPGRADE_SIZE, g_GameState->GetTexture(Textures::GAMEVIEW_UPGRADE_BACK), progress, SpriteColor_MakeF(1.f, 1.f, 1.f, opacity).rgba);
		RenderRect(pos, size, 1.0f, 1.0f, 1.0f, opacity, image);
	}
}
