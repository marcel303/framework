#include "GameRound.h"
#include "GameScore.h"
#include "GameState.h"
//#include "grs.h"
#include "GrsIntegration.h"
#include "grs_types.h"
#include "Menu_ScoreEntry.h"
#include "Menu_ScoreSubmit.h"
#include "MenuMgr.h"
#include "SocialAPI.h"
#include "StringEx.h"
#include "System.h"
#include "TempRender.h"
#include "TextureAtlas.h"
#include "Textures.h"
#include "UsgResources.h"
#include "View_KeyBoard.h"
#include "View_ScoreSubmit.h"
#include "World.h"

#ifdef IPHONEOS
	#include "GameCenter.h"
#endif
#ifdef BBOS
	#include "GameView_BBOS.h"
#endif

#define GLOW_INTERVAL 3.0f
#define SPARKLE_FREQUENCY_SUBMIT0 6.0f
#define SPARKLE_FREQUENCY_SUBMIT1 2.0f
#define SPARKLE_LIFE_SUBMIT0 1.0f
#define SPARKLE_LIFE_SUBMIT1 4.0f
#define SPARKLE_SPEED_SUBMIT0 60.0f
#define SPARKLE_SPEED_SUBMIT1 15.0f
#define DOT_RADIUS 46.0f
#define DOT_SPEED_SUBMIT0 (Calc::m2PI / 4.0f)
#define DOT_SPEED_SUBMIT1 (Calc::m2PI / 10.0f)
#define RING_CENTER Vec2F(VIEW_SX/2.0f, VIEW_SY/2.0f-26.0f)

namespace Game
{
	//static void Particle_UpdateY();
	
	View_ScoreSubmit::View_ScoreSubmit() : IView()
	{
		Initialize();
	}
	
	void View_ScoreSubmit::Initialize()
	{
		m_NoKeyBoard = false;
		
		// submission
		
		m_Database = ScoreDatabase_Local;
		
#ifdef IPHONEOS
		g_gameCenter->OnScoreSubmitComplete = CallBack(this, EH_GameCenterSubmitComplete);
		g_gameCenter->OnScoreSubmitError = CallBack(this, EH_GameCenterSubmitFailed);
#endif
		//g_GameState->m_GrsHttpClient->OnSubmitScoreResult = CallBack(this, Handle_SubmitResult);
		
		// animation
		
		m_ScreenLock.Initialize("SUBMITTING SCORE");
		m_DotGlowTimer.Initialize(g_GameState->m_TimeTracker_Global);
		m_DotGlowTimer.SetInterval(GLOW_INTERVAL);
		m_DotGlowAnim.Initialize(g_GameState->m_TimeTracker_Global, false);
		m_SparkleTimer.Initialize(g_GameState->m_TimeTracker_Global);
	}
	
	void View_ScoreSubmit::Database_set(ScoreDatabase database)
	{
		m_Database = database;
	}
	
	void View_ScoreSubmit::RefreshScoreView(Difficulty difficulty, int position)
	{
		Game::View_Scores* view = (Game::View_Scores*)g_GameState->GetView(::View_Scores);	
		
		view->Show(m_Database, difficulty, g_GameState->m_GameSettings->m_ScoreHistory);
		
		if (position >= 0)
		{
			view->ScrollIntoView(position);
		}
		else
		{
			view->ScrollUserIntoView();
		}
		
		LOG_DBG("View_ScoreSubmit::RefreshScoreView: position=%d", position);
	}
	
	static void SaveGlobal()
	{
#ifdef IPHONEOS
		const char * category = GameToGameCenter(g_GameState->m_GameRound->Modifier_Difficulty_get());
		const float score = (float)g_GameState->m_Score->Score_get();
		g_gameCenter->ScoreSubmit(category, score);
#elif defined(BBOS)
		const int mode = GameToScoreLoop(g_GameState->m_GameRound->Modifier_Difficulty_get());
		//const int level = g_GameState->m_GameRound->m_level;
		const int level = 0;
		const int score = g_GameState->m_Score->Score_get();
		g_GameState->m_Social->AsyncScoreSubmit(mode, level, score);
#elif 0
		GRS::HighScore score;
		
		score.Setup(
			GameToGrs(g_GameState->m_GameRound->m_WaveInfo.difficulty),
			g_GameState->m_GameSettings->m_GrsHistory,
			GRS::UserId(g_System.GetDeviceId()),
			g_GameState->KeyboardText_get(),
			"",
			(float)g_GameState->m_Score->Score_get(),
			GRS::TimeStamp::FromSystemTime(),
			GRS::GpsLocation(),
			GRS::CountryCode::FromString(g_System.GetCountryCode()),
			g_System.IsHacked());
		
		g_GameState->m_GrsHttpClient->SubmitHighScore(score);
#endif
	}
	
