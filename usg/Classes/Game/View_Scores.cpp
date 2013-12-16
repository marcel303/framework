#include "GameState.h"
#include "GrsIntegration.h"
#include "grs_types.h"
#include "MenuMgr.h"
#include "SocialAPI.h"
#include "SocialIntegration.h"
#include "StringBuilder.h"
#include "System.h"
#include "TempRender.h"
#include "Textures.h"
#include "UsgResources.h"
#include "View_Scores.h"
#include "World.h"

#if defined(IPHONEOS)
#include "GameCenter.h"
#endif
#if defined(BBOS)
#include "GameView_BBOS.h"
#include "SocialAPI_ScoreLoop.h"
#endif

#define RETAIN_SCORES 0

namespace Game
{
#define SCORE_TEXT_HEIGHT 40.0f
	
	View_Scores::View_Scores()
	{
		m_CacheBorder = 10;
		m_LocalLoaded = false;
		m_GrsId = 0;
		m_History = 7;
		m_ScoreCount = -1;
		m_BestRank = -1;
		m_ScorePosition = 0.0f;
		m_ScrollSpeed = 0.0f;
		m_ScrollSpeedTemp = 0.0f;
		m_ScoreVisibleCount = VIEW_SY / 40;
		m_FocusIndex = -1;
	}
	
	void View_Scores::Initialize()
	{
		TouchListener listener;
		listener.Setup(this, HandleTouchBegin, HandleTouchEnd, HandleTouchMove);
		g_GameState->m_TouchDLG->Register(USG::TOUCH_PRIO_SCOREVIEW, listener);
		
#if defined(BBOS) || defined(IPAD)
		m_Rect = RectF(Vec2F(0.0f, 0.0f), Vec2F((float)VIEW_SX, (float)VIEW_SY));
#else
		m_Rect = RectF(Vec2F(VIEW_SX/2.0f-140.0f, 0.0f), Vec2F(240.0f, (float)VIEW_SY));
#endif
		
		m_ScoreBlinkTimer.Initialize(g_GameState->m_TimeTracker_Global, false);
		m_ScoreBlinkTimer.Start(AnimTimerMode_TimeBased, false, 2.0f, AnimTimerRepeat_Mirror);
		
#ifdef IPHONEOS
		g_gameCenter->OnScoreListComplete = CallBack(this, HandleQueryListResult);
#endif
	}
	
	void View_Scores::GrsClient_set(GRS::HttpClient* client)
	{
	}
	
	void View_Scores::Update(float dt)
	{
		// update animation
		
		const float falloff = powf(0.1f, dt);
		
		m_ScrollSpeed *= falloff;
		
		Scroll(m_ScrollSpeed * dt);
		
		ScrollClamp(dt);
	}
	
