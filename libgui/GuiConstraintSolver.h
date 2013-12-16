#pragma once

#include "GuiPoint.h"
#include "libgui_forward.h"

namespace Gui
{
	class ConstraintSolver
	{
	public:
		static ConstraintSolver& I();

		Point GetConstrainedSize(const Widget* widget, const Point& size);
		void Align(Widget* container);

	private:
		ConstraintSolver();
	};
};
