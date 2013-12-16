#pragma once

#include "GuiObject.h"

namespace Gui
{
	namespace Util
	{
		namespace Actions
		{
			void Submit(Object* me, Object* sender);
			void Request(Object* me, Object* sender);
			void Hide(Object* me, Object* sender);
			void Show(Object* me, Object* sender);
			void Destroy(Object* me, Object* sender);
		};
	};
};
