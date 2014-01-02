#include <stdio.h>
#include "AudioStreamVorbis.h"
#include "internal.h"
#include "oggvorbis.h"

AudioStream_Vorbis::AudioStream_Vorbis()
	: mNumChannels(0)
	, mSampleRate(0)
	, mFile(0)
	, mVorbisFile(0)
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
		int bitstream = -1;
		
		const int currentBytesRead = (int)ov_read(
			mVorbisFile,
			bytes + bytesRead,
			bytesRemain,
			0,
			2,
			1,
			&bitstream);
		
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
	
	mFile = fopen(mFileName.c_str(), "rb");
	
	if (mFile == 0)
	{
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
	
	mNumChannels = info->channels;
	mSampleRate = static_cast<int>(info->rate);
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