	static void SaveLocal()
	{
		const int gameMode = GameToGrsId(g_GameState->m_GameRound->Modifier_Difficulty_get());
		const std::string name = g_GameState->KeyboardText_get();
		const float value = (float)g_GameState->m_Score->Score_get();
		
		Score score;
		
		score.Setup(name, value);
		
		Game::View_Scores* view = (Game::View_Scores*)g_GameState->GetView(::View_Scores);
		
		view->ScoreBoard_get().Save(gameMode, &score);
	}
	
	void View_ScoreSubmit::HandleFocus()
	{
		g_World->IsPaused_set(true);
		
#ifdef IPHONEOS
		m_NoKeyBoard = g_gameCenter->IsLoggedIn();
#elif defined(BBOS)
		m_NoKeyBoard = true;
#else
		m_NoKeyBoard = false;
#endif
		
		if (m_NoKeyBoard || g_GameState->KeyboardView_get()->ReturnCode_get() == View_KeyBoard::ReturnCode_Ok)
		{
			if (m_NoKeyBoard == false)
			{
				g_GameState->m_GameSettings->m_PlayerName = g_GameState->KeyboardText_get().c_str();
				g_GameState->m_GameSettings->Save();
			}

			switch (m_Database)
			{
				case ScoreDatabase_Global:
				{
					g_GameState->Interface_get()->ActiveMenu_set(GameMenu::MenuType_ScoreSubmit);
					
					// reset states
					
					m_ScoreSubmitted = false;
					m_ResultRank = 0;
					m_ResultString = "";
					m_ScreenLock.Initialize("SUBMITTING SCORE");
					
					// perform the actual score submission
					
					if (m_NoKeyBoard == false)
						SaveLocal();
					else
					{
#ifdef IPHONEOS
						// todo: check if we're logged in to game center and submit locally
						//       with the player name from game center
#endif
					}
					SaveGlobal();
					
					break;
				}
					
				case ScoreDatabase_Local:
				{
					g_GameState->Interface_get()->ActiveMenu_set(GameMenu::MenuType_Empty);
					
					SaveLocal();
					
					break;
				}
			}
		}
		
		// animation
		
		m_ScreenLock.Start(true);
		m_DotAngle = 0.0f;
		m_DotGlowTimer.Start();
		m_SparkleTimer.SetFrequency(SPARKLE_FREQUENCY_SUBMIT0);
		m_SparkleTimer.Start();
	}
	
	void View_ScoreSubmit::HandleFocusLost()
	{
		m_ScreenLock.Start(false);
		m_SparkleTimer.Stop();
	}
	
	void View_ScoreSubmit::Update(float dt)
	{
		if (m_NoKeyBoard == false && g_GameState->KeyboardView_get()->ReturnCode_get() == View_KeyBoard::ReturnCode_Cancel)
		{
			g_GameState->ActiveView_set(::View_Scores);
		}
		else if (m_Database == ScoreDatabase_Local)
		{
			RefreshScoreView(g_GameState->m_GameRound->Modifier_Difficulty_get(), 0);
			
			g_GameState->ActiveView_set(::View_Scores);
		}
		else
		{
			UpdateAnimation(dt);
		}
	}
	
	void View_ScoreSubmit::Render()
	{
		// draw view name
		
		m_ScreenLock.Render();
		
		// draw background
		
		Vec2F backPos(65.0f, 55.0f);
		Vec2F backPos2(VIEW_SX - 65.0f, VIEW_SY - 70.0f);
		Vec2F backSize = backPos2 - backPos;
		
		// todo: alpha blended fade
		
		RenderRect(backPos, backSize, 0.0f, 0.0f, 0.0f, 0.5f, g_GameState->GetTexture(Textures::MENU_COLOR_WHITE));
				
		// draw result string
		
		// todo: alpha blended fade
		
		DrawResultString();
	}
	
