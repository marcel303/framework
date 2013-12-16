#pragma once

#include "AnimTimer.h"
#include "IView.h"

namespace Game
{
	class View_ScoreEntry : public IView
	{
	public:
		View_ScoreEntry();
		virtual void Initialize();
		
		virtual void Update(float dt);
		virtual void Render();
		
		virtual void HandleFocus();
		
		virtual int RenderMask_get();
		
	private:
		AnimTimer m_BackAnim; // fade background to black
	};
}
