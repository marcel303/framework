#pragma once

#include <string>
#include "libgg_forward.h"
#include "PsdForward.h"
#include "PsdTypes.h"
#include "Types.h"

class PsdHeaderInfo
{
public:
	void Read(PsdInfo* pi, Stream* stream);
	void Write(PsdInfo* pi, Stream* stream);
	void Setup(const std::string& signature, int version, int channelCount, int sx, int sy, int bpp, PsdImageMode mode);

	char mSignature[4]; // 8BPS
	uint16_t mVersion;
	uint16_t mChannelCount;
	uint32_t mSy;
	uint32_t mSx;
	uint16_t mBpp;
	PsdImageMode mMode;
};
