#pragma once

#include "IView.h"
#include "ScreenLock.h"

namespace Game
{
	class View_Pause : public IView
	{
	public:
		
	private:
		// --------------------
		// View
		// --------------------
		virtual void Initialize();
		virtual void HandleFocus();
		virtual void HandleFocusLost();
		virtual void Render();
		virtual int RenderMask_get();
		virtual float FadeTime_get();
		
		// --------------------
		// Animation
		// --------------------
		ScreenLock m_ScreenLock;
	};
}