	void View_Scores::Render()
	{
		g_GameState->RenderBGFade(.7f);
		
		// render entries (from cache)

		// note: start at -1 so score moving out of view gets drawn as well
		
		for (int i = -1; i <= m_ScoreVisibleCount; ++i)
		{
			const int index = ScrollPosition_get() + i;
			const float offset = ScrollAnimationOffset_get();
			
			const SocialScore* score = CacheQuery(index);
			
			if (!score)
				continue;
			
		#if defined(BBOS)
			float xoffset = VIEW_SX/2.0f-140.0f;
		#elif defined(IPAD)
			float xoffset = VIEW_SX/2.0f-140.0f;
		#else
			float xoffset = m_Rect.m_Position[0];
		#endif

			float dx = 20.0f;
			Vec2F posR(0.0f, m_Rect.m_Position[1] + (i - offset) * SCORE_TEXT_HEIGHT);
			//Vec2F sizeR(m_Rect.m_Size[0], SCORE_TEXT_HEIGHT);
			Vec2F sizeR(VIEW_SX, SCORE_TEXT_HEIGHT);
			Vec2F pos1(xoffset, m_Rect.m_Position[1] + (i - offset) * SCORE_TEXT_HEIGHT);
			Vec2F size1(m_Rect.m_Size[0], SCORE_TEXT_HEIGHT * 0.5f);
			Vec2F pos2 = pos1 + Vec2F(dx, SCORE_TEXT_HEIGHT * 0.5f);
			Vec2F size2 = size1 - Vec2F(dx, 0.0f);
			
			float t = 0.5f + 0.5f * m_ScoreBlinkTimer.Progress_get();
			
			SpriteColor color = SpriteColor_MakeF(t, t, t, 1.0f);
			
			if (index == m_FocusIndex)
			{
				RenderRect(posR, sizeR, 0.3f, 0.6f, 1.0f, 0.3f, g_GameState->GetTexture(Textures::MENU_COLOR_WHITE));
			}
			
			StringBuilder<32> sb1;
			sb1.AppendFormat("#%d | %s", index + 1, score->m_userName.c_str());
			StringBuilder<32> sb2;
			sb2.AppendFormat("%08d", (int)score->m_value);
			RenderText(pos1, size1, g_GameState->GetFont(Resources::FONT_USUZI_SMALL), color, TextAlignment_Left, TextAlignment_Top, false, sb1.ToString());
			RenderText(pos2, size2, g_GameState->GetFont(Resources::FONT_USUZI_SMALL), SpriteColors::White, TextAlignment_Left, TextAlignment_Top, false, sb2.ToString());
		}
	}
	
	int View_Scores::RenderMask_get()
	{
		return RenderMask_Interface | RenderMask_Particles | RenderMask_WorldBackground;
	}
	
	void View_Scores::Show(ScoreDatabase database, Difficulty difficulty, int history)
	{	
		int grsId = GameToGrsId(difficulty);
		
		bool change =
			database != m_Database ||
			grsId != m_GrsId ||
			history != m_History;
		
		if (!change)
		{
			ScrollReset();
			return;
		}
		
		m_Database = database;
		m_GrsId = grsId;
		m_History = history;
			
#if defined(DEBUG) && 0
		Score score;
		score.Setup("testuser", 100.0f);
		m_ScoreBoard.Save(m_GrsId, &score);
#endif
		
		Refresh();
	}
	
	void View_Scores::ShowGrsId(int grsId)
	{
		m_GrsId = grsId;
		
		Refresh();
	}
	
	void View_Scores::ShowHistory(int history)
	{
		m_History = history;
		
		Refresh();
	}
	
	void View_Scores::Refresh()
	{
		LOG_INF("scores: refresh", 0);
		
		m_ScoreCount = -1;
		
		m_BestRank = -1;
		
		m_FocusIndex = -1;
		
		ScrollReset();
		
		CacheClear();
		
		CacheComplete();
		
#if defined(IPHONEOS) && defined(DEBUG) && 0
		g_gameCenter->ScoreSubmit("easy", 1000);
		g_gameCenter->ScoreSubmit("hard", 2000);
		g_gameCenter->ScoreSubmit("custom", 3000);
#endif
#if defined(BBOS) && defined(DEBUG) && 0
		gGameView->m_ScoreLoopMgr->ScoreSubmit(0, 0, 1000);
		gGameView->m_ScoreLoopMgr->ScoreSubmit(1, 0, 2000);
		gGameView->m_ScoreLoopMgr->ScoreSubmit(2, 0, 3000);
#endif
	}
			
	int View_Scores::ScoreCount_get()
	{
		return m_ScoreCount;
	}
	
	int View_Scores::BestRank_get()
	{
		return m_BestRank;
	}
	
	// ----------------------------------------
	// Database
	// ----------------------------------------
	
	ScoreDatabase View_Scores::DefaultDatabase_get()
	{
		// decide network connectivity, select database
		
		bool showGlobal = true;
		
		#if defined(IPHONEOS)
		showGlobal = g_gameCenter->IsLoggedIn();
		#elif defined(BBOS)
		showGlobal = true;
		#else
		showGlobal = g_System.HasNetworkConnectivity_get();
		#endif
		
		if (showGlobal)
			return ScoreDatabase_Global;
		else
			return ScoreDatabase_Local;
	}
	
