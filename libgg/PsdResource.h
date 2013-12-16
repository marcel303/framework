#pragma once

#include <string>
#include "libgg_forward.h"
#include "PsdForward.h"
#include "Types.h"

class PsdImageResource
{
public:
	PsdImageResource();
	virtual ~PsdImageResource();
	void Setup(std::string osType, uint16_t id, const std::string& name);
	
	static PsdImageResource* Read(PsdInfo* pi, Stream* stream);
	
	virtual void ReadResource(PsdInfo* pi, Stream* stream) = 0;
	void Write(PsdInfo* pi, Stream* stream);
	virtual void WriteResource(PsdInfo* pi, Stream* stream) = 0;

	char mOsType[4];
	uint16_t mId;
	std::string mName;
};
