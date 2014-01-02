#include "Calc.h"
#include "EntityPlayer.h"
#include "GameRound.h"
#include "GameState.h"
#include "Grid_Effect.h"
//#include "grs.h"
#include "MenuMgr.h"
#include "StringBuilder.h"
#include "Svn.h"
#include "System.h"
#include "TempRender.h"
#include "Textures.h"
#include "UsgResources.h"
#include "Util_ColorEx.h"
#include "View_Main.h"
#include "World.h"

namespace Game
{
	View_Main::View_Main() : IView()
	{
	}
	
	void View_Main::Initialize()
	{
		m_SpawnTimer.Initialize(g_GameState->m_TimeTracker_Global);
		m_SpawnTimer.SetInterval(7.0f);
	}
	
	void View_Main::RenderLogo()
	{
		g_GameState->Render(g_GameState->GetShape(Resources::MAINVIEW_LOGO), Vec2F(10.0f, 5.0f), 0.0f, SpriteColors::White);
	}
	
	void View_Main::HandleFocus()
	{
	#ifdef MACOS
		Res* bgm1 = g_GameState->m_ResMgr.Get(Resources::BGM_MAIN1);
		Res* bgm2 = g_GameState->m_ResMgr.Get(Resources::BGM_MAIN2);
		#ifndef DEBUG
		g_GameState->PlayMusic(bgm1, bgm2);
		#endif
	#else
		g_GameState->PlayMusic(g_GameState->m_ResMgr.Get(Resources::BGM_MENU));
	#endif
		
		g_GameState->Interface_get()->ActiveMenu_set(GameMenu::MenuType_Main);
		
		m_SpawnTimer.Start();
		
		g_World->IsPaused_set(false);
	}
	
	void View_Main::HandleFocusLost()
	{
		g_World->IsPaused_set(true);
	}
	
	void View_Main::Update(float dt)
	{
		Assert(g_GameState->m_GameRound->GameMode_get() == GameMode_IntroScreen);
		
		g_GameState->m_GameRound->Update(dt);
		
		g_World->m_GridEffect->BaseHue_set(g_GameState->m_TimeTracker_Global->Time_get() / 50.0f);
		g_World->m_GridEffect->Impulse(Vec2F(Calc::Random(0.0f, (float)WORLD_SX), Calc::Random(0.0f, (float)WORLD_SY)), 0.1f);
		
		g_GameState->ClearSB();
		g_World->Update(dt);
		
		// Note: This could be moved to GameRound::UpdateIntroScreen ..
		while (m_SpawnTimer.ReadTick())
		{
			EntityClass type = g_GameState->m_GameRound->IntroScreen_GetEnemyType();
			//float interval = 0.0f;
			float interval = 0.1f;
			float angle = Calc::Random(Calc::m2PI);
			float arc = Calc::Random(0.2f, 1.0f) * Calc::m2PI;
			int count = (int)Calc::DivideUp(arc, 0.5f);
			float radius1 = 80.0f;
			float radius2 = 160.0f;
			
			EnemyWave wave;
			wave.MakeCircle(type, g_World->m_Player->Position_get(), radius1, radius2, angle, arc, count, interval);
			g_GameState->m_GameRound->IntroScreen_WaveAdd(wave);
			
//			g_World->SpawnEnemy(g_GameState->m_GameRound->GetEnemyType(), WORLD_MID, EnemySpawnMode_ZoomIn);
		}
	}
	
	void View_Main::Render()
	{
#if 0
		StringBuilder<128> sb;
		sb.AppendFormat("GRS path: %s", g_GameState->m_GrsHttpClient->Path_get());
		RenderText(Vec2F(0.0f, (float)VIEW_SY), Vec2F((float)VIEW_SX, 0.0f), g_GameState->GetFont(Resources::FONT_CALIBRI_SMALL), SpriteColor_MakeF(1.0f, 1.0f, 1.0f, 0.2f), TextAlignment_Left, TextAlignment_Bottom, true, sb.ToString());
#endif
		
		StringBuilder<32> sb2;
		sb2.AppendFormat("build %d", Svn::Revision);
		RenderText(Vec2F(0.0f, (float)VIEW_SY), Vec2F((float)VIEW_SX, 0.0f), g_GameState->GetFont(Resources::FONT_CALIBRI_SMALL), SpriteColor_MakeF(1.0f, 1.0f, 1.0f, 0.2f), TextAlignment_Right, TextAlignment_Bottom, true, sb2.ToString());
		
		RenderLogo();
		
#ifdef DEBUG
#if 1
		int step = 10;
		
		int yoff = 0;
		
		for (int i = 0; i <= VIEW_SX; i += step)
		{
			float x1 = (float)i;
			float x2 = (float)(i + step);
			
			float hue = i / (float)VIEW_SX;
			
			SpriteColor color1 = Calc::Color_FromHue_NoLUT(hue);
			RenderRect(Vec2F(x1, 0.0f), Vec2F(x2 - x1, 10.0f), color1.v[0] / 255.0f, color1.v[1] / 255.0f, color1.v[2] / 255.0f, 1.0f, g_GameState->GetTexture(Textures::MENU_COLOR_WHITE));

#if defined(IPHONEOS)
			SpriteColor color2 = g_System.Color_FromHSB(hue, 1.0f, 1.0f);
			RenderRect(Vec2F(x1, 10.0f), Vec2F(x2 - x1, 20.0f), color2.v[0] / 255.0f, color2.v[1] / 255.0f, color2.v[2] / 255.0f, 1.0f, g_GameState->GetTexture(Textures::MENU_COLOR_WHITE));
#endif
			
			StringBuilder<32> sb3;
			sb3.AppendFormat("%.2f", hue);
			RenderText(Vec2F(x1, 20.0f + 7.0f * yoff), Vec2F(), g_GameState->GetFont(Resources::FONT_CALIBRI_SMALL), SpriteColors::White, TextAlignment_Left, TextAlignment_Top, true, sb3.ToString());
			
			yoff = (yoff + 1) % 3;
		}
#endif
#endif
	}
	
	int View_Main::RenderMask_get()
	{
		return RenderMask_Interface | RenderMask_Particles | RenderMask_WorldBackground | RenderMask_WorldPrimary;
	}
}