	ScoreDatabase View_Scores::Database_get()
	{
		return m_Database;
	}
	
	void View_Scores::Database_set(ScoreDatabase database)
	{
		m_Database = database;
	}
	
	ScoreBoard& View_Scores::ScoreBoard_get()
	{
		return m_ScoreBoard;
	}
	
	// ----------------------------------------
	// Querying
	// ----------------------------------------
	void View_Scores::HandleScoreList(SocialScoreList & scores, bool focus)
	{
		#if defined(BBOS) || 1
		if (focus)
		{
			// dirty hack for BBOS!

			if (scores.scores.size() >= 1)
			{
				int position = scores.scores[0].m_rank;

				LOG_INF("focus at position %d for user", position);

				ScrollIntoView(position);
			}

			return;
		}
		#endif

		CacheAdd(scores);

		if (scores.scoreCount != -1)
		{
			m_ScoreCount = scores.scoreCount;
		}
	}

	void View_Scores::HandleQueryListResult(void* obj, void* arg)
	{
		View_Scores* self = (View_Scores*)obj;
		GRS::QueryResult* queryResult = (GRS::QueryResult*)arg;
		
		SocialScoreList scores;

		for (size_t i = 0; i < queryResult->m_Scores.size(); ++i)
		{
			SocialScore score(
				queryResult->m_GameMode,
				0,
				queryResult->m_Scores[i].m_Score,
				queryResult->m_ResultBegin + i,
				queryResult->m_Scores[i].m_UserName);

			scores.scores.push_back(score);
		}

		self->CacheAdd(scores);
		
		self->m_ScoreCount = queryResult->m_ScoreCount;
		self->m_BestRank = queryResult->m_RankBest;
	}
	
	// ----------------------------------------
	// Scrolling
	// ----------------------------------------
	int View_Scores::ScrollPosition_get() const
	{
		return (int)Calc::RoundNearest(m_ScorePosition);
	}
	
	float View_Scores::ScrollAnimationOffset_get() const
	{
		return m_ScorePosition - ScrollPosition_get();
	}
	
	void View_Scores::Scroll(float offset)
	{
//		LOG(LogLevel_Debug, "scroll: %f", offset);
		
		ScrollAbsolute(m_ScorePosition + offset);
	}
	
	void View_Scores::ScrollAbsolute(float position)
	{
		m_ScorePosition = position;
		
		CacheComplete();
	}
	
	void View_Scores::ScrollReset()
	{
		m_ScorePosition = 0.0f;
	}
	
	void View_Scores::ScrollClamp(float dt)
	{
		if (m_ScrollActive)
			return;
		
		dt *= 3.0f;
		
		float delta;
		
		float falloff = 0.1f;
		
		delta = m_ScorePosition;
		
		if (delta < 0.0f)
			m_ScorePosition -= delta * (1.0f - powf(falloff, dt));
		
		if (m_ScoreCount >= 0)
		{
			const int listSize = m_ScoreCount - m_ScoreVisibleCount;
			
			delta = listSize - m_ScorePosition;
			
			if (delta < 0.0f)
				m_ScorePosition += delta * (1.0f - powf(falloff, dt));
		}
		/*
		
		if (m_ScorePosition < 0.0f)
			m_ScorePosition = 0.0f;*/
	}
	
	void View_Scores::ScrollIntoView(int position)
	{
		CacheClear();
		
		m_ScrollSpeed = 0.0f;
		
		m_ScorePosition = position - m_ScoreVisibleCount / 2.0f;
		
		if (m_ScorePosition < 0.0f)
			m_ScorePosition = 0.0f;
		
		m_FocusIndex = position;
		
		CacheComplete();
	}
	
