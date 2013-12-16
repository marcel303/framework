#pragma once

#include <algorithm>
#include "GuiRect.h"

namespace Gui
{
	inline Rect::Rect()
	{
		min.x = 0;
		min.y = 0;
		max.x = 0;
		max.y = 0;
	}

	inline Rect::Rect(int left, int top, int right, int bottom)
	{
		this->min.x = left;
		this->min.y = top;
		this->max.x = right;
		this->max.y = bottom;
	}

	inline Rect::Rect(const Point& min, const Point& max)
	{
		this->min = min;
		this->max = max;
	}

	inline bool Rect::Clip(const Rect& rect, Rect& out_rect)
	{
		for (int i = 0; i < 2; ++i)
		{
			if (rect.min.values[i] > max.values[i])
				return false;
			if (rect.max.values[i] < min.values[i])
				return false;
			out_rect.min.values[i] = std::max<int>(min.values[i], rect.min.values[i]);
			out_rect.max.values[i] = std::min<int>(max.values[i], rect.max.values[i]);
		}
		if (out_rect.max.values[0] - out_rect.min.values[0] > 0 &&
			out_rect.max.values[1] - out_rect.min.values[1] > 0)
			return true;
		else
			return false;
	}

	inline bool Rect::Merge(const Rect& rect, Rect& out_rect)
	{
		for (int i = 0; i < 2; ++i)
		{
			out_rect.min.values[i] = std::min<int>(min.values[i], rect.min.values[i]);
			out_rect.max.values[i] = std::max<int>(max.values[i], rect.max.values[i]);
		}
		return true;
	}

	inline bool Rect::Inside(const Point& point)
	{
		return
			point.x >= min.x &&
			point.y >= min.y &&
			point.x <= max.x &&
			point.y <= max.y;
	}
};
