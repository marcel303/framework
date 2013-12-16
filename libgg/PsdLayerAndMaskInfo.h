#pragma once

#include <vector>
#include "libgg_forward.h"
#include "PsdForward.h"
#include "Types.h"

class PsdLayerAndMaskInfo
{
public:
	PsdLayerAndMaskInfo();
	~PsdLayerAndMaskInfo();
	void Setup(bool skipAlpha);
	
	void Read(PsdInfo* pi, Stream* stream);
	void WritePrepare();
	void Write(PsdInfo* pi, Stream* stream);

	int16_t mLayerCount;
	bool mSkipAlpha;
	std::vector<PsdLayer*> mLayerList;
	bool mIsWritePrepared;
};
