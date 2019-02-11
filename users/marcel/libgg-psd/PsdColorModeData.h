#pragma once

#include "libgg_forward.h"
#include "PsdForward.h"
#include "Types.h"

class PsdColorModeData
{
public:
	void Read(PsdInfo* pi, Stream* stream);
	void Write(PsdInfo* pi, Stream* stream);
};
