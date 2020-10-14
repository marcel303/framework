#pragma once

#include "audiostream/AudioStreamVorbis.h"
#include "audioStreamToTcp.h"
#include <stdint.h>

struct Test_TcpToI2SQuad
{
	AudioStreamToTcp audioStreamToTcp;
	
	AudioStream_Vorbis audioStream;
	
	bool init(const uint32_t ipAddress, const uint16_t tcpPort, const char * filename);
	void shut();
	
	void tick();
	
	bool isActive() const;
};
