#include "ArrayStream.h"
#include "Atlas_ImageInfo.h"
#include "GameSettings.h"
#include "GameState.h"
#include "GameTypes.h"
#include "MemoryStream.h"
#include "MenuMgr.h"
#include "PspSaveData.h"
#include "StringBuilder.h"
#include "TempRender.h"
#include "Textures.h"
#include "Timer.h"
#include "UsgResources.h"
#include "Util_ColorEx.h"
#include "View_ScoresPSP.h"

#include "EntityPlayer.h"
#include "GameRound.h"
#include "Grid_Effect.h"
#include "World.h"

namespace Game
{
#define SCORE_TEXT_HEIGHT 35.0f

	PspScoreScroller::PspScoreScroller()
	{
		mWaitTimer.Initialize(&g_TimerRT);
		mScrollTimer.Initialize(g_GameState->m_TimeTracker_Global, false);
		Reset();
	}

	void PspScoreScroller::Update(float dt)
	{
		switch (mState)
		{
		case State_ScrollUpWait:
			mProgress = 1.0f;
			if (mWaitTimer.ReadTick())
				State_set(State_ScrollUp);
			break;
		case State_ScrollUp:
			mProgress = mScrollTimer.Progress_get();
			if (!mScrollTimer.IsRunning_get())
				State_set(State_ScrollDownWait);
			break;
		case State_ScrollDownWait:
			mProgress = 0.0f;
			if (mWaitTimer.ReadTick())
				State_set(State_ScrollDown);
			break;
		case State_ScrollDown:
			mProgress = mScrollTimer.Progress_get();
			if (!mScrollTimer.IsRunning_get())
				State_set(State_ScrollUpWait);
			break;
		}

		LOG_DBG("state=%d, progress=%g", mState, mProgress);
	}

	void PspScoreScroller::Reset()
	{
		State_set(State_ScrollDownWait);
		mProgress = 0.0f;
	}

	float PspScoreScroller::Progress_get() const
	{
		return mProgress;
	}

	void PspScoreScroller::State_set(State state)
	{
		mState = state;

		switch (mState)
		{
		case State_ScrollUpWait:
			mWaitTimer.SetInterval(2.0f);
			mWaitTimer.Start();
			break;
		case State_ScrollUp:
			mScrollTimer.Start(AnimTimerMode_TimeBased, true, 3.0f, AnimTimerRepeat_None);
			break;
		case State_ScrollDownWait:
			mWaitTimer.SetInterval(3.5f);
			mWaitTimer.Start();
			break;
		case State_ScrollDown:
			mScrollTimer.Start(AnimTimerMode_TimeBased, false, 3.0f, AnimTimerRepeat_None);
			break;
		}
	}

	//

	View_ScoresPSP::View_ScoresPSP()
	{
		mScoreDB = 0;
		mSelectedGameMode = -1;
		mSelectedDifficulty = Difficulty_Unknown;
		mSelectedScoreList = 0;
		mSelectedScoreListPrev = 0;
		mAnimState = 1;
		mAnimTimer.Initialize(g_GameState->m_TimeTracker_Global, false);
		mScoreBlinkTimer.Initialize(g_GameState->m_TimeTracker_Global, false);
		mScoreBlinkTimer.Start(AnimTimerMode_TimeBased, false, 2.0f, AnimTimerRepeat_Mirror);
	}

	View_ScoresPSP::~View_ScoresPSP()
	{
		delete mScoreDB;
		mScoreDB = 0;
	}

	void View_ScoresPSP::Initialize()
	{
		Assert(mScoreDB == 0);

		mScoreDB = new PspScoreDB();
		mScreenLock.Initialize("HIGHSCORE");
		mFadeTimer.Initialize(g_GameState->m_TimeTracker_Global, false);

		Load();

		mSelectedGameMode = GameType_Classic;
		mSelectedDifficulty = Difficulty_Easy;

#if defined(WIN32)
		for (int i = 0; i < 10; ++i)
		{
			Submit(0, Difficulty_Easy, rand() % 100, "test", false);
			Submit(0, Difficulty_Hard, rand() % 100, "testAAA", false);
			Submit(0, Difficulty_Custom, rand() % 100, "testgfgd", false);
		}

		Submit(0, Difficulty_Easy, 100, "!!!!!", true);

		Show(0, Difficulty_Easy);
#endif
	}
	
