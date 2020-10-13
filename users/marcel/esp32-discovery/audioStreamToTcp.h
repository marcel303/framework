#pragma once

#include "threadedTcpClient.h"
#include <atomic>
#include <functional>
#include <stdint.h>

#include "audiostream/AudioStreamVorbis.h" // todo : move to cpp

struct AudioStreamToTcp
{
	/**
	 * Function invoked to whenever new sample data needs to be streamed to the endpoint.
	 * @param samples The array of sample values that needs to be filled in. The total number of samples equals @numSamples * @numChannels. Channel values are stored interleaved inside this array.
	 * @param numFrames The number of frames to provide. A single frame consists of @numChannels values of sample data.
	 * @param numChannels The number of channels per frame.
	 */
	using ProvideFunction = std::function<void(
		void * __restrict samples,
		const int numFrames,
		const int numChannels)>;
	
	enum SampleFormat
	{
		kSampleFormat_S16,
		kSampleFormat_Float
	};
	
	ThreadedTcpConnection tcpConnection;
	
	std::atomic<float> volume;
	
	int numBuffers = 0;
	int numFramesPerBuffer = 0;
	int numChannelsPerFrame = 0;
	SampleFormat sampleFormat = kSampleFormat_Float;
	ProvideFunction provideFunction;
	
	AudioStreamToTcp();
	
	bool init(
		const uint32_t ipAddress,
		const uint16_t tcpPort,
		const int numBuffers,
		const int numFramesPerBuffer,
		const int numChannelsPerFrame,
		const SampleFormat sampleFormat,
		const ProvideFunction & provideFunction);
	void beginShutdown();
	void waitForShutdown();
};
