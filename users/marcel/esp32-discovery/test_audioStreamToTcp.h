#pragma once

#include "audiostream/AudioStreamVorbis.h"
#include "audioStreamToTcp.h"
#include <stdint.h>

struct Test_AudioStreamToTcp
{
	AudioStreamToTcp audioStreamToTcp;
	
	AudioStream_Vorbis audioStream;
	
	bool init(
		const uint32_t ipAddress,
		const uint16_t tcpPort,
		const int numBuffers,
		const int numFramesPerBuffer,
		const int numChannelsPerFrame,
		const bool lowQualityMode,
		const char * filename);
	void shut();
	
	void tick();
	
	bool isActive() const;
};
