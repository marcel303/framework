#pragma once

#include <vector>
#include "libgg_forward.h"
#include "PsdForward.h"

class PsdImageResourceList
{
public:
	~PsdImageResourceList();
	
	void Read(PsdInfo* pi, Stream* stream);
	void Write(PsdInfo* pi, Stream* stream);

	std::vector<PsdImageResource*> mResourceList;
};
