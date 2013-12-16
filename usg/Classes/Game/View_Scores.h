#pragma once

#include <map>
#include "AnimTimer.h"
#include "IView.h"
#include "libiphone_forward.h"
#include "ScoreBoard.h"
#include "TouchInfo.h"
#include "Types.h"

namespace Game
{
	// panel for displaying scores from a single catagory, including scrolling, dynamic caching, etc
	
	enum ScoreDatabase
	{
		ScoreDatabase_Global,
		ScoreDatabase_Local
	};
	
	class View_Scores : public IView
	{
	public:
		View_Scores();
		void Initialize();
		
		void GrsClient_set(GRS::HttpClient* client);
		
		void Show(ScoreDatabase database, Difficulty difficulty, int history);
		void ShowGrsId(int grsId);
		void ShowHistory(int history);
		void Refresh();

	private:
		// --------------------
		// View
		// --------------------
		virtual void Update(float dt);
		virtual void Render();
		
		virtual int RenderMask_get();
		
		//
		
		int ScoreCount_get();
	public:
		int BestRank_get();
	private:
		
		// ----------------------------------------
		// Database
		// ----------------------------------------
	public:
		ScoreDatabase DefaultDatabase_get();
		ScoreDatabase Database_get();
		void Database_set(ScoreDatabase database);
		ScoreBoard& ScoreBoard_get();
		
		// ----------------------------------------
		// Querying
		// ----------------------------------------
		void HandleScoreList(SocialScoreList & scores, bool focus);

	private:
		static void HandleQueryListResult(void* obj, void* arg);
		
		// ----------------------------------------
		// Scrolling
		// ----------------------------------------
		int ScrollPosition_get() const;
		float ScrollAnimationOffset_get() const;
		
		void ScrollReset();
		void Scroll(float offset);
		void ScrollAbsolute(float position);
		void ScrollClamp(float dt);
	public:
		void ScrollIntoView(int position);
		void ScrollUserIntoView();
	private:
		
		// --------------------
		// View
		// --------------------
		virtual void HandleFocus();
		virtual void HandleFocusLost();
		
		// --------------------
		// Touch related
		// --------------------
		static bool HandleTouchBegin(void* obj, const TouchInfo& touchInfo);
		static bool HandleTouchMove(void* obj, const TouchInfo& touchInfo);
		static bool HandleTouchEnd(void* obj, const TouchInfo& touchInfo);
		
		// ----------------------------------------
		// Caching
		// ----------------------------------------
		void CacheClear(); // clear cache
		void CacheAdd(SocialScoreList & scores); // add multiple entries to cache
		void CacheAdd(int index, SocialScore & highScore); // add entry to cache
		void CacheCompact(); // remove cache entries which are no longer required
		void CacheComplete(); // check current view and query missing scores
		void CacheComplete_Global();
		void CacheComplete_Local();
		SocialScore* CacheQuery(int index);
		int CacheMin_get() const;
		int CacheMax_get() const;
		
		int m_CacheBorder;

		std::map<int, SocialScore> m_Cache;
		
		RectF m_Rect;
		ScoreDatabase m_Database;
		ScoreBoard m_ScoreBoard;
		bool m_LocalLoaded;
		int m_GrsId;
		int m_History;
		int m_ScoreCount;
		int m_BestRank;
		float m_ScorePosition;
		float m_ScrollSpeed;
		float m_ScrollSpeedTemp;
		int m_ScoreVisibleCount;
		int m_FocusIndex;
		AnimTimer m_ScoreBlinkTimer;
		bool m_ScrollActive;
	};
	
	//
}
