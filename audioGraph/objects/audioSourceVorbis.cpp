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
#include "audioSourceVorbis.h"
#include "audiostream/oggvorbis.h"
#include "Debugging.h"
#include "Log.h"

AudioSourceVorbis::AudioSourceVorbis()
	: file(nullptr)
	, vorbisFile(nullptr)
	, sampleRate(0)
	, numChannels(0)	
	, position(0)
	, loop(false)
	, hasLooped(false)
{
	vorbisFile = new OggVorbis_File();
}

AudioSourceVorbis::~AudioSourceVorbis()
{
	close();
	
	delete vorbisFile;
	vorbisFile = nullptr;
}

void AudioSourceVorbis::generate(SAMPLE_ALIGN16 float * __restrict samples, const int numSamples)
{
	Assert(file != nullptr);
	
	if (file == nullptr)
	{
		memset(samples, 0, sizeof(float) * numSamples);
		return;
	}
	
	int numSamplesRemaining = numSamples;
	
	hasLooped = false;
	
	while (numSamplesRemaining != 0)
	{
		int bitstream = -1;
		
		float ** channels;
		
		int numSamplesRead = (int)ov_read_float(
			vorbisFile,
			&channels,
			numSamplesRemaining,
			&bitstream);	
		
		if (numSamplesRead < 0)
		{
			numSamplesRead = 0;
		}
		
		//LOG_DBG("read %d samples", numSamplesRead);
		
		const int sampleOffset = numSamples - numSamplesRemaining;
		
		if (numChannels == 1)
		{
			memcpy(samples + sampleOffset, channels[0], numSamplesRead * sizeof(float));
		}
		else if (numChannels == 2)
		{
			memcpy(samples + sampleOffset, channels[0], numSamplesRead * sizeof(float));
			// todo : add the second channel
		}
		
		if (numSamplesRead == 0)
		{
			// reached EOF
			
			if (loop)
			{
				// not done yet!
				
				close();
				
				//open(mFileName.c_str(), loop);
				
				hasLooped = true;
			}
			else
			{
				// we're done
				
				break;
			}
		}
		else
		{
			Assert(numSamplesRead <= numSamplesRemaining);
			
			numSamplesRemaining -= numSamplesRead;
		}
	}
}

void AudioSourceVorbis::open(const char * filename, bool _loop)
{
	close();

	loop = _loop;

	file = fopen(filename, "rb");
	
	if (file == nullptr)
	{
		LOG_ERR("Vosbis Audio Stream: failed to open file (%s)", filename);
		Assert(file != 0);
		close();
		return;
	}
	
	LOG_DBG("Vorbis Audio Stream: opened file: %s", filename);
	
	const int result = ov_open(file, vorbisFile, NULL, 0);

	if (result != 0)
	{
		LOG_ERR("Vosbis Audio Stream: failed to create vorbis decoder (%d)", result);
		close();
		return;
	}
	
	LOG_DBG("Vorbis Audio Stream: created vorbis decoder", 0);
	
	vorbis_info * info = ov_info(vorbisFile, -1);
	
	sampleRate = static_cast<int>(info->rate);
	numChannels = info->channels;
	
	LOG_DBG("Vorbis Audio Stream: channelCount=%d, sampleRate=%d", numChannels, sampleRate);
	
	if (numChannels != 1 && numChannels != 2)
	{
		LOG_ERR("Vosbis Audio Stream: channel count is not supported (%d)", numChannels);
		close();
		return;
	}
}

void AudioSourceVorbis::close()
{
	if (file != nullptr)
	{
		ov_clear(vorbisFile);
		LOG_DBG("Vorbis Audio Stream: destroyed vorbis decoder", 0);
		
		fclose(file);
		file = 0;
		LOG_DBG("Vorbis Audio Stream: closed file", 0);
		
		position = 0;
	}
}
