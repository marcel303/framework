#pragma once

#include "framework.h"

struct LiveUiColors
{
	static Color makeColor(const int in_r, const int in_g, const int in_b)
	{
		return Color(in_r / 255.f, in_g / 255.f, in_b / 255.f);
	}
	
#if 1
	Color text = makeColor(40, 40, 40);
	
	Color gaugeFilled = makeColor(200, 100, 100);
	Color gaugeEmpty = makeColor(100, 100, 200);
	
	Color elemBackgroundActive = makeColor(190, 190, 190);
	Color elemBackgroundHover = makeColor(210, 210, 210);
	Color elemBackground = makeColor(200, 200, 200);
	
	Color separator = makeColor(200, 200, 200);
	
	Color tooltipBackground = makeColor(200, 200, 100);
	Color tooltipText = makeColor(20, 20, 20);
#else
	Color text = makeColor(200, 200, 200);
	
	Color gaugeFilled = makeColor(200, 0, 0);
	Color gaugeEmpty = makeColor(100, 100, 100);
	
	Color elemBackgroundActive = makeColor(40, 40, 40);
	Color elemBackgroundHover = makeColor(50, 50, 50);
	Color elemBackground = makeColor(60, 60, 60);
	
	Color separator = makeColor(20, 20, 20);
	
	Color tooltipBackground = makeColor(0, 0, 40);
	Color tooltipText = makeColor(200, 200, 200);
#endif
};
