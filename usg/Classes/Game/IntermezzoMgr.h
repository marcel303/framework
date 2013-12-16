#pragma once

#include "AnimSeq.h"
#include "AnimTimer.h"
#include "Log.h"
#include "PolledTimer.h"

// intermezzo's are short interruptions between or after game play
// the intermezzo manager controls the world / camera's actions while active
// note: the intermezzo mgr is only a clean way to remove all animation during game state switches from world and store it separately

namespace Game
{
	enum IntermezzoType
	{
		IntermezzoType_LevelBegin,
		IntermezzoType_WaveBanner,
		IntermezzoType_MiniBossAlert,
		IntermezzoType_KillStreak
	};
	
	class IntermezzoMgr
	{
	public:
		IntermezzoMgr();
		void Initialize();
		
		void Start(IntermezzoType type);
		void Update(float dt);
		void Render_Overlay();
		
//		bool IsActive_get() const;
//		bool TakeOver_get() const;
		
	private:
		// --------------------
		// Wave banner
		// --------------------
		
		class WaveBanner
		{
		public:
			WaveBanner();
			void Start();
			void Update(float dt);
			void Render();

			void Render(float x, float y, float a);
			
			enum State
			{
				State_Idle,
				State_FadeIn,
				State_Wait,
				State_FadeOut
			};
			
			State m_State;
			PolledTimer m_Timer;
			AnimTimer m_AnimTimer;
		};
		
		WaveBanner m_WaveBanner;
		
		// --------------------
		// Mini boss alert
		// --------------------
		
		// --------------------
		// Kill streak
		// --------------------
				
		// --------------------
		// Shared
		// --------------------
		
		PolledTimer m_Timer;
		AnimTimer m_AnimTimer;
		PolledTimer m_Timer2;
//		LogCtx m_Log;
	};
}
