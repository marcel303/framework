#pragma once

#include "IView.h"

namespace Game
{
	class View_WinSetup : public IView
	{
	public:
		virtual void HandleFocus();
		virtual void HandleFocusLost();
		virtual void Render();
		virtual int RenderMask_get();
	};
}