	void View_ScoresPSP::Load()
	{
		try
		{
			uint8_t bytes[65536];
			int byteCount = sizeof(bytes);

#if defined(PSP)
			if (PspSaveData_Load(PSPSAVE_APPNAME, PSPSAVE_SCORES, bytes, byteCount, byteCount) == false)
			{
				mScoreDB->Clear();
				return;
			}
#else
			byteCount = 0;
#endif

			ArrayStream stream(bytes, byteCount);

			StreamReader reader(&stream, false);

			mScoreDB->Load(reader);
		}
		catch (std::exception& e)
		{
			LOG_ERR("unable to load scores: %s. discarding current save", e.what());

#if defined(PSP)
			PspSaveData_Remove(PSPSAVE_APPNAME, PSPSAVE_SCORES);
#endif
		}
	}

	void View_ScoresPSP::Save()
	{
		try
		{
			MemoryStream stream;

			StreamWriter writer(&stream, false);

			mScoreDB->Save(writer);

#if defined(PSP)
			PspSaveData_Save(PSPSAVE_APPNAME, PSPSAVE_SCORES, PSPSAVE_DESC, PSPSAVE_DESC_LONG, stream.Bytes_get(), stream.Length_get(), true);
#else
#endif
		}
		catch (std::exception& e)
		{
			LOG_ERR("unable to save scores: %s", e.what());
		}
	}

	void View_ScoresPSP::Submit(int gameMode, Difficulty difficulty, int score, const char* name, bool save)
	{
		PspScoreKey key(gameMode, difficulty);

		PspScoreList* list = mScoreDB->Get(key);

		PspScore temp(score, name);

		list->Add(temp);

		if (save)
		{
			Save();
		}
	}
	
	void View_ScoresPSP::Show(int gameMode, Difficulty difficulty)
	{
		ChangeGameMode(gameMode, false);
		ChangeDifficulty(difficulty, false);

		Refresh();
	}

	void View_ScoresPSP::ChangeGameMode(int gameMode, bool refresh)
	{
		mSelectedGameMode = gameMode;

		if (refresh)
			Refresh();
	}

	void View_ScoresPSP::ChangeDifficulty(Difficulty difficulty, bool refresh)
	{
		mSelectedDifficulty = difficulty;

		if (refresh)
			Refresh();
	}

	void View_ScoresPSP::Refresh()
	{
		PspScoreKey key(mSelectedGameMode, mSelectedDifficulty);

		mSelectedScoreListPrev = mSelectedScoreList;
		mSelectedScoreList = mScoreDB->Get(key);

		mAnimState = 0;
		mAnimTimer.Stop();

		mScoreScroller.Reset();
	}

	// --------------------
	// View
	// --------------------
	void View_ScoresPSP::Update(float dt)
	{
		mScoreScroller.Update(dt);

		if (!mAnimTimer.IsRunning_get())
		{
			if (mAnimState != 2)
			{
				mAnimTimer.Start(AnimTimerMode_TimeBased, false, 1.0f, AnimTimerRepeat_None);
				mAnimState++;
			}
		}

		// update world

		Assert(g_GameState->m_GameRound->GameMode_get() == GameMode_IntroScreen);
		
		g_GameState->m_GameRound->Update(dt);
		
		g_World->m_GridEffect->BaseHue_set(g_GameState->m_TimeTracker_Global->Time_get() / 50.0f);
		g_World->m_GridEffect->Impulse(Vec2F(Calc::Random(0.0f, (float)WORLD_SX), Calc::Random(0.0f, (float)WORLD_SY)), 0.1f);
		
		g_GameState->ClearSB();
		g_World->Update(dt);

#if 1
		//while (m_SpawnTimer.ReadTick())
		static int i = 0; i++;
		if (i % 600 == 0)
		{
			EntityClass type = g_GameState->m_GameRound->Classic_GetEnemyType();
			float interval = 0.1f;
			float angle = Calc::Random(Calc::m2PI);
			float arc = Calc::Random(0.2f, 1.0f) * Calc::m2PI;
			int count = (int)Calc::DivideUp(arc, 0.5f);
			float radius1 = 80.0f;
			float radius2 = 160.0f;
			
			EnemyWave wave;
			wave.MakeCircle(type, g_World->m_Player->Position_get(), radius1, radius2, angle, arc, count, interval);
			g_GameState->m_GameRound->Classic_WaveAdd(wave);
			
//			g_World->SpawnEnemy(g_GameState->m_GameRound->GetEnemyType(), WORLD_MID, EnemySpawnMode_ZoomIn);
		}
#endif
	}

