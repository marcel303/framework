#pragma once

#include "libgg_forward.h"
#include "PsdHeaderInfo.h"
#include "PsdColorModeData.h"
#include "PsdResourceList.h"
#include "PsdLayerAndMaskInfo.h"
#include "PsdImageData.h"

class PsdInfo
{
public:
	void Read(Stream* stream);
	void Write(Stream* stream);
	
	PsdHeaderInfo mHeaderInfo;
	PsdColorModeData mColorModeData;
	PsdImageResourceList mImageResourceList;
	PsdLayerAndMaskInfo mLayerAndMaskInfo;
	PsdImageData mImageData;
};
