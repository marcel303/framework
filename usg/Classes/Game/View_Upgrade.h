#pragma once

#include "IView.h"
#include "ScreenLock.h"

namespace Game
{
	class View_Upgrade : public IView
	{
	public:
		View_Upgrade();
		void Initialize();
		
	private:
		// --------------------
		// View related
		// --------------------
		
		virtual void Update(float dt);
		virtual void Render();
		
		virtual int RenderMask_get();
		
		virtual void HandleFocus();
		virtual void HandleFocusLost();
		
		virtual float FadeTime_get();
		
		bool mIsActive;
		
		// --------------------
		// Animation
		// --------------------		
		ScreenLock mScreenLock;
	};
}
