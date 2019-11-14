#pragma once

#include "framework.h"

struct OpticalFlow
{
	Surface luminance[2];
	int current_luminance = 0;
	
	Surface sobel[2];
	int current_sobel = 0;
	
	Surface opticalFlow;
	
	struct
	{
		float blurRadius = 22.f;
	} sourceFilter;
	
	void init(const int sx, const int sy);
	void shut();
	
	void update(const GxTextureId source);
};
