#ifdef BBOS
#define HAS_VORBIS 1
#else
#define HAS_VORBIS 1
#endif

#include <stdio.h>
#if HAS_VORBIS
	#if defined(IPHONEOS) || defined(BBOS)
		#include "ivorbisfile.h"
	#else
		#include <vorbis/vorbisfile.h>
	#endif
#endif

#include "AudioStreamVorbis.h"

#if defined(IPHONEOS) || defined(BBOS)
	#define USE_TREMOR 1
#else
	#define USE_TREMOR 0
#endif

AudioStream_Vorbis::AudioStream_Vorbis()
	: mNumChannels(0)
	, mSampleRate(0)
	, mFile(0)
	, mVorbisFile(0)
	, mPosition(0)
	, mLoop(false)
	, mHasLooped(false)
{
#if HAS_VORBIS
	mVorbisFile = new OggVorbis_File();
#endif
}

AudioStream_Vorbis::~AudioStream_Vorbis()
{
	Close();
	
#if HAS_VORBIS
	delete mVorbisFile;
	mVorbisFile = 0;
#endif
}

int AudioStream_Vorbis::Provide(int numSamples, AudioSample* __restrict buffer)
{
#if HAS_VORBIS
	Assert(mFile != 0);
	
	if (mFile == 0)
		return 0;
	
	int bytesRemain = numSamples * sizeof(AudioSample);
	int bytesRead = 0;
	
	char* bytes = (char*)buffer;
	
	mHasLooped = false;
	
	while (bytesRemain != 0)
	{
#if USE_TREMOR
		int bitstream = -1;
		
		const int currentBytesRead = ov_read(
			mVorbisFile,
			bytes + bytesRead,
			bytesRemain,
			&bitstream);
#else
		int bitstream = -1;
		
		const int currentBytesRead = (int)ov_read(
			mVorbisFile,
			bytes + bytesRead,
			bytesRemain,
			0,
			2,
			1,
			&bitstream);
#endif
		
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
			Assert(currentBytesRead <= bytesRemain);
			
			bytesRemain -= currentBytesRead;
			bytesRead += currentBytesRead;
		}
		
	}
	
	Assert((bytesRead % sizeof(AudioSample)) == 0);
	
	int numReadSamples = bytesRead / sizeof(AudioSample);
	
	mPosition += numReadSamples;
	
	return numReadSamples;
#else
	for (int i = 0; i < numSamples; ++i)
	{
		buffer[i].channel[0] = 0;
		buffer[i].channel[1] = 0;
	}
	return numSamples;
#endif
}

void AudioStream_Vorbis::Open(const char* fileName, bool loop)
{
#if HAS_VORBIS
	Close();

	mFileName = fileName;
	mLoop = loop;
	
	mFile = fopen(mFileName.c_str(), "rb");
	
	if (mFile == 0)
	{
		Assert(mFile != 0);
		Close();
		return;
	}
	
	LOG_DBG("Vorbis Audio Stream: opened file: %s", mFileName.c_str());
	
	if (ov_open(mFile, mVorbisFile, NULL, 0) != 0)
	{
		LOG_ERR("Vosbis Audio Stream: failed to create vorbis decoder", 0);
		Close();
		return;
	}
	
	LOG_DBG("Vorbis Audio Stream: created vorbis decoder", 0);
	
	vorbis_info* info = ov_info(mVorbisFile, -1);
	
	mNumChannels = info->channels;
	mSampleRate = static_cast<int>(info->rate);
#endif
}

void AudioStream_Vorbis::Close()
{
#if HAS_VORBIS
	if (mFile != 0)
	{
		ov_clear(mVorbisFile);
		LOG_DBG("Vorbis Audio Stream: destroyed vorbis decoder", 0);
		
		fclose(mFile);
		mFile = 0;
		LOG_DBG("Vorbis Audio Stream: closed file", 0);
		
		mPosition = 0;
	}
#endif
}

int AudioStream_Vorbis::Position_get()
{
	return mPosition;
}

bool AudioStream_Vorbis::HasLooped_get()
{
	return mHasLooped;
}
