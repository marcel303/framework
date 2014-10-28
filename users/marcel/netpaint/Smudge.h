#ifndef SMUDGE_H
#define SMUDGE_H

#include "Canvas.h"

class smudge_desc_t
{
public:
	smudge_desc_t()
	{
		//size = 40;
		//strength = 4.0f;
		//strength = 0.5f;
		//strength = 0.95f;
		strength = 1.0f;
	}

	bool operator==(const smudge_desc_t& desc)
	{
		return
			/*size == desc.size &&*/
			strength == desc.strength;
	}

	bool operator!=(smudge_desc_t& desc)
	{
		return !((*this) == desc);
	}

	//int32 size;
	float strength;
};

class Smudge// : public Canvas
{
public:
	void Create(/*int size, */float strength)
	{
		//Canvas::Create(size, size, 1);

		//desc.size = size;
		desc.strength = strength;

		// TODO: Fill canvas with smudge constants.
	}

	void Apply(Canvas* canvas)
	{
	}

	smudge_desc_t desc;
};

#endif
