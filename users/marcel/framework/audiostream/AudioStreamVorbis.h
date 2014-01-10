#pragma once

#include <string>
#include "AudioMixer.h"

class AudioStream_Vorbis : public AudioStream
{
public:
	AudioStream_Vorbis();
	virtual ~AudioStream_Vorbis();
	
	virtual int Provide(int numSamples, AudioSample* __restrict buffer);
	
	void Open(const char* fileName, bool loop);
	void Close();
	int Position_get();
	bool HasLooped_get();
	
	int mSampleRate;
	
private:
	std::string mFileName;
	FILE* mFile;
	struct OggVorbis_File* mVorbisFile;
	int mNumChannels;
	int mPosition;
	bool mLoop;
	bool mHasLooped;
};