	void View_ScoresPSP::Render()
	{
		PspScoreList* scoreList = mAnimState == 0 ? mSelectedScoreListPrev : mSelectedScoreList;

		float fadeOut = 1.0f - mFadeTimer.Progress_get();

		const Vec2F size(VIEW_SX - 70.0f * 2.0f, 25.0f);
		float fadeYa1 = 30.0f;
		float fadeYa2 = 0.0f;

		float fadeYb1 = VIEW_SY - SCORE_TEXT_HEIGHT - 110.0f;
		float fadeYb2 = VIEW_SY - SCORE_TEXT_HEIGHT - 80.0f;

		int visibleCount = 4;
		int scrollCount =  Calc::Min(12, scoreList->ScoreCount);
		float scrollSize = Calc::Max(0, scrollCount - visibleCount) * SCORE_TEXT_HEIGHT;
		//float offset = cosf(g_TimerRT.Time_get()) * 100.0f;
		float offset = -mScoreScroller.Progress_get() * scrollSize;
		Vec2F position(70.0f, 30.0f + offset);

		float fadeX = mAnimState == 0 ? 1.f - mAnimTimer.Progress_get() : mAnimState == 1 ? mAnimTimer.Progress_get() : 1.0f;

		//const float a = 0.7f + sinf(g_TimerRT.Time_get()) * 0.3f;
		//const float a = 0.3f;
		//RenderRect(Vec2F(position[0], fadeYa2), Vec2F(size[0], size[1] + fadeYb2 - fadeYa2), 0.0f, 0.0f, 0.0f, a, g_GameState->GetTexture(Textures::COLOR_WHITE));
		const AtlasImageMap* backImage = g_GameState->GetTexture(Textures::COLOR_WHITE);
		g_GameState->DataSetActivate(backImage->m_TextureAtlas->m_DataSetId);
		SpriteGfx* gfx = g_GameState->m_SpriteGfx;
		gfx->Reserve(4, 6);
		gfx->WriteBegin();
		const float u = backImage->m_Info->m_TexCoord[0];
		const float v = backImage->m_Info->m_TexCoord[1];
		//const SpriteColor baseColor = Calc::Color_FromHue(g_World->m_GridEffect->BaseHue_get());
		const SpriteColor baseColor = Calc::Color_FromHue(g_World->m_GridEffect->BaseHue_get() + 0.05f);
		const SpriteColor color1 = SpriteColor_Make(baseColor.v[0], baseColor.v[1], baseColor.v[2], (int)(255 * fadeOut));
		const SpriteColor color2 = SpriteColor_Make(baseColor.v[0], baseColor.v[1], baseColor.v[2], 0);
		gfx->WriteVertex(0.0f,         0.0f,         color1.rgba, u, v);
		gfx->WriteVertex(VIEW_SX/2.0f, 0.0f,         color2.rgba, u, v);
		gfx->WriteVertex(VIEW_SX/2.0f, VIEW_SY/1.0f, color2.rgba, u, v);
		gfx->WriteVertex(0.0f,         VIEW_SY/1.0f, color1.rgba, u, v);
		gfx->WriteIndex3(0, 1, 2);
		gfx->WriteIndex3(0, 2, 3);
		gfx->WriteEnd();

		if (scoreList)
		{
			for (int index = 0; index < scrollCount; ++index)
			{
				PspScore* score = scoreList->ScoreList + index;

				// todo: render scores

				const float fadeA = Calc::Mid(1.0f - (position.y - fadeYa1) / (fadeYa2 - fadeYa1), 0.0f, 1.0f);
				const float fadeB = Calc::Mid(1.0f - (position.y - fadeYb1) / (fadeYb2 - fadeYb1), 0.0f, 1.0f);
				const float fade = fadeX * fadeA * fadeB * fadeOut;

				StringBuilder<64> sb;
				sb.AppendFormat("%d: %s", score->Score, score->Name.c_str());

#if 0
				RenderText(
					position,
					size,
					g_GameState->GetFont(Resources::FONT_USUZI_SMALL),
					SpriteColor_MakeF(1.0f, 1.0f, 1.0f, fade),
					TextAlignment_Center, TextAlignment_Center,
					false,
					sb.ToString());

				position[1] += size[1];
#else
				float dx = 20.0f;
				Vec2F posR(position);
				Vec2F sizeR(size[0], SCORE_TEXT_HEIGHT);
				Vec2F pos1(position);
				Vec2F size1(size[0], SCORE_TEXT_HEIGHT * 0.5f);
				Vec2F pos2 = pos1 + Vec2F(dx, SCORE_TEXT_HEIGHT * 0.5f);
				Vec2F size2 = size1 - Vec2F(dx, 0.0f);
				
				float t = 0.5f + 0.5f * mScoreBlinkTimer.Progress_get();
				int m_FocusIndex = 0;
				
				SpriteColor color = SpriteColor_MakeF(t, t * 0.95f, t * 0.90f, fade);
				
				if (index == m_FocusIndex)
				{
					RenderRect(posR, sizeR, 0.3f, 0.6f, 1.0f, 0.3f * fade, g_GameState->GetTexture(Textures::MENU_COLOR_WHITE));
				}
				
				StringBuilder<32> sb1;
				sb1.AppendFormat("#%d | %s", index + 1, score->Name.c_str());
				StringBuilder<32> sb2;
				sb2.AppendFormat("%08d", score->Score);
				RenderText(pos1, size1, g_GameState->GetFont(Resources::FONT_USUZI_SMALL), color, TextAlignment_Left, TextAlignment_Top, false, sb1.ToString());
				RenderText(pos2, size2, g_GameState->GetFont(Resources::FONT_USUZI_SMALL), SpriteColor_MakeF(1.0f, 1.0f, 1.0f, fade), TextAlignment_Left, TextAlignment_Top, false, sb2.ToString());

				position[1] += SCORE_TEXT_HEIGHT;
#endif
			}
		}

		mScreenLock.Render();
	}

	int View_ScoresPSP::RenderMask_get()
	{
		//return RenderMask_Interface | RenderMask_Particles | RenderMask_WorldBackground;
		return RenderMask_Interface | RenderMask_Particles | RenderMask_WorldBackground | RenderMask_WorldPrimary;
	}
	
	// --------------------
	// View
	// --------------------
	float View_ScoresPSP::FadeTime_get()
	{
		return mScreenLock.FadeTime_get();
	}

	void View_ScoresPSP::HandleFocus()
	{
#if defined(PSP_UI)
		g_GameState->Interface_get()->ActiveMenu_set(GameMenu::MenuType_ScoresPSP);
#endif

		Show(mSelectedGameMode, mSelectedDifficulty);

		mScreenLock.Start(true);

		mFadeTimer.Stop();

		g_World->IsPaused_set(false);
	}

	void View_ScoresPSP::HandleFocusLost()
	{
		mScreenLock.Start(false);

		mFadeTimer.Start(AnimTimerMode_TimeBased, false, mScreenLock.FadeTime_get(), AnimTimerRepeat_None);

		g_World->IsPaused_set(true);
	}
}
