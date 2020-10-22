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

#include "AudioStreamWave.h"

#include "WaveLoader.h"

#include "Debugging.h"
#include "Log.h"

AudioStreamWave::AudioStreamWave()
	: mFile(0)
	, mNumChannels(0)
	, mSampleRate(0)
	, mDuration(0)
	, mPosition(0)
	, mLoop(false)
	, mHasLooped(false)
{
}

AudioStreamWave::~AudioStreamWave()
{
	Close();
}

int AudioStreamWave::Provide(int numSamples, AudioSample* __restrict buffer)
{
	if (mFile == 0)
		return 0;
	
	const int numReadSamples = 0;
	
	mPosition += numReadSamples;
	
	return numReadSamples;
}

void AudioStreamWave::Open(const char* fileName, bool loop)
{
	Close();

	mFileName = fileName;
	mLoop = loop;
	
	mFile = fopen(mFileName.c_str(), "rb");
	
	if (mFile == 0)
	{
		LOG_ERR("Wave Audio Stream: failed to open file (%s)", fileName);
		Close();
		return;
	}
	
	LOG_DBG("Wave Audio Stream: opened file: %s", mFileName.c_str());

	// read WAVE headers
	
	WaveHeadersReader headers;
	
	if (!headers.read(mFile))
	{
		LOG_ERR("Wave Audio Stream: failed to read WAVE headers");
		Close();
		return;
	}
	
	mNumChannels = headers.fmtChannelCount;
	mSampleRate = headers.fmtSampleRate;
	mDuration = headers.dataLength; // todo : divide by sample size and channel count
	
	LOG_DBG("Wave Audio Stream: channelCount=%d, sampleRate=%d", mNumChannels, mSampleRate);
}

void AudioStreamWave::Close()
{
	if (mFile != 0)
	{
		fclose(mFile);
		mFile = 0;
		LOG_DBG("Wave Audio Stream: closed file");
	}
	
	mFileName.clear();
	mPosition = 0;
}

int AudioStreamWave::Position_get() const
{
	return mPosition;
}

bool AudioStreamWave::HasLooped_get() const
{
	return mHasLooped;
}
