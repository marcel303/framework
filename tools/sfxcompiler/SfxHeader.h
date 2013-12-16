#pragma once

#include "Stream.h"

class SfxHeader
{
public:
	uint8_t mChannelCount;
	uint8_t mBitsPerChannel;
	uint32_t mFrameCount;
	uint32_t mFrameRate;
	float mDuration;
	uint32_t mByteCount;
	
	SfxHeader();
	
	void Setup(uint8_t channelCount, uint8_t bitsPerChannel, uint32_t frameCount, uint32_t frameRate);
	
	void Load(Stream* stream);
	void Save(Stream* stream);
};
