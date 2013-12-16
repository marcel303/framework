#pragma once

#include "IView.h"
#include "PolledTimer.h"

namespace Game
{
	class View_Main : public IView
	{
	public:
		View_Main();
		
		virtual void Initialize();
		
		static void RenderLogo();
	private:
		// --------------------
		// Entities
		// --------------------
		PolledTimer m_SpawnTimer;
		
		// --------------------
		// View
		// --------------------
		virtual void Update(float dt);
		virtual void Render();
		
		virtual int RenderMask_get();
		
		virtual void HandleFocus();
		virtual void HandleFocusLost();
	};
}
