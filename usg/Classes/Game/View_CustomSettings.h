#pragma once

#include "IView.h"
#include "ScreenLock.h"

namespace Game
{
	class View_CustomSettings : public IView
	{
	public:
		View_CustomSettings();
		void Initialize();
		virtual ~View_CustomSettings();

		void Show(View nextView);
		
	private:
		// --------------------
		// View
		// --------------------
		virtual void Update(float dt);
		virtual void Render();
		
		virtual int RenderMask_get();
		
		virtual void HandleFocus();
		virtual void HandleFocusLost();
		
		virtual float FadeTime_get();
		
		ScreenLock m_ScreenLock;
		bool m_IsActive;
		

	public:
		View m_NextView;

	};
}