	void View_Scores::ScrollUserIntoView()
	{
#if defined(BBOS)
		Difficulty difficulty;
		GrsIdToGame(m_GrsId, difficulty);
		const int mode = GameToScoreLoop(difficulty);

		int count = m_ScoreVisibleCount + m_CacheBorder;

		g_GameState->m_Social->AsyncScoreListAroundUser(mode, m_History, 1);
#endif
	}

	// --------------------
	// View
	// --------------------
	
	void View_Scores::HandleFocus()
	{
		g_GameState->Interface_get()->ActiveMenu_set(GameMenu::MenuType_Scores);
		
		g_GameState->m_TouchDLG->Enable(USG::TOUCH_PRIO_SCOREVIEW);
		
		m_ScrollActive = false;

#if defined(DEBUG) && 0
		SocialSCUI_ShowUI();
#endif
	}
	
	void View_Scores::HandleFocusLost()
	{
		g_GameState->m_TouchDLG->Disable(USG::TOUCH_PRIO_SCOREVIEW);
	}
	
	// --------------------
	// Touch related
	// --------------------
	
	bool View_Scores::HandleTouchBegin(void* obj, const TouchInfo& touchInfo)
	{
		View_Scores* self = (View_Scores*)obj;
		
		if (!self->m_Rect.IsInside(touchInfo.m_LocationView))
			return false;
		
		self->m_ScrollSpeed = 0.0f;
		self->m_ScrollSpeedTemp = 0.0f;
		self->m_ScrollActive = true;
		
		return true;
	}
	
	bool View_Scores::HandleTouchMove(void* obj, const TouchInfo& touchInfo)
	{
		View_Scores* self = (View_Scores*)obj;
		
		self->Scroll(-touchInfo.m_LocationDelta[1] / SCORE_TEXT_HEIGHT);
		
		self->m_ScrollSpeedTemp = -touchInfo.m_LocationDelta[1] / SCORE_TEXT_HEIGHT * 60.0f;
		
		g_World->SpawnZoomParticles(touchInfo.m_Location, 10);
		
		return true;
	}
	
	bool View_Scores::HandleTouchEnd(void* obj, const TouchInfo& touchInfo)
	{
		View_Scores* self = (View_Scores*)obj;
		
#ifdef BBOS
		// don't scroll unless the user is really sweeping across the screen
		// this makes it a little bit easier to use the buttons next to the name
		if (Calc::Abs(self->m_ScrollSpeedTemp) < 5.f)
			self->m_ScrollSpeedTemp = 0.f;
#endif

		self->m_ScrollSpeed = self->m_ScrollSpeedTemp;
//		self->m_ScrollSpeed = 1.0f;
		self->m_ScrollActive = false;
		
		return true;
	}

	// ----------------------------------------
	// Caching
	// ----------------------------------------
	void View_Scores::CacheClear()
	{
		#if RETAIN_SCORES
		for (std::map<int, SocialScore>::iterator i = m_Cache.begin(); i != m_Cache.end(); ++i)
		{
			SocialScore & score = i->second;
			g_GameState->m_Social->ScoreRelease(score);
			score.m_user = NULL;
		}
		#endif

		m_Cache.clear();
		
		m_LocalLoaded = false;
	}
	
	void View_Scores::CacheAdd(SocialScoreList & scores)
	{
		for (size_t i = 0; i < scores.scores.size(); ++i)
		{
			SocialScore & score = scores.scores[i];
			
			CacheAdd(score.m_rank, score);
		}
	}
	
	void View_Scores::CacheAdd(int index, SocialScore & highScore)
	{
		#if RETAIN_SCORES
		g_GameState->m_Social->ScoreAcquire(highScore);
		#endif

		std::map<int, SocialScore>::iterator i = m_Cache.find(index);

		if (i != m_Cache.end())
		{
			#if RETAIN_SCORES
			SocialScore & oldScore = i->second;
			g_GameState->m_Social->ScoreRelease(oldScore);
			oldScore.m_user = NULL;
			#endif

			m_Cache.erase(i);
		}

		LOG_DBG("got score: %d, %s", highScore.m_rank, highScore.m_userName.c_str());

		m_Cache[index] = highScore;
	}
	
