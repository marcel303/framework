/*
	Copyright (C) 2020 Marcel Smit
	marcel303@gmail.com
	https://www.facebook.com/marcel.smit981

	Permission is hereby granted, free of charge, to any person
	obtaining a copy of this software and associated documentation
	files (the "Software"), to deal in the Software without
	restriction, including without limitation the rights to use,
	copy, modify, merge, publish, distribute, sublicense, and/or
	sell copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following
	conditions:

	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
	OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
	HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
	WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
	OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include "AudioStream.h"
#include <stdio.h> // FILE
#include <string>

class AudioStream_Vorbis : public AudioStream
{
public:
	AudioStream_Vorbis();
	virtual ~AudioStream_Vorbis();
	
	virtual int Provide(int numSamples, AudioSample* __restrict buffer);
	
	void Open(const char* fileName, bool loop);
	void Close();
	
	int32_t SampleRate_get() const { return mSampleRate; }
	int64_t Duration_get() const { return mDuration; }
	
	int64_t Position_get() const { return mPosition; }
	bool HasLooped_get() const { return mHasLooped; }

	bool IsOpen_get() const { return mFile != 0; }
	const char * FileName_get() const { return mFileName.c_str(); }
	bool Loop_get() const { return mLoop; }
	
private:
	std::string mFileName;
	FILE* mFile;
	struct OggVorbis_File* mVorbisFile;
	int16_t mNumChannels;
	int32_t mSampleRate;
	int64_t mDuration;
	int64_t mPosition;
	bool mLoop;
	bool mHasLooped;
};
