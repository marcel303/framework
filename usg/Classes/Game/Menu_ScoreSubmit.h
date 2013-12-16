#pragma once

#include "Forward.h"
#include "Menu.h"
#include "TriggerTimerEx.h"

namespace GameMenu
{
	class Menu_ScoreSubmit : public Menu
	{
	public:
		virtual void Init();
		virtual void HandleFocus();
		virtual void Update(float dt);
		
		void Handle_SubmitResult();
		
		static void Handle_Continue(void* obj, void* arg);
		
		TriggerTimerG mSkipTrigger;
	};
}
