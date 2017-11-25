/*
	Copyright (C) 2017 Marcel Smit
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

#include <stdio.h>
#include "AudioStreamVorbis.h"
#include "internal.h"
#include "oggvorbis.h"

static void DuplicateInPlace(short * buffer, int numSamples);

AudioStream_Vorbis::AudioStream_Vorbis()
	: mSampleRate(0)
	, mFile(0)
	, mVorbisFile(0)
	, mNumChannels(0)	
	, mPosition(0)
	, mLoop(false)
	, mHasLooped(false)
{
	mVorbisFile = new OggVorbis_File();
}

AudioStream_Vorbis::~AudioStream_Vorbis()
{
	Close();
	
	delete mVorbisFile;
	mVorbisFile = 0;
}

int AudioStream_Vorbis::Provide(int numSamples, AudioSample* __restrict buffer)
{
	fassert(mFile != 0);
	
	if (mFile == 0)
		return 0;
	
	int bytesRemain = numSamples * sizeof(AudioSample);
	int bytesRead = 0;
	
	char* bytes = (char*)buffer;
	
	mHasLooped = false;
	
	while (bytesRemain != 0)
	{
		const int bytesToRead = bytesRemain * mNumChannels / 2;
		
		int bitstream = -1;
		
		int currentBytesRead = (int)ov_read(
			mVorbisFile,
			bytes + bytesRead,
			bytesToRead,
			0,
			2,
			1,
			&bitstream);	
		
		if (mNumChannels == 1)
		{
			DuplicateInPlace((short*)(bytes + bytesRead), currentBytesRead / sizeof(short));
			
			currentBytesRead *= 2;
		}
		
		if (currentBytesRead == 0)
		{
			// reached EOF
			
			if (mLoop)
			{
				// not done yet!
				
				Close();
				
				Open(mFileName.c_str(), mLoop);
				
				mHasLooped = true;
			}
			else
			{
				// we're done
				
				break;
			}
		}
		else
		{
			fassert(currentBytesRead <= bytesRemain);
			
			bytesRemain -= currentBytesRead;
			bytesRead += currentBytesRead;
		}
		
	}
	
	fassert((bytesRead % sizeof(AudioSample)) == 0);
	
	int numReadSamples = bytesRead / sizeof(AudioSample);
	
	mPosition += numReadSamples;
	
	return numReadSamples;
}

void AudioStream_Vorbis::Open(const char* fileName, bool loop)
{
	Close();

	mFileName = fileName;
	mLoop = loop;
	
	fopen_s(&mFile, mFileName.c_str(), "rb");
	
	if (mFile == 0)
	{
		logError("Vosbis Audio Stream: failed to open file (%s)", fileName);
		fassert(mFile != 0);
		Close();
		return;
	}
	
	logDebug("Vorbis Audio Stream: opened file: %s", mFileName.c_str());
	
	int result = ov_open(mFile, mVorbisFile, NULL, 0);
	if (result != 0)
	{
		logError("Vosbis Audio Stream: failed to create vorbis decoder (%d)", result);
		Close();
		return;
	}
	
	logDebug("Vorbis Audio Stream: created vorbis decoder", 0);
	
	vorbis_info* info = ov_info(mVorbisFile, -1);
	
	mSampleRate = static_cast<int>(info->rate);
	mNumChannels = info->channels;
	
	logDebug("Vorbis Audio Stream: channelCount=%d, sampleRate=%d", mNumChannels, mSampleRate);
}

void AudioStream_Vorbis::Close()
{
	if (mFile != 0)
	{
		ov_clear(mVorbisFile);
		logDebug("Vorbis Audio Stream: destroyed vorbis decoder", 0);
		
		fclose(mFile);
		mFile = 0;
		logDebug("Vorbis Audio Stream: closed file", 0);
		
		mPosition = 0;
	}
}

int AudioStream_Vorbis::Position_get()
{
	return mPosition;
}

bool AudioStream_Vorbis::HasLooped_get()
{
	return mHasLooped;
}

//

static void DuplicateInPlace(short * buffer, int numSamples)
{
	short * src = buffer + (numSamples - 1) * 1;
	short * dst = buffer + (numSamples - 1) * 2;
	
	for (int i = 0; i < numSamples; ++i)
	{
		const short value = src[0];
		dst[0] = value;
		dst[1] = value;
		src -= 1;
		dst -= 2;
	}
}
