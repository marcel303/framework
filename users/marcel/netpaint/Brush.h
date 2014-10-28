#pragma once

#include "canvas.h"
#include "sample.h"
#include "Types.h"

class brush_desc_t
{
public:
	brush_desc_t()
	{
		size = 0;
		//hardness = 0.0f;
		hardness = 0.5f;
		cos_freq[0] = 0.0f;
		cos_freq[1] = 0.0f;
	}

	bool operator==(const brush_desc_t& desc)
	{
		return
			size == desc.size &&
			hardness == desc.hardness &&
			cos_freq[0] == desc.cos_freq[0] &&
			cos_freq[1] == desc.cos_freq[1];
	}

	bool operator!=(brush_desc_t& desc)
	{
		return !((*this) == desc);
	}

	int32_t size;
	float hardness;
	float cos_freq[2];
};

class Brush : public Canvas
{
public:
	Brush() : Canvas()
	{
	}

	void Create(int size)
	{
		Canvas::Create(size, size, 1);

		desc.size = size;
	}

	brush_desc_t desc;
};