	void View_ScoreSubmit::Render_Additive()
	{
		// draw ring
		
		DrawRing();
	}
	
	float View_ScoreSubmit::FadeTime_get()
	{
		return m_ScreenLock.FadeTime_get();
	}
	
	int View_ScoreSubmit::RenderMask_get()
	{
		return RenderMask_Interface | RenderMask_Particles | RenderMask_WorldBackground;
	}
	
	void View_ScoreSubmit::NextView()
	{
//		RefreshScoreView(g_GameState->m_GameRound->m_WaveInfo.difficulty, m_ResultRank);
		
		g_GameState->ActiveView_set(::View_Scores);
	}
	
#if defined(IPHONEOS)
	void View_ScoreSubmit::EH_GameCenterSubmitComplete(void * obj, void * arg)
	{
		View_ScoreSubmit * self = (View_ScoreSubmit*)obj;
		int rank = *(int*)arg;
		
		self->Handle_GameCenterSubmitComplete(rank);
	}
	void View_ScoreSubmit::EH_GameCenterSubmitFailed(void * obj, void * arg)
	{
		View_ScoreSubmit * self = (View_ScoreSubmit*)obj;
		
		self->Handle_GameCenterSubmitFailed();
	}
#endif
	
#if defined(IPHONEOS) || defined(BBOS) || defined(WIN32) || defined(MACOS)
	void View_ScoreSubmit::Handle_GameCenterSubmitComplete(int rank)
	{
		// update states
		
		m_ScoreSubmitted = true;
		m_ScreenLock.Initialize("SCORE SUBMITTED");
		
		// notify menu
		
		GameMenu::Menu_ScoreSubmit* menu = (GameMenu::Menu_ScoreSubmit*)g_GameState->m_MenuMgr->Menu_get(GameMenu::MenuType_ScoreSubmit);
		menu->Handle_SubmitResult();
		
		// scroll submitted score into view
		
#ifdef BBOS
		if (gGameView->m_GamepadIsEnabled)
			m_ResultString = "Press [X] to continue";
		else
			m_ResultString = "Touch to continue";
#else
		m_ResultString = "Touch to continue";
#endif
		
		m_ResultRank = rank;
		
		RefreshScoreView(g_GameState->m_GameRound->Modifier_Difficulty_get(), rank);
		
		m_SparkleTimer.SetFrequency(SPARKLE_FREQUENCY_SUBMIT1);
	}
	
	void View_ScoreSubmit::Handle_GameCenterSubmitFailed()
	{
		// update states
		
		m_ScoreSubmitted = true;
		m_ScreenLock.Initialize("SCORE SUBMIT FAILED");
		
		// notify menu
		
		GameMenu::Menu_ScoreSubmit* menu = (GameMenu::Menu_ScoreSubmit*)g_GameState->m_MenuMgr->Menu_get(GameMenu::MenuType_ScoreSubmit);
		
		menu->Handle_SubmitResult();
		
		// scroll best rank into view
		
		RefreshScoreView(g_GameState->m_GameRound->Modifier_Difficulty_get(), 0);

#ifdef BBOS
		if (gGameView->m_GamepadIsEnabled)
			m_ResultString = "Press [X] to continue";
		else
			m_ResultString = "Touch to continue";
#else
		m_ResultString = "Touch to continue";
#endif
		
		m_SparkleTimer.SetFrequency(SPARKLE_FREQUENCY_SUBMIT1);
	}
#else
	void View_ScoreSubmit::Handle_SubmitResult(void* obj, void* arg)
	{
		View_ScoreSubmit* self = (View_ScoreSubmit*)obj;
		
		GRS::QueryResult* result = (GRS::QueryResult*)arg;
		
		// update states
		
		self->m_ScoreSubmitted = true;
		self->m_ScreenLock.Initialize("SCORE SUBMITTED");
		
		// notify menu
		
		GameMenu::Menu_ScoreSubmit* menu = (GameMenu::Menu_ScoreSubmit*)g_GameState->m_MenuMgr->Menu_get(GameMenu::MenuType_ScoreSubmit);
		
		menu->Handle_SubmitResult();
		
		if (result->m_RequestResult == GRS::RequestResult_Ok)
		{
			// scroll submitted score into view
			
			self->RefreshScoreView(g_GameState->m_GameRound->Modifier_Difficulty_get(), result->m_RankCurr);
			
			self->m_ResultString = "Click to continue";
			self->m_ResultRank = result->m_RankCurr;
		}
		else
		{
			// scroll best rank into view
			
			self->RefreshScoreView(g_GameState->m_GameRound->Modifier_Difficulty_get(), 0);
			
//			self->m_ResultString = String::FormatC("Failed: %s", result->m_Error).c_str();
			self->m_ResultString = "Failed to submit score";
		}
		
		self->m_SparkleTimer.SetFrequency(SPARKLE_FREQUENCY_SUBMIT1);
	}
#endif
	
