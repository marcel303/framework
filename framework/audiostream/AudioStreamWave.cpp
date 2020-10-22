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

	// read source values and convert them into int16 destination values
	// we will convert this data to stereo later on

// todo : don't read past the end of the file
// todo : check fread return codes
// todo : implement looping

	int16_t * dstValues = (int16_t*)alloca(numSamples * mNumChannels * 2);
	
	if (mCompressionType == WAVE_FORMAT_PCM)
	{
		if (mBitDepth == 8)
		{
			// for 8 bit data the integers are unsigned. convert them to signed here
			
			uint8_t * srcValues = (uint8_t*)alloca(numSamples * mNumChannels);
			fread(srcValues, numSamples * mNumChannels, 1, mFile);
			
			for (int i = 0; i < numSamples; ++i)
			{
				const int value = int(srcValues[i]) - 128;
				
				dstValues[i] = value << 8;
			}
		}
		else if (mBitDepth == 16)
		{
			// 16 bit data is already signed. no conversion needed
			fread(dstValues, numSamples * mNumChannels, 1, mFile);
		}
		else if (mBitDepth == 24)
		{
			uint8_t * srcValues = (uint8_t*)alloca(numSamples * mNumChannels * 3);
			fread(srcValues, numSamples * mNumChannels * 3, 1, mFile);
			
			for (int i = 0; i < numSamples; ++i)
			{
				int32_t value =
					(srcValues[i * 3 + 0] << 8) |
					(srcValues[i * 3 + 1] << 16) |
					(srcValues[i * 3 + 2] << 24);
				
				// perform a shift right to sign-extend the 24 bit number (which we packed into the top-most 24 bits of a 32 bits number)
				value >>= 8;
				
				dstValues[i] = value;
			}
		}
		else if (mBitDepth == 32)
		{
			int32_t * srcValues = (int32_t*)alloca(numSamples * mNumChannels * 4);
			fread(srcValues, numSamples * mNumChannels * 4, 1, mFile);
			
			for (int i = 0; i < numSamples; ++i)
			{
			// todo : pack so we get endianness right. see 24 bits case
			
				dstValues[i] = srcValues[i] >> 16;
			}
		}
	}
	else if (mCompressionType == WAVE_FORMAT_IEEE_FLOAT)
	{
		if (mBitDepth == 32)
		{
			float * srcValues = (float*)alloca(numSamples * mNumChannels * 4);
			fread(srcValues, numSamples * mNumChannels * 4, 1, mFile);
		
		// todo : shuffle so we get endianness right. see 24 bits case
			for (int i = 0; i < numSamples; ++i)
			{
				dstValues[i] = int16_t(srcValues[i] * ((1 << 15) - 1));
			}
		}
		else
		{
			Assert(false);
		}
	}
	else
	{
		Assert(false);
	}
	
	for (int i = 0; i < numSamples; ++i)
	{
		if (mNumChannels == 1)
		{
			buffer[i].channel[0] = dstValues[i];
			buffer[i].channel[1] = dstValues[i];
		}
		else if (mNumChannels == 2)
		{
		// todo : memcpy
			buffer[i].channel[0] = dstValues[i * 2 + 0];
			buffer[i].channel[1] = dstValues[i * 2 + 1];
		}
	}
	
	const int numReadSamples = numSamples;
	
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
	
	if (headers.hasFmt == false)
	{
		LOG_ERR("Wave Audio Stream: missing FMT chunk. cannot load WAVE data when we don't know the format yet");
		Close();
		return;
	}

	if (headers.hasData == false)
	{
		LOG_ERR("Wave Audio Stream: missing DATA chunk. cannot load WAVE data when we don't know the format yet");
		Close();
		return;
	}
	
	if (headers.fmtChannelCount != 1 && headers.fmtChannelCount != 2)
	{
		LOG_ERR("Wave Audio Stream: channel count not supported. must be 1 or 2 but is: %d", headers.fmtChannelCount);
		Close();
		return;
	}
	
	int numBytesPerSample = 0;
	
	if (headers.fmtCompressionType == WAVE_FORMAT_PCM)
	{
		if (headers.fmtBitDepth == 8)
			numBytesPerSample = 1;
		else if (headers.fmtBitDepth == 16)
			numBytesPerSample = 2;
		else if (headers.fmtBitDepth == 24)
			numBytesPerSample = 3;
		else if (headers.fmtBitDepth == 32)
			numBytesPerSample = 4;
	}
	else if (headers.fmtCompressionType == WAVE_FORMAT_IEEE_FLOAT)
	{
		if (headers.fmtBitDepth == 32)
			numBytesPerSample = 4;
	}

	if (numBytesPerSample == 0)
	{
		LOG_ERR("Wave Audio Stream: unknown sample format (c:%d, b:%d)",
			headers.fmtCompressionType,
			headers.fmtBitDepth);
		Close();
		return;
	}
	
// todo : do this same seek when looping
	if (fseek(mFile, headers.dataOffset, SEEK_SET) != 0)
	{
		LOG_ERR("Wave Audio Stream: failed to seek to WAVE data");
		Close();
		return;
	}
	
	mNumChannels = headers.fmtChannelCount;
	mSampleRate = headers.fmtSampleRate;
	mDuration = headers.dataLength / numBytesPerSample / headers.fmtChannelCount;
	mNumBytesPerSample = numBytesPerSample;
	mCompressionType = headers.fmtCompressionType;
	mBitDepth = headers.fmtBitDepth;
	
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
