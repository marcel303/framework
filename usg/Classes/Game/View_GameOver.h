#pragma once

#include "DeltaTimer.h"
#include "EventHandler.h"
#include "IView.h"
#include "TriggerTimerEx.h"

namespace Game
{
	class View_GameOver : public IView, public EventHandler
	{
	public:
		View_GameOver();
		virtual ~View_GameOver();
		void Initialize();
		void Shutdown();

	private:
		// --------------------
		// View
		// --------------------
		virtual void Update(float dt);
		virtual void Render();
		
		virtual int RenderMask_get();
		
		virtual void HandleFocus();
		virtual void HandleFocusLost();
		
		// --------------------
		// Touch related
		// --------------------
		static bool HandleTouchBegin(void* obj, const TouchInfo& touchInfo);
		static bool HandleTouchMove(void* obj, const TouchInfo& touchInfo);
		static bool HandleTouchEnd(void* obj, const TouchInfo& touchInfo);

		// --------------------
		// EventHandler
		// --------------------
		virtual bool OnEvent(Event& event);
		
		// continue text
		AnimTimer m_TouchTextTimer;
		bool m_TouchEnabled;
		
		// explosions
		PolledTimer m_RippleTimer;
		PolledTimer m_ExplosionTimer;
		TriggerTimerG m_TouchTrigger;
		
		// hint
		TriggerTimerG m_HintTrigger;
		enum HintState
		{
			HintState_Disabled,
			HintState_AnimIn,
			HintState_Show,
			HintState_AnimOut
		};
		HintState m_HintState;
		const char* m_Hint;
	};
}
