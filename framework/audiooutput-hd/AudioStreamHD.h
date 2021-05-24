#pragma once

class AudioStreamHD
{
public:
	struct StreamInfo
	{
		float  frameRate;        // number of (single- or mult-channel) frames that are output per second
		double secondsSincePlay; // the number of seconds since Play() was called
		int    framesSincePlay;  // the number of frames since Play() was called
		float  outputLatency;    // audio output latency in seconds. use this for precision timing when for instance scheduling notes
	};
	
	struct ProvideInfo
	{
		const float ** inputSamples;
		int numInputChannels;
		float ** outputSamples;
		int numOutputChannels;
		int numFrames;
	};
	
	virtual ~AudioStreamHD() { }
	
	virtual int Provide(
		const ProvideInfo & provideInfo,
		const StreamInfo & streamInfo) = 0;
};
