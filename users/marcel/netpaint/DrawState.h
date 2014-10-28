#pragma once

#include "Brush.h"
#include "Canvas.h"
#include "Coord.h"
#include "Smudge.h"

class blur_desc_t
{
public:
	blur_desc_t()
	{
		size = 0;
	}

	bool operator==(const blur_desc_t& desc)
	{
		return
			size == desc.size;
	}

	bool operator!=(blur_desc_t& desc)
	{
		return !((*this) == desc);
	}

	int32_t size;
};

class CL_DrawState
{
public:
	CL_DrawState()
	{
		coord.p[0] = 0.0f;
		coord.p[1] = 0.0f;

		color[0] = 0.1f;
		color[1] = 0.3f;
		color[2] = 0.5f;

		blend_mode = BM_REPLACE;

		//opacity = 0.1f;
		//opacity = 1.0f;
		opacity = 0.5f;
	}

	brush_desc_t brush_desc;
	blur_desc_t blur_desc;
	smudge_desc_t smudge_desc;

	coord_t coord;
	float color[3];
	//BLEND_MODE blend_mode;
	int32_t blend_mode;
	float opacity;
	coord_t direction;
};
