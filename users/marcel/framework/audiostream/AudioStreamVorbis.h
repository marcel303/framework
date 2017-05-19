#pragma once

// Copyright (C) 2013 Grannies Games - All rights reserved

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

	bool IsOpen_get() const { return mFile != 0; }
	const char * FileName_get() const { return mFileName.c_str(); }
	bool Loop_get() const { return mLoop; }
	
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
