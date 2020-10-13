#pragma once

#include "threadedTcpClient.h"
#include <atomic>
#include <stdint.h>

#include "audiostream/AudioStreamVorbis.h" // todo : move to cpp

struct Test_TcpToI2SQuad
{
	ThreadedTcpConnection tcpConnection;
	
	std::atomic<float> volume;
	
	AudioStream_Vorbis audioStream;
	
	bool init(const uint32_t ipAddress, const uint16_t tcpPort, const char * filename);
	void shut();
};
