#pragma once

#include "PsdResource.h"
#include "PsdTypes.h"

class PsdResolutionInfo : public PsdImageResource
{
public:
	PsdResolutionInfo();
	virtual ~PsdResolutionInfo();
	void Setup(const std::string& name, int xppi, int xppiUnit, PsdResolutionUnit sxUnit, int yppi, int yppiUnit, PsdResolutionUnit syUnit);
	
	virtual void ReadResource(PsdInfo* pi, Stream* stream);
	virtual void WriteResource(PsdInfo* pi, Stream* stream);

	uint16_t mXppi;
	uint32_t mXppiUnit; // 1=pixels per inch, 2=pixels per cm
	uint16_t mSxUnit;
	uint16_t mYppi;
	uint32_t mYppiUnit;
	uint16_t mSyUnit;
};