	void View_Scores::CacheCompact()
	{
		// todo: check if cache too large or not
		
		// todo: remove items with biggest offset difference
	}
	
	void View_Scores::CacheComplete()
	{
		switch (m_Database)
		{
			case ScoreDatabase_Global:
				CacheComplete_Global();
				break;
			case ScoreDatabase_Local:
				CacheComplete_Local();
				break;
				
			default:
				throw ExceptionVA("not implemented");
		}
	}
	
	void View_Scores::CacheComplete_Global()
	{
		//LOG_DBG("scores: cache complete global", 0);
		
		// todo: check cache for missing score ranges
		
		int index1 = -1;
		int index2 = -1;
		
		int min = CacheMin_get();
		int max = CacheMax_get();
		
		// Find first non cached value
		
		for (int i = min; i <= max && index1 < 0; ++i)
			if (!CacheQuery(i))
				index1 = i;
		
		// Cache already complete?
		
		if (index1 < 0)
			return;
		
		// Not complete. Find the end of the range we need to update
		
		for (int i = index1; i <= max && index2 < 0; ++i)
			if (CacheQuery(i))
				index2 = i;
		
		if (index2 < 0)
			index2 = max;
		
		int count = index2 - index1 + 1;
		
		if (count < 5 && index2 != m_ScoreCount - 1)
			return;
		
		// Update fetching state
		
		for (int i = index1; i <= index2; ++i)
		{
			SocialScore score(0, 0, 0, i, "(waiting..)");
			CacheAdd(i, score);
		}
		
		// Query range
		
		LOG_DBG("scores: cache query %u - %u", index1, index2);
		
#ifdef IPHONEOS
		Difficulty difficulty;
		GrsIdToGame(m_GrsId, difficulty);
		const char * category = GameToGameCenter(difficulty);
		
		g_gameCenter->ScoreList(category, index1, index2, m_History);
#endif

#ifdef BBOS
		Difficulty difficulty;
		GrsIdToGame(m_GrsId, difficulty);
		const int mode = GameToScoreLoop(difficulty);

		g_GameState->m_Social->AsyncScoreListRange(mode, m_History, index1, index2-index1+1);
#endif
	}
	
	void View_Scores::CacheComplete_Local()
	{
#if !defined(BBOS) // todo: remove, fix local
		if (m_LocalLoaded)
			return;
		
		m_LocalLoaded = true;
		
		Score* scores = 0;
		int scoreCount = 0;
		
		m_ScoreBoard.Load(m_GrsId, &scores, scoreCount);
		
		for (int i = 0; i < scoreCount; ++i)
		{
			Score & score = scores[i];
			SocialScore socialScore(0, 0, int(score.score), i, score.name);
			CacheAdd(i, socialScore);
		}
		
		m_ScoreCount = scoreCount;
			 
		delete[] scores;
		scores = 0;
#endif
	}
	
	SocialScore* View_Scores::CacheQuery(int index)
	{
		if (index < 0 || (m_ScoreCount >= 0 && index >= m_ScoreCount))
			return 0;
		
		std::map<int, SocialScore>::iterator ptr = m_Cache.find(index);
		
		if (ptr == m_Cache.end())
			return 0;
		else
			return &ptr->second;
	}
	
	int View_Scores::CacheMin_get() const
	{
		int result = ScrollPosition_get() - m_CacheBorder;
		
		if (result < 0)
			result = 0;
		
		return result;
	}
	
	int View_Scores::CacheMax_get() const
	{
		int result = ScrollPosition_get() + m_ScoreVisibleCount + m_CacheBorder;
		
		if (m_ScoreCount >= 0)
			if (result >= m_ScoreCount)
				result = m_ScoreCount - 1;
		
		return result;
	}
}
