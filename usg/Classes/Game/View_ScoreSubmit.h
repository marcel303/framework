#pragma once

#include "FixedSizeString.h"
#include "IView.h"
#include "PolledTimer.h"
#include "ScreenLock.h"
#include "TouchInfo.h"
#include "Types.h"
#include "View_Scores.h"

namespace Game
{	
	class View_ScoreSubmit : public IView
	{
	public:
		View_ScoreSubmit();
		virtual void Initialize();

		// --------------------
		// View related
		// --------------------
		virtual void HandleFocus();
		virtual void HandleFocusLost();

		virtual void Update(float dt);
		virtual void Render();
		virtual void Render_Additive();
		
		virtual float FadeTime_get();
		virtual int RenderMask_get();
		
		void NextView();
		
		bool m_NoKeyBoard;
		
		// --------------------
		// Submission
		// --------------------
		void Database_set(ScoreDatabase database);
#if defined(IPHONEOS)
		static void EH_GameCenterSubmitComplete(void * obj, void * arg);
		static void EH_GameCenterSubmitFailed(void * obj, void * arg);
#endif
#if defined(IPHONEOS) || defined(BBOS) || defined(WIN32) || defined(MACOS)
		void Handle_GameCenterSubmitComplete(int rank);
		void Handle_GameCenterSubmitFailed();
#else
		static void Handle_SubmitResult(void* obj, void* arg);
#endif
		
		ScoreDatabase m_Database;
		bool m_ScoreSubmitted;
		FixedSizeString<256> m_ResultString;
		int m_ResultRank;
		
		// --------------------
		// Drawing
		// --------------------
		void DrawRing();
		void DrawResultString();
		
		// --------------------
		// Animation
		// --------------------
		Vec2F DotPosition_get() const;
		void UpdateAnimation(float dt);
		
		ScreenLock m_ScreenLock;
		float m_DotAngle;
		PolledTimer m_DotGlowTimer;
		AnimTimer m_DotGlowAnim;
		PolledTimer m_SparkleTimer;
		
		// --------------------
		// Helpers
		// --------------------
		void RefreshScoreView(Difficulty difficulty, int position);
	};
}
