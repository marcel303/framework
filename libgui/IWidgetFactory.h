#pragma once

#include "libgui_forward.h"

namespace Gui
{
	class IWidgetFactory
	{
	public:
		virtual Widget* Create(const char* name) = 0;
	};
}
