#pragma once

#include "PsdForward.h"
#include "PsdResource.h"
#include "PsdTypes.h"

class PsdDisplayInfo : public PsdImageResource
{
public:
	PsdDisplayInfo();
	virtual ~PsdDisplayInfo();
	void Setup(const std::string& name, PsdColorSpace colorSpace, int r, int g, int b, int a, int opacity, bool kind);
	
	virtual void ReadResource(PsdInfo* pi, Stream* stream);
	virtual void WriteResource(PsdInfo* pi, Stream* stream);

	PsdColorSpace mColorSpace;
	uint16_t mColor[4];
	uint16_t mOpacity;
	bool mKind;
};