	// --------------------
	// Drawing
	// --------------------
	
	void View_ScoreSubmit::DrawRing()
	{
		// todo: alpha blended fade
		
		// draw ring graphic
		
		g_GameState->Render(g_GameState->GetShape(Resources::SCORESUBMIT_RINGU), RING_CENTER, 0.0f, SpriteColors::White);
		
		// draw dot graphic (rotated)
		
		g_GameState->Render(g_GameState->GetShape(Resources::SCORESUBMIT_DOT), DotPosition_get(), 0.0f, SpriteColors::White);
		
		if (m_ScoreSubmitted)
		{
			// draw 'touch' button
			
			g_GameState->Render(g_GameState->GetShape(Resources::SCORESUBMIT_OK), RING_CENTER, 0.0f, SpriteColors::White);
		}
	}
	
	void View_ScoreSubmit::DrawResultString()
	{
		const uint32_t offset = (VIEW_SY-320)/2;
		const float y1 = 208.0f + offset;
		const float y2 = 228.0f + offset;
		
		const Vec2F pos(0.0f, y1);
		const Vec2F size((float)VIEW_SX, y2 - y1);
		
		const FontMap* font = g_GameState->GetFont(Resources::FONT_CALIBRI_LARGE);
		
		if (m_ScoreSubmitted)
		{
			// draw result string
			
			RenderText(pos, size, font, SpriteColors::White, TextAlignment_Center, TextAlignment_Center, true, m_ResultString.c_str());
		}
		else
		{
			// draw waiting text
			
			RenderText(pos, size, font, SpriteColors::White, TextAlignment_Center, TextAlignment_Center, true, "Please be patient..");
		}
	}
	
	// --------------------
	// Animation
	// --------------------
	
	void View_ScoreSubmit::UpdateAnimation(float dt)
	{
		if (m_ScoreSubmitted)
			m_DotAngle += DOT_SPEED_SUBMIT1 * dt;
		else
			m_DotAngle += DOT_SPEED_SUBMIT0 * dt;

		while (m_DotGlowTimer.ReadTick())
		{
			m_DotGlowAnim.Start(AnimTimerMode_FrameBased, true, 15, AnimTimerRepeat_None);
		}
		
		while (m_SparkleTimer.ReadTick())
		{
			// spawn sparkly particle
			
			const Vec2F dotPos = DotPosition_get();
			
			Particle& p = g_GameState->m_ParticleEffect_UI.Allocate(g_GameState->GetTexture(Textures::SCORESUBMIT_PARTICLE)->m_Info, 0, 0);
			
			const float speed = m_ScoreSubmitted ? SPARKLE_SPEED_SUBMIT1 : SPARKLE_SPEED_SUBMIT0;
			const Vec2F speedVec = Vec2F::FromAngle(m_DotAngle + Calc::mPI2) * speed;
			
			const float life = m_ScoreSubmitted ? SPARKLE_LIFE_SUBMIT1 : SPARKLE_LIFE_SUBMIT0;
			
			Particle_RotMove_Setup(&p, dotPos[0], dotPos[1], life, 30.0f, 30.0f, Calc::Random(Calc::mPI2), speedVec[0], speedVec[1], Calc::Random(-Calc::m4PI, Calc::m4PI));
		}
	}
	
	Vec2F View_ScoreSubmit::DotPosition_get() const
	{
		return RING_CENTER + Vec2F::FromAngle(m_DotAngle) * DOT_RADIUS;
	}
}
