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

#include <string.h>

AudioStreamWave::AudioStreamWave()
	: mFile(0)
	, mNumChannels(0)
	, mSampleRate(0)
	, mDurationInFrames(0)
	, mPositionInFrames(0)
	, mLoop(false)
	, mHasLooped(false)
	, mDataOffset(0)
	, mCompressionType(0)
	, mBitDepth(0)
	, mNumBytesPerSample(0)
{
}

AudioStreamWave::~AudioStreamWave()
{
	Close();
}

int AudioStreamWave::Provide(int numFrames, AudioSample* __restrict buffer)
{
	if (mFile == 0)
		return 0;
	
	int numFramesTodo = numFrames;
	int numFramesRead = 0;
	
	int16_t * __restrict dstValues = (int16_t*)alloca(numFrames * mNumChannels * 2);
	
	for (;;)
	{
		if (numFramesTodo == 0)
			break;
			
		// handle looping
		
		if (mPositionInFrames == mDurationInFrames)
		{
			mPositionInFrames = 0;
			mHasLooped = true;
			
			if (fseek(mFile, mDataOffset, SEEK_SET) != 0)
			{
				Close();
				break;
			}
		}

		// read source values and convert them into int16 destination values
		// we will convert this data to stereo later on
		
		const int numFramesRemaining = mDurationInFrames - mPositionInFrames;
		
		const int numFramesToRead =
			numFramesRemaining > numFramesTodo
				? numFramesTodo
				: numFramesRemaining;
		
		const int numSamplesToRead = numFramesToRead * mNumChannels;
		
		uint8_t * __restrict srcValues = (uint8_t*)alloca(numSamplesToRead * mNumBytesPerSample);
		
		if (fread(srcValues, numSamplesToRead * mNumBytesPerSample, 1, mFile) != 1)
		{
			Close();
			break;
		}
		
		if (mCompressionType == WAVE_FORMAT_PCM)
		{
			if (mBitDepth == 8)
			{
				// for 8 bit data the integers are unsigned. convert them to signed here
				
				for (int i = 0; i < numSamplesToRead; ++i)
				{
					const int value = int(srcValues[i]) - 128;
					
					dstValues[i] = value << 8;
				}
			}
			else if (mBitDepth == 16)
			{
				for (int i = 0; i < numSamplesToRead; ++i)
				{
					const int16_t value =
						(srcValues[i * 2 + 0] << 0) |
						(srcValues[i * 2 + 1] << 8);
					
					dstValues[i] = value;
				}
			}
			else if (mBitDepth == 24)
			{
				for (int i = 0; i < numSamplesToRead; ++i)
				{
					const int32_t value =
						(srcValues[i * 3 + 0] << 8) |
						(srcValues[i * 3 + 1] << 16) |
						(srcValues[i * 3 + 2] << 24);
					
					// perform a shift right to sign-extend the 24 bit number (which we packed into the top-most 24 bits of a 32 bits number)
					dstValues[i] = value >> 8;
				}
			}
			else if (mBitDepth == 32)
			{
				for (int i = 0; i < numSamplesToRead; ++i)
				{
					const int32_t value =
						(srcValues[i * 4 + 0] << 0) |
						(srcValues[i * 4 + 1] << 8) |
						(srcValues[i * 4 + 2] << 16) |
						(srcValues[i * 4 + 3] << 24);
				
					dstValues[i] = value >> 16;
				}
			}
		}
		else if (mCompressionType == WAVE_FORMAT_IEEE_FLOAT)
		{
			if (mBitDepth == 32)
			{
				// perform endian conversion
				
				const uint8_t * src = srcValues;
				float * dst = (float*)srcValues;
				
				for (int i = 0; i < numSamplesToRead; ++i)
				{
					dst[i] =
						(src[i * 4 + 0] << 0) |
						(src[i * 4 + 1] << 8) |
						(src[i * 4 + 2] << 16) |
						(src[i * 4 + 3] << 24);
				}
				
				// perform scaling and conversion to integer
				
				for (int i = 0; i < numSamplesToRead; ++i)
				{
					dstValues[i] = int16_t(dst[i] * ((1 << 15) - 1));
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
		
		AudioSample * __restrict bufferWithOffset = buffer + numFramesRead;
		
		if (mNumChannels == 1)
		{
			for (int i = 0; i < numFramesToRead; ++i)
			{
				bufferWithOffset[i].channel[0] = dstValues[i];
				bufferWithOffset[i].channel[1] = dstValues[i];
			}
		}
		else if (mNumChannels == 2)
		{
			memcpy(bufferWithOffset, dstValues, numFramesToRead * sizeof(AudioSample));
		}
		
		numFramesTodo -= numFramesToRead;
		numFramesRead += numFramesToRead;
		
		mPositionInFrames += numFramesToRead;
	}
	
	return numFramesRead;
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
	
	if (fseek(mFile, headers.dataOffset, SEEK_SET) != 0)
	{
		LOG_ERR("Wave Audio Stream: failed to seek to WAVE data");
		Close();
		return;
	}
	
	mNumChannels = headers.fmtChannelCount;
	mSampleRate = headers.fmtSampleRate;
	mDurationInFrames = headers.dataLength / numBytesPerSample / headers.fmtChannelCount;
	
	mDataOffset = headers.dataOffset;
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
	mPositionInFrames = 0;
}

int AudioStreamWave::Position_get() const
{
	return mPositionInFrames;
}

bool AudioStreamWave::HasLooped_get() const
{
	return mHasLooped;
}
