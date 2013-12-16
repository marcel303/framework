#pragma once

#include "GuiPoint.h"

namespace Gui
{
	class Rect
	{
	public:
		inline Rect();
		inline Rect(int left, int top, int right, int bottom);
		inline Rect(const Point& min, const Point& max);

		inline bool Clip(const Rect& rect, Rect& out_rect);
		inline bool Merge(const Rect& rect, Rect& out_rect);
		inline bool Inside(const Point& point);

		Point min;
		Point max;
	};
};

#include "GuiRect.inl"
