#pragma once

#include "vfxNodes/vfxNodeBase.h"

// would be nice to have a mask input which selects which sequence elements are active ..

struct VfxNodeSequence : VfxNodeBase
{
	enum Input
	{
		kInput_1,
		kInput_2,
		kInput_3,
		kInput_4,
		kInput_5,
		kInput_6,
		kInput_7,
		kInput_8,
		kInput_COUNT
	};

	enum Output
	{
		kOutput_Any,
		kOutput_COUNT
	};

	int anyOutput;
	
	VfxNodeSequence();
};
