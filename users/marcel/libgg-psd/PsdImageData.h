#pragma once

#include "ImageData.h"
#include "libgg_forward.h"
#include "PsdForward.h"
#include "PsdTypes.h"
#include "Types.h"

class PsdImageData
{
public:
	PsdImageData();
	void Setup(Image* image, PsdCompressionType compression);
	
	void Read(PsdInfo* pi, Stream* stream);
	void Write(PsdInfo* pi, Stream* stream);

	Image mImage;
	PsdCompressionType mCompression;
};
