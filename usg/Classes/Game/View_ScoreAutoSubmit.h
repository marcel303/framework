#pragma once
#include "AnimTimer.h"
#include "IView.h"
#include "TriggerTimer.h"

namespace Game
{
	class View_ScoreAutoSubmit : public IView
	{
	public:
		View_ScoreAutoSubmit();
		virtual ~View_ScoreAutoSubmit();
		
		virtual void Update(float dt);
		virtual void Render();
		
		virtual void HandleFocus();
		virtual int RenderMask_get();
		virtual float FadeTime_get();
		
		static void Handle_ScoreSubmitComplete(void * obj, void * arg);
		static void Handle_ScoreSubmitFailed(void * obj, void * arg);
		static void Handle_Dismiss(void * obj, void * arg);
		
	private:
		enum State
		{
			State_Submit,
			State_SubmitComplete,
			State_FadeOut,
			State_Done
		};
		
		AnimTimer m_fadeInTimer;
		TriggerTimer m_skipTimer;
		AnimTimer m_fadeOutTimer;
		State m_state;
	};
}
